# Stability-first execution plan (2026-10-27)

Status: investigation and triage. This document does not claim a gameplay defect is fixed.

## Scope and evidence

Main repository: mateussmeneses/theorder1886-port. Reference architecture:
FelipePrado0/SpartanPort is a source of tested patterns, not a renderer to merge wholesale.

Prioritize black/white output in issues #43, #47 and #24, graphics/stalls in #63, #39,
#29 and #30, and Chapter 9 crash #11. Several affected reporters find
windows-v0.6.1 better than 0.7.x, but it is not universally free of glitches.

Reuse tools/batch_runs.py, tools/regression_report.py, tools/fault_stacks.py and
tests/test_regression_harness.py before creating another benchmark tool.
Follow docs/PERFORMANCE_BASELINE.md for existing evidence and limitations.

## Work packages

| ID | Deliverable | Acceptance gate |
| --- | --- | --- |
| Q1 | Better issue forms, contributor rules, auto-labeling | Pure unit tests, PR review and a real issue event after merge |
| R1 | Regression search from v0.6.1 through v0.7.9 (#43/#47) | Same SKU, checkpoint, GPU/driver, matched output and log; reporter retest |
| R2 | GPU visual correctness (#29/#30/#39/#63) | Draw/resource IDs, before/after captures, no skipped visible draws |
| P1 | Compilation, scheduler and frametime (#63/#68/#40) | Controlled cold/warm A/B, P95/P99/P99.9 and worst-frame comparison |
| L1 | Launcher install/update/config reliability | Clean-install, portable settings, rollback and packaged smoke tests |
| C1 | Dead-code and historical-doc cleanup | Proven unused, full test/build gate preserved, provenance retained |
| M1 | README and Ko-fi transparency | Accurate status, verifiable clips, explicit unsupported/experimental features |

Keep R1, P1 and L1 on separate branches/PRs. Do not merge them with marketing work.

## Repeatable performance experiment

1. Keep originals untouched: saves, caches, game dump, runtime files. Create isolated
   user/profile/cache copies for each test.
2. Record exact release/commit, executable SHA-256, CUSA/game update, eboot.bin SHA-256,
   OS, CPU, GPU, driver, Vulkan version, active upscaler, output and internal resolution,
   frame cap, patches and MSAA.
3. Record chapter/checkpoint and a deterministic input/camera route.
4. Separate first encounter (cold/first visit) and second visit (warm) results.
   Never clear all user caches as a normal upgrade procedure.
5. Interleave conditions with at least five repetitions per condition as an initial
   benchmark design. Confirm available CLI flags with batch_runs.py --help.
6. Analyze raw frame time P95/P99/P99.9, worst interval, count of frames above
   50 and 100 ms, queue wait, driver compile wait, GPU/present wait and skipped draws.
   Do not add concurrent worker spans as exclusive elapsed CPU time.
7. Obtain graphics captures in separate runs: GPU debugger overhead skews timing.
8. Assert no regressions in the same visual scene. Better frame times are not a fix
   when render targets or geometry are silently omitted.

## Regression archaeology

For #43 compare windows-v0.6.1, windows-v0.7.5, windows-v0.7.7 and windows-v0.7.9.
Start with the RTX 3060 Ti CUSA00035 01.02 report. Use a repeatable predicate before
git bisect; if the symptom is not deterministic, test small commit groups instead.
Audit the presentation chain, shader paths, descriptors and render-target lifecycle.

For #63 use separate Chapter 8 and Chapter 9 checkpoints. The reporter observed
different behavior between those chapters on v0.6.1. Treat shader variants,
cache correctness, barriers and image aliasing as hypotheses pending matched
capture, not proven causes.

## Pull request evidence checklist

- Which symptom, issue, exact good/bad tag and commit?
- What changed, and why would it cause the symptom?
- What regression test failed before and passed after?
- What build/test commands ran and which were skipped?
- What matching game/GPU/checkpoint retest remains unverified?

Do not mark an issue fixed from unrelated hardware smoke tests.
Do not commit proprietary assets, private logs or dumped game files.
Maintain upstream credits and licenses when adapting source.

## First code-archeology checkpoint: v0.6.1 to v0.7.1 (2026-10-27)

GitHub tag comparison shows **four commits / 36 changed files** from
windows-v0.6.1 to windows-v0.7.0, and **one commit / 13 changed files** from
windows-v0.7.0 to windows-v0.7.1. This is a search window, not a proven bad commit.

| Commit | Candidate behavior to isolate | Mechanism to measure |
| --- | --- | --- |
| b333700d79 | Shader resource handling, barriers and DrawPipe changes | Presence of expected bound resources, correct barriers, render output |
| 7b67a84fc2 | New graphics/launcher options and anti-flicker toggles | Effective launch environment and renderer flags |
| 2e2b9e397c | Render target clear handling | Correct clear/load and contents of final color targets |
| 8bdf10d8fc | Buffer-cache memoization and black-screen changes | Buffer data version/dirty state vs GPU writes |
| 276495a6ba | Additional resource/format changes in v0.7.1 | Changed image format/depth/tiling interpretation |

High-signal source deltas requiring **reversible, isolated A/B**:

- In gpu/shadps4/video_core/renderer_vulkan/vk_rasterizer.cpp,
  ImplausibleTexture's MaxTextureGuestSize changed from **512 MiB to 256 MiB**
  in v0.7.0. Count rejected descriptors per stage/draw; do not raise the bound
  globally without proving that rejected descriptors refer to valid game data.
- The same file added a hard-coded list of stage.pgm_hash values whose
  textures are bound as null. Verify a real T# descriptor and its guest mapping
  before removing any skip or allowing a previously rejected binding.
- In gpu/shadps4/video_core/texture_cache/image_view.cpp, undefined/unknown
  image formats now fall back to R32G32B32A32Sfloat. Capture format pairs,
  actual image bytes and the consuming shader. A fallback is not automatically
  layout-compatible with the original guest format.
- In gpu/shadps4/video_core/renderer_vulkan/vk_rasterizer.cpp, the
  0x9846aaff compute shader path forces special VSM coverage values. Compare
  resulting shadow tiles against the reference pass without disabling all
  shadow work.
- In the v0.7.0 -> v0.7.1 diff, run.py changed the default
  THEORDER_READBACK_LINEAR value from 1 to 0. Record whether an override is
  active and test both modes with identical resources/caches.
- In the same v0.7.1 diff, AmdGpu::Image::NumLayers, new 3D tile modes, and
  a larger volumetric-clear heuristic changed. Compare decoded descriptors
  and image dimensions before making any source-level fix.

Suggested observation fields: port SHA/tag; game SKU/update/eboot SHA-256;
shader hash; draw/dispatch ID; descriptor address, size and format; reason for
rejection; target image layout; produced color/depth hashes; first and warm
visit; driver/GPU. Do not write raw copyrighted game data to public logs.

A 0.7.x black-screen symptom accompanied by audio and readable UI is consistent
with missing/incorrect scene composition, but does not identify any one
shader, driver, clear, cache or texture bug. Require matching capture and a
positive/negative A/B on the affected hardware before editing the default
render path.
