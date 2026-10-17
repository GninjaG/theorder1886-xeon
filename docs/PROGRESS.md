# Progress log (The Order: 1886 port)

Short, dated state for resuming work. Newest first.

## 2026-10-17 (17:35): the player's manual session, windows-v0.4.0

**~20 FPS in a building after the rooftop (60 FPS mode):** the player's 0.3.9 log (88 MB) held
707k "Buffer binding in shader ... isn't dword aligned" warnings, 4-7k a second there. Windows'
CRT writes an unbuffered stream that is a pipe or a file one character per system call:
~80 us a line through the launcher's pipe, ~180 us into a file, ~2 us with one fwrite
(scratchpad micro-benchmark). The GPU log now formats the line and writes it once, holding
stdout's lock as well (as gow3_pc 0d49242, so runtime printf lines no longer interleave); the
warning prints once per shader. Rooftop, 60 FPS, 16:9, 120 s: 0.3.9 31.9/31.6/30.8 FPS, now
38.4/38.0. Runtime printf (stdout _IONBF, probe.c) still goes per character: ~20 lines a second.
**Black spots on faces:** shadPS4 players saw them too (PR #4798 comments: Galahad's face, the
lycans). In their screenshot the spots are hexagons at the game's black level, the depth of
field's bokeh shape (motion blur stretches them): NaN pixels. Ported shadPS4 #5257/#5346:
unordered N* compares, NaN-skipping min3/max3, med3 with NaN = min3, saturate NaN -> 0,
V_RSQ_LEGACY_F32 (rsq(0) = 0). ShaderBinaryVersion 9. Some programs leave DX10_CLAMP off (seen
with a temporary log); zero is used for them too. Not reproduced: the scripted prologue and
rooftop runs showed no spots before or after.
**Aspect ratio:** the 3D view is right in gameplay and cutscenes at 16:9 (same horizontal view
as 2.4, more above and below; dense captures paired by content). The Iggy UI is laid out again
for the taller picture: glyphs ~1.07x, positions spread with the height (~1.33x) and the
chapter title's right edge runs 35 px past the screen. The player's screen is 1920x1200
(16:10). Next: find the UI's display size/layout (the aspect lives at display device +0xe0,
copied from the video config at +0x98; no float reads of [reg+0xe0] near loads of 0x16b1df0).
**GitHub #11 (RX Vega 64, chapter IX):** vk::Result=ErrorOutOfHostMemory; the reporter's
chapter IX save (shadPS4 PR #4798, converted: param.sfo -> param.bin) loads and runs 150 s here.
Check() now prints the calling file and line.
**REA:** the MCP server resolves to the source folder again (native/windows/build missing,
Ghidra refused); rizin and scratchpad scanners were used instead.

## 2026-10-17 (10:25): scene start pipeline bursts, windows-v0.3.6

Known pipelines (stored keys) build on the GPU thread for a 1-2 ms binary restore; a scene start
needs ~100 at once (0.3-1.4 s frames). vk_pipeline_cache.cpp SyncBuildBudgetLeft: past 8 ms of
such builds in a 33 ms window (CompileTimer off the compiler workers) the rest build in the
background (PortToggle::SyncBuildBudget, bit 59). Prologue A/B 3+3: worst frames 167-366 vs
267-300 ms; rooftop load 300-399 vs 833 ms. Frames dumped right after the burst
(scratchpad dump_after_burst.py: THEORDER_FRAME_DUMP_TRIGGER re-created after a "Stall:" line)
are complete: the waiting draws fall in the load.

## 2026-10-17 (09:50): a code buffer crash and pipeline freezes, windows-v0.3.5

**Crash:** a prologue run stopped with "code is too big" (Xbyak) in a GPU thread coroutine.
Every shader permutation appended its own copy of the SRT walker to the 32 MB code buffer
(never freed); ~3000 copies in 80 s were identical to one of ~200 walkers (26 MB vs 0.4 MB).
Identical walkers are now shared (flatten_extended_userdata_pass.cpp; "SRT walkers:" lines).
**Freezes:** "Pipeline:" lines (any pipeline over 50 ms in the driver) showed 4-8 compiles of
130-190 ms on the GPU thread per run, warm cache, new keys each run. The stage hash is
HashCombine(program, permutation index), and permutations made this session take the next
free index; material shaders have hundreds (image type 2D/array and sRGB of each texture,
e.g. while the 1x1 streaming placeholder is bound: THEORDER_PERMUTATION_LOG=1). A stored key
with that index (another session's permutation) made the pipeline "known", built on the GPU
thread for a binary restore that missed (binaries are keyed by the driver's key). Pipelines
with a permutation made this session (Program::Module::fresh) now build in the background.
2-4 a run remain (unskippable compute dispatches; keys whose index a later session reused).

## 2026-10-17 (09:10): what the GPU thread's Finish waits are for

After 0.3.4 the GPU command thread spends ~20% in BufferCache::DownloadMemory -> Finish on the
rooftop (60 FPS mode). New THEORDER_FRAME_STATS line "Buffer downloads by time" (buffer_cache.cpp)
and THEORDER_GPU_WRITE_WATCH: one download a frame for the window 0x106ea80000 (~4.6 ms each), set
off by CPU writes (the compute queue's ReleaseMem label, liverpool.cpp; the compute command buffer,
sceGnmDispatchInitDefaultHardwareState) to a page where a compute dispatch binds two 192-byte
written storage buffers; and the eboot's own write at 0xcc8791 to 0x105b500000 (~20 ms/s). The
"Mapped ..." line now prints the guest image address (RIP 0x800cc8791 = eboot 0xcc8791). Details
and the next step: docs/BACKLOG.md item 3.

## 2026-10-17 (08:40): the mapped range check, windows-v0.3.4

**Profile** (THEORDER_SAMPLE='DrawRec|VkRecorder|GpuCommand', rooftop, 60 FPS mode): a fifth of
the draw recording thread (stage B) and a tenth of the GPU command thread went to
Rasterizer::IsMapped (behind MemoryManager::IsValidGpuMapping, checked for every resource of
every draw): its Common::RecursiveSharedLock on a SharedFirstMutex takes a std::mutex and a
condition variable per call, plus the recursive lock's per-thread hash map; the interval lookup
itself is cheap.
**Change** (PortToggle::MappedRangeCopies, bit 58): each thread checks its own copy of
mapped_ranges, taken again when mapped_ranges_generation (bumped under the lock by MapMemory
and UnmapMemory) moved. Rooftop, 60 FPS mode, 3+3 interleaved runs: ~57k -> ~71k draws/s,
~33 -> ~39 FPS, stage B 73-81% -> 68-74% busy.

## 2026-10-17 (later): what the graphics queue waits on

**Measured** (rooftop, 60 FPS mode, THEORDER_FRAME_STATS=1, new "GFX memory waits" line in
liverpool.cpp): the graphics queue's WaitRegMem drains (37-40% of the GPU command thread) are all
for three labels the compute queues write (0x10661a53f8, 0x1066182010, 0x1066182020; one compute
write per wait). A drain never met the wait: the compute queues run only when the graphics task
yields, and before every packet they drain themselves ("compute work uses the caches").
**Tried, not kept:** compute queue dispatches, WriteData, DmaData, ReleaseMem and Rewind handed
to the draw recording thread in order (RunInOrder, ReleaseMem labels noted as pending fences),
pending labels counted as written on both queues, the graphics wait yielding before it drains.
The WaitRegMem drains went from ~495 to ~2 ms/s, but the rooftop went from 55-65k to 48-50k
draws/s (two A/Bs of 3+3 interleaved runs): the compute recording moved onto stage B, the limit
(76-78% busy before, 87-90% after). While stage B is the bottleneck, stage A's drains are where
it records the compute work itself, not lost time.
**Next:** stage B's cost per draw (backlog 1).

## 2026-10-17: GitHub issues #3 and #1, windows-v0.3.3

**#3 (0.3.1 crashes at start, AMD RX 9070 XT, eboot 01.00, sha256 b451e437...):** fault at guest
offset 0x9b47f9, next to 0x9b47fc, one of the sound engine bring-up trace hooks probe.c
installed at the 01.02 offsets in any eboot (install_order1886_diagnostics). Those hooks now go in
only where every site holds the call/branch/sequence they replace (order1886_hook_sites_match).
Patch lines without a Pattern fell back to their 01.02 Address in any build: scripts/patches.py
now skips them outside the reference build (`reference`, sha256 == REFERENCE_SHA256). The 60 FPS
and aspect ratio patches got signatures (scratchpad generator: instructions before the site, rip
displacements and branch offsets as ??; the FOV float64 constants as Targets through the vmulsd
that loads each one, Ref="1"); on 01.02 they write exactly what the Addresses did
(test_builtin_signatures_find_the_reference_sites, needs out/eboot.elf). Not tested on 01.00.
**#1 (0.2.0, Intel Arc iGPU, characters black in the prologue cell):** the log has 58
"Patched SRT walker" in the frame with the first compiles. The shadPS4 SRT walker fault handler
patches a faulting load to `xor reg, reg` for good, and it ran before runtime_fault_recover
(probe.c dispatches to Core::Signals first): a speculative walk (draw preparation workers,
vk_pipeline_cache.cpp; the verify path in vk_rasterizer.cpp) reading reused memory zeroed that
table entry for every later draw of the shader. It now returns to the recovery point. Never seen
here on NVIDIA (0 patches, warm or cold cache, out/user_cold), so the Intel effect is unconfirmed.
Patched walker code is also written into later permutations' .meta (RegisterShaderMeta copies
the live code); caches from 0.2.0 are dropped anyway (ShaderBinaryVersion changed in d5ee454).

## 2026-10-16 (evening): gameplay below 30 FPS, the memory map lock

**What the player saw:** the user's 9.5-minute 0.3.0 session (dist user/last_run.log, 60 FPS
patch, 16:10) had 26 of 114 five-second windows under 20 FPS. Reproduced from their save (copy in
out/user_gameplay, THEORDER_USER_DIR; the London rooftop at "Identify the United India House"):
~1430 draws/frame, 23-25 FPS at the native 30, the GPU command thread never idle.
**New tools:** `THEORDER_SAMPLE=<thread name part>[|...]` (probe.c, Windows): samples the named
threads' eight innermost frames every ms (THEORDER_SAMPLE_START/_SECS/_OUT); `tools/sample_report.py`
names them from the DWARF (self and inclusive per thread). The us/draw statistic now works on
Windows (GetThreadTimes).
**Found:** `runtime_memory_clamp` (ClampRangeSize of every buffer binding) was 29% of the GPU
command thread and 25% of the draw recording thread, nearly all inside winpthreads' rwlock (an
internal mutex per acquisition). The memory map lock is an SRWLOCK on Windows now (nested reads
of a thread acquire it once): 21 -> 14-17 us/draw, the rooftop at 28-30 FPS.
Next: the GPU thread waits for the draw recording thread at WaitRegMem packets (22% of its time,
~90/s) and for the GPU in BufferCache::DownloadMemory (11%).

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
file 10.4 ms (a cold file cache), binary handles 0.26 ms, driver 1.3 ms. fread instead of
std::ifstream (e3909ef), 3+3 interleaved prologue runs: file 3.2-3.4 -> 1.8-2.2 ms per pipeline
(the first ifstream run, colder: 6.0), the prologue-start burst of ~95 pipelines 477-501 ->
262-390 ms (first run 1001).
Tried and reverted: draw-preparation workers starting the restore of stored pipelines on the
compiler threads (the GPU thread waiting for them). 3+3 interleaved runs: no change in the
bursts (validation clean). A count showed why: in a 120 s prologue run workers started 12 such
restores while the GPU thread restored 525 stored pipelines itself; at a scene load the draws'
shader programs are not loaded yet (LoadCachedProgram runs on the GPU thread), so workers cannot
select their pipelines. Next, if the pauses still matter: let workers load cached programs, or
read the binaries ahead from an emulator key -> driver binary name index.
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
