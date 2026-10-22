# Performance baseline

## Prologue evidence after the instrumentation checkpoint

Source: `9bd9ae7` for the first four runs; `qte.json` includes the scripted-input
fix described in `INPUT_IMPROVEMENTS.md`. Same host/game as below. Isolated profiles copy the existing
save with `--user-template user`; the original save is not used as a writable
runtime directory. The 130-second capture confirms the rendered handcuff scene.
The 300-second capture confirms the first Triangle QTE, not free movement/combat.

Local manifests under `out\checkpoint-20261022`:

| Manifest | Captured seconds | Mean FPS | P95 / P99 ms | Worst ms | Compile wait s | Binary hits / compiled |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `prologue.json` (cold port cache) | 126.465 | 36.998 | 18.066 / 313.817 | 5,300.484 | 47.331 | 163 / 472 |
| `prologue-warm.json` | 126.961 | 50.031 | 17.806 / 32.801 | 1,359.461 | 19.120 | 639 / 121 |
| `nohot.json` (same warm source, hot-list warmup disabled) | 127.907 | 47.269 | 17.892 / 97.901 | 3,067.849 | 18.513 | 594 / 124 |
| `extended.json` (300 s, later QTE) | 297.517 | 52.794 | 17.798 / 20.153 | 1,373.935 | 36.640 | 801 / 230 |
| `qte.json` (330 s, Triangle attempts, rebuilt runtime) | 326.264 | 56.519 | 17.735 / 18.305 | 991.303 | 18.425 | 1,051 / 119 |

The last run confirms the rebuilt runtime still boots/renders and receives
Triangle presses/releases plus a left-stick override. Its 310 s capture is a
dark transitional image, not evidence of free movement or successful QTE completion.
It uses the accumulated extended-run cache/save copy, so it is not an input-fix
performance A/B.

These are diagnostic runs, not an optimization before/after. All exited 0 without
reported guest faults. Inputs are scheduled from launcher start, so compilation
changes scene progress at a given wall time; do not compare the rows as equivalent
gameplay. The "cold" label refers to the isolated port cache, not the OS/driver
cache; within-run binary reuse can still produce hits.

In the cold trace, 79 of 82 intervals over 100 ms also contain over 50 ms of
synchronous compilation wait. In the extended trace it is 90 of 93. Extended
frame 12,294 lasts 1,373.935 ms, including 1,363.117 ms of compilation wait.
This is direct evidence for visible compilation stalls, not evidence that every
stutter has that cause. A whole-trace P99 near 20 ms does not exclude second-long
outliers. No visible-draw waits were removed.

The captured 130-second route is reproducible with the existing harness:

```powershell
python tools\batch_runs.py --runs 1 --secs 130 --frame-trace `
  --scene isolated-existing-prologue-save --cache-state cold `
  --profile-root out\prologue-cold --user-template user `
  --manifest out\prologue-cold.json --report-dir out\prologue-cold-report `
  --cond prologue=THEORDER_PERF_LOG=1 `
  --pad "24:cross:0.5 28:cross:0.5 32:cross:0.5 40:cross:0.5 50:cross:0.5 60:cross:0.5 70:cross:0.5 80:cross:0.5 90:cross:0.5" `
  --frame-dump-at 110 --capture-dir out\prologue-cold-captures
```

Use the resulting isolated profile as the warm template, with a different output
root. Save progression and final scene must be checked before accepting a matched
comparison. The controller remained connected; physical input was not isolated.

## 2026-10-22 checkpoint: measured title-menu workload, not gameplay

The previous version of this document contained combat FPS, memory and improvement
tables without reproducible captures. Those numbers are withdrawn. An empty-set
hash fast path does not prove that gameplay stutters have been eliminated.

Host: Windows 11, Intel Family 6 Model 183, NVIDIA RTX 5060 Laptop GPU,
driver 621.1.0.0, Vulkan 1.4.357. Game: CUSA00035 01.02.
Source: consolidated `1fb2c3e` plus the instrumentation changes recorded in Git.
No AMD or Intel GPU compatibility is established by these runs.

Evidence is local, under `out\checkpoint-20261022` (not committed game data):

| Run / evidence | Scope | Result |
| --- | --- | --- |
| `manifest.json` | Initial native Windows launch | 0 flips, exit `0xC000007B`, not a GPU benchmark |
| `clang-manifest.json` | 30 s, CLANG64 environment | Exit 0, 1,440 logged flips |
| `metrics.json` | 65 s, isolated cold profile | Exit 0, 3,720 logged flips, 13 process samples |
| `captures\metrics-01-20261022T142847345849Z` | Final display capture | Title menu confirmed; scripted inputs did NOT establish gameplay |
| `ab.json` | 40 s cold / warm isolated port caches | Both exit 0; whole-trace statistics below |
| `native.json` | 90 s from normal PowerShell after DLL-path fix | Exit 0, 5,220 logged flips, 5,016 raw intervals; capture still frontend, not validated gameplay |
| `wait.json` | Final 25 s instrumentation check | Exit 0, 1,114 intervals; 400.853 ms measured compilation waits, maximum 146.128 ms in one interval |

The harness reports flips from periodic log messages; the CSV contains individual
observed intervals. Capture starts after the first flip and is flushed every five
seconds; abrupt process termination can omit the final unflushed tail.

| Whole-trace metric | Cold | Warm |
| --- | ---: | ---: |
| Captured intervals | 2,018 | 1,933 |
| Captured duration (s) | 35.073 | 36.298 |
| Mean FPS (frames / observed time) | 57.537 | 53.254 |
| Median frame (ms) | 16.658 | 16.679 |
| Frame P95 (ms) | 17.803 | 17.867 |
| Frame P99 (ms) | 31.827 | 32.384 |
| Worst frame (ms) | 369.008 | 1,880.082 |
| Stutters/min (>1.5x whole-trace median) | 42.768 | 51.243 |

These are single startup/menu samples, NOT a controlled optimization A/B or a
regression verdict. Warm used a copy of the earlier menu profile; cold copied
persistent data without `cache` or `shader`. OS/driver caches were not controlled.
Warm reported 267 shader metas and 155 binary hits at startup, but still compiled
new pipelines later.

The warm CSV's frame 452 took 1,880.082 ms with 1,377.143 ms of aggregate
pipeline-driver work, 2.667 ms recorder wait and 9.060 ms host-copy wait.
Driver work can run concurrently across workers: do not interpret its sum as
exclusive CPU time. Logs also record key-scoped visible-pipeline waits.
This identifies a compilation-related investigation target, not the complete cause.
Visible draws must not be dropped to improve this metric.

The final native Windows run confirms the startup-path fix without manually
preparing CLANG64: 58.934 mean FPS, 17.822/18.522 ms raw P95/P99, 401.409 ms worst
frame, 23.263 stutters/min over 85.112 captured seconds. Binary totals: 9 hits,
199 compiled, 199 saved, no reported rejection/error. A `--level` argument and
scripted Cross presses did not prove gameplay; the final capture shows the
frontend's save-information screen. Do not compare this longer startup interval
against the shorter samples as an optimization gain.

## Reproduce

From a normal PowerShell, after `build.sh` has produced the development executable:

```powershell
python tools\batch_runs.py --runs 1 --secs 65 --frame-trace --scene title-menu `
  --cache-state cold --profile-root out\baseline-cold `
  --manifest out\baseline-cold.json --report-dir out\baseline-cold-report `
  --cond measured=THEORDER_PERF_LOG=1
```

For warm runs, use a new profile root, `--cache-state warm`, and `--user-template`
pointing to a previously collected isolated profile. Never clear the user's caches
or modify the user's saves to manufacture a cold run.

Raw CSV: `THEORDER_FRAME_CSV`, set per run by `--frame-trace`.
Process sampler: `THEORDER_PERF_LOG=1`, twice-per-second sampling, five-second logs.
CSV wait/work counters are overlapping instrumentation, not a CPU/GPU frame-time split.
GPU timestamp profiling remains the existing `THEORDER_GPU_PROFILE=1` path.
Arbitrary mutex timing, asset I/O, per-thread CPU, process WDDM VRAM and full GPU
frame timing are not yet integrated into this report.

## Version comparison and remaining gates

- v0.7.5: `windows-v0.7.5`, `4c76cd3`.
- v0.7.6 candidate: `windows-v0.7.6-rc.1`, `f1f0c8a`; no final v0.7.6 tag was found.
- Previous remote master: `10e44b4`; consolidated master: `1fb2c3e`.
- No equivalent gameplay benchmark across those builds, bisect verdict, combat,
  explosions, transitions or 30-60 minute gameplay soak has been obtained here.
- Establish a verified repeatable gameplay checkpoint and input replay before
  attributing the user's v0.7.6 gameplay regression or claiming it fixed.
