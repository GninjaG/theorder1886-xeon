# Windows v0.7.6-rc.1: Investigation Build

**Pre-release for testing only. Not a replacement for stable `windows-v0.7.5`.**

## Base and Scope

This build unifies the two release lines onto a single branch (`master`): the v0.7.4/v0.7.5
feature line (120 FPS, FSR, launcher reset, sun shadows #26, 32:9 #27, PT-BR fallback #28,
Hi-Z, RDNA4 format fallback #33, `-usedlc` #16) merged together with the flicker root-cause
investigation (FEAT-001..005 diagnostic IDs and targeted depth/render-pass sync fixes) and the
regression harness. Every v0.7.5 feature is preserved; the RCA work is added on top. The merge
kept the v0.7.5 multiwave LDS-barrier default and exposes `THEORDER_LDS_BARRIERS_MULTIWAVE` as an
explicit diagnostic override (=0 off, =1 on, unset = v0.7.5 default). AMD wave32 visual validation
is still pending.

- Stable image/buffer/frame/draw/dispatch diagnostic IDs from the RCA work.
- Targeted render-pass and depth-store synchronization corrections, with regression tests.
- Pending visible graphics-pipeline draws wait on their own compile instead of silently
  disappearing; this may trade popping for compilation stalls.
- Regression harness: isolated profiles/configs/data, cache provenance, runtime identity,
  captures, temporal diffs, memory telemetry, Markdown/HTML reports and fault-stack JSON.
- Report fixes for runs without captures, peak aggregation, ambiguous references and empty masks.
- Explicit RGBA/BGRA diagnostic dumps and compatible legacy conversion.

## What Is Not Proven

No gameplay before/after comparison confirms that #29, #30 or #39 is resolved.
AMD/Chapter-5 behavior and the signed-format fallback require device-specific runtime
evidence. The documented format-remap path is active on the local NVIDIA device too;
its visual correctness must not be inferred from compilation or logic tests.

The earlier O10 HTILE A/B collected boot/prologue footage on the older build, NOT the
reported rooftop scene. Those results are not validation of this RC. No PS4/shadPS4
golden was approved. No FPS improvement, full-campaign stability or issue closure is claimed.

## Testing Safely

Extract into a new folder, leave the stable installation intact and use COPIES of saves
and settings. Do not point this RC at your original user profile: the existing launcher
upgrade routine can clear the selected profile's shader/pipeline cache when versions differ.
No cache deletion is proposed here as a flicker fix.

For reproduction, record game title/version and eboot hash, GPU/driver, checkpoint, exact
inputs, settings, cache state and stable resource/draw IDs. Use Vulkan synchronization
validation and capture equivalent frames before comparing builds. Do not mix the cache
or autosaves of two conditions. Warm harness runs clone the template cache for every run;
allow enough disk space for these copies. The source checkout contains the Python harness.

## Validation Record

Build, test, package and smoke-check results are appended only after execution. Existing
RCA evidence and its limitations remain in `docs/PROGRESS.md`. This release includes no
game files, saves, user configs or shader caches.