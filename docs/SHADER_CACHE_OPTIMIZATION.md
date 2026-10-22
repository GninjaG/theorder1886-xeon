# Shader cache and microstutter investigation

2026-10-22 checkpoint. No claim of stutter elimination.

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
