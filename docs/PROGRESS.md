# Progress log (The Order: 1886 port)

Short, dated state for resuming work. Newest first.

## 2026-10-16 (later): REA decompiles the eboot

REA's Ghidra provider admits only PE images on Windows: `scripts/eboot_pe.py` wraps the eboot in
one (docs/ENGINE_RE.md, "Decompiling with REA"), imported with the System V convention. Checked
with the REA CLI: `main` decompiles with its imports named (`memset`, `__stack_chk_fail`) after a
12-minute first analysis. Two setup problems: `npx rea-agents@6.5.0` ran a global `npm link` of a
source build without the Windows native addon (the published package is now in
`C:\Users\Mateus\tools\rea`), and Claude Code's first start of the server timed out while npx was
downloading it. The MCP entry now runs that package (Claude Code reaches the tools).

**Black triangle, hypothesis dropped:** the fullscreen vertex buffer is not rewritten every
frame. `THEORDER_WATCH=<hex address>:<bytes>` (new diagnostic, runtime_memory.c) saw the 60 bytes
at 0x1059c18988 written once and unchanged for 80 s; the same address in all 20 captured frames.
Next: look at the port side (buffer cache copy of that range, or the V# of the bad frame).
**Startup fault 0x20aa22:** decompiled (docs/ENGINE_RE.md, "Task system queue"); the push
(`sub_20b010`) follows the same tagged Michael-Scott scheme, so the bad head index comes from
outside the algorithm. A fault there now prints r8-r15 and the queue header (head, tail, free
list, count, node array); not seen in 40 short startups.
**Startup crash in iertutil.dll (fixed):** 1 of those 40 runs died on GLib's `pool-0` thread
inside iertutil.dll (via mrmcorer.dll). libgio's DllMain starts a thread that enumerates the
installed UWP apps (gwin32appinfo.c); GIO came in only through FFmpeg's SVG decoder (avcodec ->
librsvg -> gio, pango, gdk-pixbuf). `packaging/windows/rsvg_stub.c` builds a librsvg-2-2.dll with
the three functions avcodec imports; beside the executable (out/ and the package's bin/) it is
loaded instead, and libgio, pango, gdk-pixbuf, iertutil and mrmcorer no longer load (checked on
a running game).
**Scene-load pauses:** the pipeline binary report now splits a restore: driver key 0.04 ms,
file 10.4 ms, binary handles 0.26 ms, driver 1.3 ms. Reading the 2 MB files with fread instead
of std::ifstream (e3909ef): file 2.1-2.6 ms, 11-16 ms per pipeline built in a stall (was 22).
Next if still worth it: read the binaries of the upcoming draws ahead on a few threads (needs
an emulator key -> driver binary name index; the name comes from vkGetPipelineKeyKHR).
**`-startlevel`:** still the title screen with `pro_010` + `pro_010_checkpoint_005`, also with
the `*(game+0x208)` gate at 0x4480cf patched out and after one ✕ press. Next: find where the
stored level hash (`param_1[6]`) is used after the title.

## 2026-10-16: God of War leftovers removed, aspect ratio option

**Cleanup (18c2ae5, d36f8f4, 90a0060):** God of War III's cheats, patches, glyphs, fonts, icons,
tools and docs are gone; the launcher only offers options that work for The Order. Every
`GOW3_*` variable is `THEORDER_*`, the executables are `theorder-*`, the settings file is
`theorder.ini` (run.py and the launcher move an old `gow3.ini` once), code identifiers `Port*`/
`port_*`.

**Aspect ratio (launcher Graphics/home, in-game menu Image tab; applied on restart):**
`aspect_ratio=original|16:9|16:10|21:9` in theorder.ini; run.py adds "Aspect Ratio - <name>".
- The engine's video setup stores the picture aspect 2.4f (immediate at 0x44ce8c) and centers a
  picture of that shape in the 1920x1080 display buffer (measured: 2.4 -> 1920x800 at y 140,
  64:27 -> 1920x810 at y 135, 1.6 -> 1728x1080 at x 96). Its scene targets take the picture's
  size (1920x810 render targets at 21:9).
- Cameras keep a horizontal FOV defined for 16:9 and turn it into a vertical one with
  `2*atan(tan(h/2) * 0.5625)` (doubles in each function's constant pool: 0x147f2b0, 0x147f2d0,
  0x147f3c0/0x147f3d0 projection builders at 0x8c6a44/0x8c7a42, 0x147f5e8, 0x147f610, 0x147f670).
  The aspect patch alone kept the vertical view and cropped the sides (Hor-); the patches also
  scale those constants by 2.4/aspect, so the view keeps the original width and shows more above
  and below (Vert+, checked on the title menu's background: same logo size and position).
  0x147f5d0 stays: its function converts back with 2.4 (0x147f5d8), still right under Vert+.
- The presenter keeps only the picture (`Presenter::picture`, `PortGraphics::PictureRect`): the
  frame is sized so the picture fills the window (FSR 1 and the post pass still see the whole
  image) and the swapchain blit copies the picture. Checked with 21:9 (1280x540) and 16:10
  (1152x720) windows: no bars, no edge lines; validation layer: no new messages.
- Cost at 60 FPS in the prologue (RTX 5060 Laptop): the scene has 35% more pixels at 16:9; 57.8
  against 58.3 FPS on average over 3+3 interleaved 70 s runs, 3960 flips in both (the GPU command
  thread, not the GPU, limits this machine).

**Crash fixed (the user's, at a gameplay checkpoint):** `_start` (0xa0) calls the initializer
at 0x20 itself, after `_init_env` and atexit, as on the PS4; probe.c called it as well, so every
global was constructed twice and the engine's memory pools (8+24+810+654+52 MiB) were mapped
twice. Direct memory then ran out when a save loads (`direct_allocate loop reject` at
0x13c000000, then the Streaming IO thread read -1 at 0x7fe12b): reproduced with the user's save
on the old build at 36 s. probe.c now leaves the initializer to the entry (`entry_calls`); with
the same save the game runs 7 min, 4.22 of 5.3 GB of direct memory in use, allocations only at
start and at the load. The watchdog stop prints the direct memory still in use.
**Flat light blue gameplay fixed (`specialization.h`):** from that save the gameplay (cover
tutorial) showed one flat color (171,211,242) with the HUD. Per-pass dumps of a captured frame
(`THEORDER_DIAG_IMAGES=1`, now with depth, `THEORDER_DIAG_SAMPLED=1` for sampled textures and
`THEORDER_DIAG_DRAW_TARGET=<address>` for every draw into one target) showed the scene fine until
a half-res fog buffer (0x104ea60000) added ~185 everywhere; a simulation of its analytic
in-scattering shader with the captured constants gave ~0.004. The buffer still held the loading
screen: its clear (fullscreen VS e5979e2b, x = s0 + {-1,3}) never drew. Captures now note each
stage's user data registers: the VS read ud_regs0.x, the fragment shader's descriptor pointer
(~1982.0 as a float), while its own s0 (0.0) was pushed at ud_regs0.z. StageSpecialization
ignored the start bindings of shaders without resources, so the permutation compiled after a
fragment shader without user data was reused after one with two registers. Now compared.
Also (not the cause): V_MAC_LEGACY/V_MAD_LEGACY use the legacy multiply (0 * Inf = 0) and the
_CLAMP rcp/rsq/log clamp infinities to max_float (ShaderBinaryVersion 8).

**Engine notes:** `docs/ENGINE_RE.md` collects what the executable showed: start-up order,
video setup and display-mode flags, the FOV constants, and the developer command line the
retail build still parses (`THEORDER_ARGS`: `-cores`, `-use4K`, `-syncinterval`, `-startlevel`
... tried without effect so far).

## 2026-10-15 (night): windows-v0.2.0

**State:** 70 s prologue runs (warm cache): 0 faults in 30+ runs this session, 30 FPS after the
intro, 55-111 skipped draws per run (was 1600-2100), 1.4-2.1 s of compile on the GPU thread (was
9-11 s), a few scene-load pauses (worst frame 470-600 ms when ~200 pipelines are restored at the
prologue start). 60 FPS mode, 240 s: 57 FPS average, 41-55 in the heaviest windows (600-700
draws per frame: the GPU thread is then busy all frame).

**Fixed (commits 1be3d5f..):**
1. LoadCachedProgram read at most 256 permutations; the particle lighting compute shader
   `0x804aeead` has 500+, so each session re-translated the rest under new indices and their
   pipelines missed the binary cache (~200 ms driver compiles each, 2 MB binaries saved again:
   41 GB). Limit 4096; metas record the SPIR-V hash (ShaderMetaVersion 8), checked at load.
2. That shader samples 16 SH light volumes (3D) whose inactive slots hold stale T#s (cube maps,
   render targets, unmapped addresses): `ImageResource::GetSharp` reads as Null what the
   rasterizer would not bind (unmapped address via `Shader::g_valid_image_address`, size limits,
   base mip past the chain) and, for plain sampling, a cube without the MIMG DA bit or a 3D image
   with it. New permutations per run: ~120 -> ~20.
3. `SkippableDispatch`: a new compute permutation builds in the background (dispatches skipped)
   when the program ran in 8+ frames up to the last 4 flips and writes no image (Iggy's one-shot
   glyph copies keep the synchronous build). Frames counted by `PortStats::flips` (gpu_frames
   only advances with camera motion).
4. Pipeline binary cache budget (4096 MiB, `THEORDER_PIPELINE_BINARY_CACHE_MB`), trimmed by mtime on
   the meta prefetch thread; a hit refreshes the file time. The user's 0.1.0 session ended with
   "No space left on device".
5. `ResolveOverlap` no longer stops the game on an equal-address overlap (stale T# with garbage
   depth/pitch over a real texture); `ImplausibleTexture` also rejects guest size > 16 MiB and
   64x the texels.
6. Pipelines whose key is stored (`g_`/`c_` .key, `DataBase::Exists`) are created on the GPU
   thread (binary restore) instead of skipping their draws while an async job runs: the skipped
   draws were the "flicker" the user reported (20344 in their 5.5 min 0.1.0 session).
7. Removed patches: 900p, 720p and Letterbox Removal wrote inside instructions (0xe6cfb2 is the
   displacement of `movl 0x4710(%r9)`; 0xa57cdd/ce3 a rip-relative offset).

**Found, not done (next):**
1. **Black triangle flicker**: a frame dump (out/flicker/corr frame 24, flip 1776, prologue
   ~62 s) shows the display buffer's clear color (0,0,4) over the upper-left of the picture,
   bounded by a straight diagonal: a fullscreen pass covered only part of the screen. The final
   composite VS (b9c87fc1) and the clear VS (e5979e2b) build positions from gl_VertexID, but
   0ece32eb (tonemap #111 and other fullscreen passes) reads position and UV from a vertex
   buffer the engine rewrites every frame (docs/parallel_gpu.md: EOP fences are signalled at
   record time). One wrong vertex gives exactly this shape. Not reproduced since (2 runs with
   consecutive dumps at 55-66 s; the fast camera pans there blur legitimately). Next: find how
   that 60-byte vertex buffer reaches the GPU (constant ring vs arena re-upload) and whether a
   later upload can overwrite it before the GPU reads it. Captures now print the flip
   (`presented buffer ..., flip N`) to match frame dumps.
2. Scene-load pauses: restoring ~200 known pipelines on the GPU thread costs ~5 ms each (2 MB
   binary read + checksum + driver). Prefetch them from the draw-preparation workers' keys
   (PreparedDraw::key) onto the compiler threads before the GPU thread needs them.
3. Remaining particle shader permutations (2D vs 3D T#s in DA=0 slots): a dual 2D/3D
   declaration per slot, selected at run time, would end them.
4. Rare guest fault at startup: `guest offset 0x20aa22` (lock-free queue pop in a Task Kernel
   thread, garbage head index), 1 in ~40 runs.
5. 60 FPS: the GPU thread is the limit in heavy scenes (~25-30 us per draw).

## 2026-10-15 (release): windows-v0.1.0

**Fixed:** menus without text or button icons (the title's "PRESS ✕", every menu). Iggy copies
each glyph and button icon into its textures once, with a compute dispatch (`0xc98f383802f87e7d`,
storage image write). The asynchronous compute pipelines of the entry below skipped dispatches
whose pipeline was still being built, so those copies never ran and the textures stayed empty.
Compute pipelines are built on the GPU thread again by default; `THEORDER_ASYNC_COMPUTE=1` keeps the
faster, lossy path. Cost: compile stalls of new compute permutations are back (~15 s of compile
time in the first 70 s of a session; the "mini stutters").

**Release:** the Port launcher, adapted (`The Order 1886.exe`, settings in
`%APPDATA%/theorder1886-launcher`, frame rate 30 original / 60 experimental: `run.py` adds flip
rate 0 and a 60 limit with the 60 FPS patch). `packaging/windows/package.sh`, run from MSYS2's own
bash (Git Bash's runtime expands its `find` patterns), builds `dist/theorder1886-windows.zip`.

**Next:** fewer stalls without losing one-shot work: build compute pipelines in the background
but wait, instead of skipping, for dispatches that write images (check first what `0x804aeead`
writes: if images too, it needs another rule).

## 2026-10-15 (later): branch fix/nvidia-crashes-and-shader-specialization, 15 commits

**State:** 4-minute prologue session, 0 faults, every 5 s window at 30.0 FPS, p99 frame time
~34.5 ms. 20+ consecutive crash-free runs since the draw-pipe race fix.

**Since the entry below (all committed):** async compute pipelines (stalls 9.4 s -> 0.1 s per
40 s); textures over 512 MiB of computed size bound as null (stale T#s with garbage pitch/layers
uploaded 1.3-3.3 GB: 1-3 s freezes); FMask read replacements return F32x4 and Flat only on
fragment inputs (both invalid SPIR-V); lazy load of a program's cached permutations
(`THEORDER_LAZY_SHADER_CACHE`); `THEORDER_FLIP_RATE`; `THEORDER_BIG_TRACK_LOG`; tools: `--game-args`, `--pad`
(`--pad "24:cross 28:cross 32:cross ..."` reaches the in-engine prologue).

**60 FPS (experimental, verified):** the patch in `patches/TheOrder1886-Orbis.xml` changes the
four 33333 us immediates of the 01.02 eboot to 16667 (checked in the binary). Recipe: an ini with
`engine_fps=120` as `THEORDER_CONFIG` (the launcher falls back to the 60 FPS patch), plus
`THEORDER_FLIP_RATE=0 THEORDER_FPS_LIMIT=60`. Game speed is correct (frame dumps line up in time with a 30
FPS run); the intro holds 60, the 3D prologue 45-58 FPS: the GPU thread is 77-93% busy.

**Next:**
1. Motion-vector vertex shaders are retranslated every session (`LoadShaderMeta` rejects them:
   they embed `MotionVectors::positions_address/params_address`, emit_spirv_special.cpp). Pass the
   addresses as push constants or a fixed binding instead, so they can be cached: the remaining
   ~6.7 s of capture waits per 90 s at 60 FPS.
2. Launcher: for The Order, 30 is native and 60 needs the patch plus flip rate 0 and a 60 limit
   (scripts/graphics_options.py treats 60 as native, inherited from Port).
3. `VUID-vkCmdDispatchIndirect-magFilter-04553`: linear sampler with an integer image (benign on
   NVIDIA so far).
4. Test beyond the prologue (the only save, `Playthrough00`, is there too).

## 2026-10-15

**How to test (use these, not single runs)**
- `bash build.sh` (MSYS2 CLANG64). If the copy to `out/theorder-probe.exe` fails ("busy"), a game
  process is still alive: the build did not update the exe. `tools/batch_runs.py` kills orphans
  and prints the exe build time.
- `python tools/batch_runs.py --runs 8 --secs 40 --cond base= --cond x=THEORDER_VAR=1`: interleaved
  A/B runs, failure rates (fault / stopped presenting). Logs in `out/batch/`.
- `python tools/fault_stacks.py out/batch/*.log`: symbolized fault stacks.
- Vulkan validation (MSYS2 package `mingw-w64-clang-x86_64-vulkan-validation-layers`):
  `THEORDER_VK_VALIDATION=1 VK_ADD_LAYER_PATH='C:\msys64\clang64\bin' VK_LOADER_LAYERS_ENABLE='*validation*'`.
  Count the VUIDs before and after a change; it found most of the bugs below in minutes.
- Game dump: `C:\Users\Mateus\Downloads\games\CUSA00035`. One game run reserves ~6 GB of RAM;
  a background batch was stopped once by Claude Code for low system memory.

**Fixed and verified (uncommitted)**
1. NVIDIA driver crashes (`nvoglv64.dll+0xe1a0a0` / `+0xee8804`, null reads at 0x130/0x134, on
   the GPU, draw-recording and recorder threads; once a corrupt record chunk in our code): a race.
   `Liverpool::ProcessCommands` drained the draw pipe only if commands were queued *before* its
   loop; a guest `sceVideoOutSubmitFlip` queued in between ran `PrepareFrame` while the draw
   thread recorded into the same command buffer. Now drained per command, and the presenter
   drains right before recording into `draw_scheduler` (`liverpool.cpp`, `vk_presenter.cpp`).
   40 s runs: 5/8 failed before, 8/8 clean after (HEAD: 2/6 failed). Found with
   `THEORDER_STAGE_CHECK=1` (scheduler use while the draw thread runs a packet).
2. Stencil attachment bound for depth-only views (VUID-VkRenderingInfo-pStencilAttachment-06548):
   depth and stencil attached independently, stencil only when the image has it
   (`vk_scheduler.cpp`, `vk_rasterizer.cpp`). Commit b0bfec1 had made both depend on either.
3. A failed detile left a 20 GB `vkCmdCopyBufferToImage` from the 4 GB guest buffer (GPU fault):
   copies past the source are skipped and logged (`texture_cache.cpp`).
4. Pipeline layout vs SPIR-V binding mismatch (VUID 07990/07988) in the material compute
   shader `0x804aeead`: a stale FMask T# in a texture slot dropped the image and sampler from that
   translation only. FMask reads now keep both declared, the spec records `is_fmask`, and the
   rasterizer binds FMask T#s as null (`resource_patching_pass.cpp`, `specialization.h`).
5. Stale/garbage T#s (no format, empty mip range, address < 64 KiB or >= 2^40) read as
   `Image::Null` in `ImageResource::GetSharp` (one source for spec, SPIR-V and binding); unbound
   images specialized from that null sharp; their `num_bindings` must match.
6. MSAA copies of integer images used a float shader (VUID-vkCmdDraw-format-07753): uint/sint
   variants of `ms_image_blit.frag` (`blit_helper.cpp`, `gpu/CMakeLists.txt`).
7. `Shader::Profile::translation_revision` (now 2): bump it when translation changes what a shader
   declares, so an old pipeline cache is rebuilt.
8. Windows watchdog prints every thread's stack and exits with TerminateProcess (ExitProcess
   deadlocked on the loader lock with threads suspended, leaving a zombie that locked the exe).
9. Diagnostics: `Stall:` lines split compile time (translate / SPIR-V / driver); a GFX
   `WaitRegMem` unmet for 2 s is reported; `THEORDER_PERMUTATION_LOG=1` explains new permutations.

**Implemented, not yet run**
- Asynchronous compute pipeline creation (`AsyncComputeCompilation`, `vk_pipeline_cache.cpp`):
  ~95% of the remaining stutter is the driver compiling new permutations of `0x804aeead`
  (~180 ms each, ~10 s per 40 s run, combinations of 2D/cube/3D textures per slot). Dispatches
  are skipped until the pipeline is ready, as draws already are. Builds; needs a validation run,
  a batch and a look at frame dumps (`THEORDER_FRAME_DUMP_INTERVAL`, `tools/convert_dumps.py`).

**Open**
- `THEORDER_STAGE_CHECK` still reports `PrepareFrame` once per run near flip #1 (counting added to
  tell a startup-only case from a recurring one). Temporary diagnostic: remove once settled.
- Remaining VUIDs: `vkCmdDispatchIndirect-magFilter-04553` (linear filter on R8G8B8A8_UINT),
  an occasional invalid SPIR-V (`pCode-08737`, vector index out of bounds), one
  `dynamicRenderingUnusedAttachments-08911` (garbage color format) seen once.
- REA cannot decompile `out/eboot.elf` here (no provider for ELF on Windows); not needed for the
  current GPU-side work.
- `gow3_pc` has 252 commits since 2026-10-01; MSAA/AMD fixes there are worth reviewing
  (`git -C ../gow3_pc log`), e.g. 10a7864, 6b51083, 1d34202.
