# Progress log (The Order: 1886 port)

Short, dated state for resuming work. Newest first.

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
  a batch and a look at frame dumps (`THEORDER_FRAME_DUMP_INTERVAL`, `convert_dumps.py`).

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
