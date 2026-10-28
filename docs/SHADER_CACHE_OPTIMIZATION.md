# Shader cache and microstutter investigation

2026-10-22 checkpoint. No claim of stutter elimination.

## Graphics motion-address specialization

`GraphicsPipeline` supplied the session's two object-motion buffer device
addresses as vertex-stage Vulkan specialization values. Those addresses are
allocated at runtime, so they became pipeline-creation inputs even though the
motion shader module itself was otherwise identical across sessions. The
motion shader now loads both `u64` addresses from push constants populated by
the rasterizer. Only graphics pipelines whose vertex runtime info enables
motion vectors use the larger 96-byte push-constant range; compute and
non-motion graphics pipelines retain the existing 80-byte range. The binary
version is raised only for motion-enabled vertex shader metadata, so unrelated
cached SPIR-V remains valid.

`motion-shader-test` exercised the actual SPIR-V backend and was red before the
change (`motion vertex shader specializes session buffer addresses`, exit 2).
It checks that these `SpecId` decorations are absent, that the generated motion
vertex shader reads both address members at the host-layout offsets, and that a
plain vertex shader does not declare either member. Full build, 47 C/C++
regression tests and Python pass; the focused test also passed after the
plain-shader assertion was added.

The final-build isolated run (`out\motion-address-final.json`) exited 0 after
134.7 s and 7,440 flips with object motion enabled, no reported guest fault and
no hang. Its 7,338-interval trace has median/P95/P99 16.662/17.877/18.359 ms,
worst 1,741.505 ms and 10.9 intervals/minute above 1.5x median; it logged 547
binary hits and 102 compiles. The final display trigger produced no image files,
so the visible scene using motion vectors is not verified. This standalone run
is not a matched A/B and does not demonstrate fewer driver creations or better
frametimes. The earlier disk-full attempt after 2,018 flips remains invalid
evidence. A controlled comparison of identical application keys and driver
cache outcomes in a visually confirmed scene is still required.

## Content identity, not permutation-index identity

Two isolated 75-second warm runs (`out\checkpoint-20261022\identity.json`) copy
the same initial QTE profile. Both exit 0 without reported guest faults.
`identity-content-analysis.json` correlates actual cached SPIR-V SHA-256 with
emulator and driver keys. For compute shader `0x804aeead`, 42 application keys
occur among binary misses in both sessions; **all 42 have different SPIR-V and
different driver keys**. A repeated numeric permutation index is not evidence
that the driver recompiled the same binary.

The runs produce 60 / 70 new permutations of that shader. Of those, 11 / 13
have byte contents already present at an earlier index. Most new variants are
not duplicates. This supports resource-specialization variation as a major
remaining target, not disabling image-type specialization. It does not resolve
the separate graphics motion-address hypothesis.

The renderer now retains the persisted SPIR-V checksum for live, deferred and
warm-loaded modules. Compute selection can use an earlier permutation's pipeline
key only with matching nonzero checksums, runtime information, start bindings,
descriptor counts **and complete image specializations**. The last condition
preserves the existing fallback-image mask used by `BindTextures`; matching
binary checksums alone would not preserve that host-side state. Shader patching
and collection bypass this reuse. No shader metadata or module is discarded,
no cache version is invalidated, and no draw/dispatch wait or fallback policy
is relaxed. Graphics keys are unchanged.

`THEORDER_COMPUTE_PIPELINE_REUSE=0` restores the previous key selection for
diagnostics. With `THEORDER_PERMUTATION_LOG=1`, first reuse selections log both
indices and the checksum; regression manifests preserve those events.
`compute-pipeline-reuse-test` goes from index 1 (red) to index 0 (green), and
rejects different/unknown checksums, image/fallback-mask, layout and runtime
differences. The selected index is cached to avoid rescanning the program on
every dispatch.

Final guarded-runtime evidence: `compute-reuse-final.json`, two isolated 80 s
title-screen runs, both exit 0 without reported guest faults. The enabled run
records `0x9846aaff` permutation 1 -> 0, checksum `59f4eb55c8623cbe`.
This proves the path executes, not a total count of avoided driver creations.
The broad `0x804aeead` variants are deliberately not merged through incompatible
fallback masks. The sampled frame results **do not establish a speedup**; see
`PERFORMANCE_BASELINE.md`. Earlier `compute-reuse.json` is development-only
evidence predating the final image-mask guard and must not be used as the result.

## Reference inspection

Reference: `HayatoG/iss_pc` commit `cde38afd3220fbdc79321eb18a57861469ec02a1`.
GitHub's repository license summary was null, but `port/LICENSE` contains GPL v2
and the inspected compiler/renderer files declare `GPL-2.0-or-later`.
This checkpoint inspected concepts; it did not import third-party implementation.
Any later incorporation still needs file-specific attribution and dependency review.

Inspected reference paths:

- `port/gpu/shadps4/video_core/renderer_vulkan/vk_precompile.cpp`:
  captured jobs, specialization/profile/driver fingerprints, bounded build queue,
  separate worker caches merged after compilation, pipeline release after warming.
  Its offline default uses available hardware threads minus one; it is not evidence
  that the same policy improves concurrent gameplay here.
- `vk_async_compiler.h`: bounded submission, priority work, terminal job failure,
  drain-before-destruction and pool reuse.
- `port/gpu/shadps4/video_core/texture_cache/readback_queue.h`: submission-order
  retirement, bounded bytes/entries, exact versus page overlap and cancellation.
- `texture_cache.cpp`: LRU resource accounting, pressure thresholds, separate
  integrated-adapter budget treatment and observable readback waits.

The Order already has corresponding async compilation, specialization metadata,
SPIR-V storage, pipeline binaries, driver cache persistence, deferred readbacks
and texture collection. Preserve their validated correctness constraints.
No wave/LDS/barrier translation changes were justified by this menu-only workload.

## Evidence and changes

- Added opt-in, buffered per-flip CSV correlating frame intervals with translation,
  SPIR-V emission, pipeline-driver work, readback/tick/recorder/host-copy waits
  and texture/buffer upload bytes.
- Whole-trace P95/P99 are now distinct from percentiles of five-second summaries.
- Cumulative binary cache hits, compilations, rejections, saves and errors are
  reported every statistics window; the report keeps the latest totals, not their sum.
- The visible-draw `WaitGraphics` path previously did not contribute to the
  synchronous compilation-wait counter. It now does; `compile_wait_ns` in the CSV
  measures that wait alongside existing shader waits instead of misreporting zero.
- Cold/warm menu experiment and paths are in `PERFORMANCE_BASELINE.md`.
- The warm sample had genuine hits but also new builds and visible-draw waits.
  Worker timing sums can exceed wall time; use correlated frame/log evidence.

## Next correctness gate

Copied-save prologue runs now expose the stall in an actual rendered scene:
`prologue.json`, `prologue-warm.json`, `nohot.json`, `extended.json` under
`out\checkpoint-20261022`. In the extended trace 90 of 93 intervals over 100 ms
also contain over 50 ms of compilation wait. Binary totals were 801 hits,
230 compiled/saved and zero reported rejections/errors. Full timing and scene
caveats are in `PERFORMANCE_BASELINE.md`.

Disabling only `THEORDER_HOT_WARMUP` against the same initial warm profile did not
eliminate new specializations (124 binary compilations versus 121 with it enabled).
This does not support removing the warmup as a fix. Permutation logging identifies
changing image types, integer formats, FMask and sRGB in material/lighting shaders;
ignoring these fields would risk incorrect SPIR-V and is not justified.

For the first warm profile, shader `0xad441fb1` has 94 cached SPIR-V files but
60 distinct byte contents; `0x8c8a4817` has 99/63 and `0x804aeead` has 203/178.
There is some redundant translation, but most variants are not byte-identical.
This is a diagnosis target, not a license to merge resource-dependent pipeline keys.
No cache invalidation, specialization or compile-worker policy changed here.

Identify why the repeatedly encountered combinations were not already warmed,
using the exact pipeline key, specialization metadata and captured scene.
Catalog only reconstructible combinations; do not guess resource-dependent keys.
Do not remove visible-pipeline waits or silently skip essential draws.

Offline region prewarming, adaptive gameplay worker tuning, driver failure
reproduction and comparisons against v0.7.5 remain unvalidated.
The previous precompilation plan's instant-restore and zero-stutter promises
are superseded by these measured limitations.

## Ahead-of-Time (AOT) Pre-Compilation & Persistent Driver Cache (2026-10-28)

To address stutter from driver-level native ISA compilation on newly encountered pipelines,
a complete Ahead-of-Time (AOT) precompilation system is implemented:

- **AOT Engine (`PrecompileAllShadersAOT`)**:
  Scans all known pipeline keys (`PipelineKey`), shader metadata (`ShaderMeta`), and SPIR-V
  modules in `Storage::DataBase`. Triggers parallel compilation of all graphics and compute
  pipelines across **all 28 threads of the Intel Xeon E5-2697 v3** (`std::thread::hardware_concurrency()`).
- **Disk Persistence (`shader_cache/pipeline_cache.bin`)**:
  Serializes the resulting `VkPipelineCache` into `shader_cache/pipeline_cache.bin` with
  `DriverCacheHeader` validation (magic `IVCK`, version, XXH3_64 hash, driver UUID).
  Loaded on next boot before rendering begins to eliminate runtime driver compilation stalls.
- **Activation Paths**:
  - CLI: `--precompile-shaders`
  - Environment variable: `THEORDER_PRECOMPILE_SHADERS=1`
  - In-game settings (`theorder.ini`): `precompile_shaders_on_boot = 1`
  - C/C++ API: `theordergpu_precompile_shaders_aot()`, `PrecompileAllShadersAOT()`
- **Console Feedback**:
  Outputs `[ShaderAOT] Compilando X/Y shaders usando 28 threads...` and
  `[ShaderAOT] Concluído com sucesso! Salvo em shader_cache/`.
