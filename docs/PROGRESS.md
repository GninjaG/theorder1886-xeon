# Progress log (The Order: 1886 port)

Short, dated state for resuming work. Newest first.

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
   glyph copies keep the synchronous build). Frames counted by `Gow3Stats::flips` (gpu_frames
   only advances with camera motion).
4. Pipeline binary cache budget (4096 MiB, `GOW3_PIPELINE_BINARY_CACHE_MB`), trimmed by mtime on
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
   ~62 s) shows a black triangle over the upper-left half of the image for one frame, with no
   compute dispatch skipped then. Likely vertex data read after the guest rewrote it (a fade
   quad). Check what the GPU reads in place vs copies before the EOP fence
   (`RunEventWriteEop`, `SignalAfterHostCopies`, GOW3_STREAM_MEMORY).
2. **Aspect ratio (16:9 / 16:10 / 21:9)**: the engine's video setup at 0x44ce86 stores 2.4f
   (`movl $0x4019999a, -0x3e8(%rbp)`, immediate at 0x44ce8c) next to the 1080p mode
   (0x780/0x438 at 0x44cf3d-0x44cf6f): the 1920x800 letterbox likely derives from it. Try
   float32 patches there (16:9 = 1.7777778; 16:10 needs a 1920x1200 display mode or a 1728-wide
   scene), and for 21:9 a presenter option that crops the letterbox (native 2.4:1 nearly fills
   2560x1080). Tools: scratchpad-style frame captures (GOW3_FRAME_DUMP_TRIGGER, now logging
   "Frame dump N at flip M").
3. Scene-load pauses: restoring ~200 known pipelines on the GPU thread costs ~5 ms each (2 MB
   binary read + checksum + driver). Prefetch them from the draw-preparation workers' keys
   (PreparedDraw::key) onto the compiler threads before the GPU thread needs them.
4. Remaining particle shader permutations (2D vs 3D T#s in DA=0 slots): a dual 2D/3D
   declaration per slot, selected at run time, would end them.
5. Rare guest fault at startup: `guest offset 0x20aa22` (lock-free queue pop in a Task Kernel
   thread, garbage head index), 1 in ~40 runs.
6. 60 FPS: the GPU thread is the limit in heavy scenes (~25-30 us per draw).

## 2026-10-15 (release): windows-v0.1.0

**Fixed:** menus without text or button icons (the title's "PRESS ✕", every menu). Iggy copies
each glyph and button icon into its textures once, with a compute dispatch (`0xc98f383802f87e7d`,
storage image write). The asynchronous compute pipelines of the entry below skipped dispatches
whose pipeline was still being built, so those copies never ran and the textures stayed empty.
Compute pipelines are built on the GPU thread again by default; `GOW3_ASYNC_COMPUTE=1` keeps the
faster, lossy path. Cost: compile stalls of new compute permutations are back (~15 s of compile
time in the first 70 s of a session; the "mini stutters").

**Release:** the GoW3 launcher, adapted (`The Order 1886.exe`, settings in
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
(`GOW3_LAZY_SHADER_CACHE`); `GOW3_FLIP_RATE`; `GOW3_BIG_TRACK_LOG`; tools: `--game-args`, `--pad`
(`--pad "24:cross 28:cross 32:cross ..."` reaches the in-engine prologue).

**60 FPS (experimental, verified):** the patch in `patches/TheOrder1886-Orbis.xml` changes the
four 33333 us immediates of the 01.02 eboot to 16667 (checked in the binary). Recipe: an ini with
`engine_fps=120` as `THEORDER_CONFIG` (the launcher falls back to the 60 FPS patch), plus
`GOW3_FLIP_RATE=0 GOW3_FPS_LIMIT=60`. Game speed is correct (frame dumps line up in time with a 30
FPS run); the intro holds 60, the 3D prologue 45-58 FPS: the GPU thread is 77-93% busy.

**Next:**
1. Motion-vector vertex shaders are retranslated every session (`LoadShaderMeta` rejects them:
   they embed `MotionVectors::positions_address/params_address`, emit_spirv_special.cpp). Pass the
   addresses as push constants or a fixed binding instead, so they can be cached: the remaining
   ~6.7 s of capture waits per 90 s at 60 FPS.
2. Launcher: for The Order, 30 is native and 60 needs the patch plus flip rate 0 and a 60 limit
   (scripts/graphics_options.py treats 60 as native, inherited from GoW3).
3. `VUID-vkCmdDispatchIndirect-magFilter-04553`: linear sampler with an integer image (benign on
   NVIDIA so far).
4. Test beyond the prologue (the only save, `Playthrough00`, is there too).

## 2026-10-15

**How to test (use these, not single runs)**
- `bash build.sh` (MSYS2 CLANG64). If the copy to `out/theorder-probe.exe` fails ("busy"), a game
  process is still alive: the build did not update the exe. `tools/batch_runs.py` kills orphans
  and prints the exe build time.
- `python tools/batch_runs.py --runs 8 --secs 40 --cond base= --cond x=GOW3_VAR=1`: interleaved
  A/B runs, failure rates (fault / stopped presenting). Logs in `out/batch/`.
- `python tools/fault_stacks.py out/batch/*.log`: symbolized fault stacks.
- Vulkan validation (MSYS2 package `mingw-w64-clang-x86_64-vulkan-validation-layers`):
  `GOW3_VK_VALIDATION=1 VK_ADD_LAYER_PATH='C:\msys64\clang64\bin' VK_LOADER_LAYERS_ENABLE='*validation*'`.
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
   `GOW3_STAGE_CHECK=1` (scheduler use while the draw thread runs a packet).
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
   `WaitRegMem` unmet for 2 s is reported; `GOW3_PERMUTATION_LOG=1` explains new permutations.

**Implemented, not yet run**
- Asynchronous compute pipeline creation (`AsyncComputeCompilation`, `vk_pipeline_cache.cpp`):
  ~95% of the remaining stutter is the driver compiling new permutations of `0x804aeead`
  (~180 ms each, ~10 s per 40 s run, combinations of 2D/cube/3D textures per slot). Dispatches
  are skipped until the pipeline is ready, as draws already are. Builds; needs a validation run,
  a batch and a look at frame dumps (`THEORDER_FRAME_DUMP_INTERVAL`, `tools/convert_dumps.py`).

**Open**
- `GOW3_STAGE_CHECK` still reports `PrepareFrame` once per run near flip #1 (counting added to
  tell a startup-only case from a recurring one). Temporary diagnostic: remove once settled.
- Remaining VUIDs: `vkCmdDispatchIndirect-magFilter-04553` (linear filter on R8G8B8A8_UINT),
  an occasional invalid SPIR-V (`pCode-08737`, vector index out of bounds), one
  `dynamicRenderingUnusedAttachments-08911` (garbage color format) seen once.
- REA cannot decompile `out/eboot.elf` here (no provider for ELF on Windows); not needed for the
  current GPU-side work.
- `gow3_pc` has 252 commits since 2026-10-01; MSAA/AMD fixes there are worth reviewing
  (`git -C ../gow3_pc log`), e.g. 10a7864, 6b51083, 1d34202.
