# Performance baseline

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
