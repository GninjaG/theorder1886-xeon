# Shader cache and microstutter investigation

2026-10-22 checkpoint. No claim of stutter elimination.

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

Identify why the repeatedly encountered combinations were not already warmed,
using the exact pipeline key, specialization metadata and captured scene.
Catalog only reconstructible combinations; do not guess resource-dependent keys.
Do not remove visible-pipeline waits or silently skip essential draws.

Offline region prewarming, adaptive gameplay worker tuning, driver failure
reproduction and comparisons against v0.7.5 remain unvalidated.
The previous precompilation plan's instant-restore and zero-stutter promises
are superseded by these measured limitations.
