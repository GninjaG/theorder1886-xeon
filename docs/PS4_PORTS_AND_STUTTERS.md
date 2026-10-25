# PS4 Port Performance and Stutter Research

Research checked 2026-10-25. Social posts, per-game compatibility reports, and a per-game native port are different evidence classes; none is a substitute for a matched benchmark of this port.

## Findings in This Port

The user's test found a significant gameplay improvement with `THEORDER_VISIBLE_PIPELINE_PRIORITY=1`. It now defaults on; `THEORDER_VISIBLE_PIPELINE_PRIORITY=0` restores the old behavior. The change moves an awaited visible graphics pipeline ahead of jobs still queued. It cannot preempt an active driver compile or reduce the driver's compilation time.

The user reports that entering a previously unseen game area still produces substantial stutters until the area stabilizes, although overall pacing is much better. This is qualitative, not a matched capture, but it fits new pipeline misses: the driver still has to create each unseen key. The existing hot list records up to 2,000 recently used graphics/compute keys and warms them in the background on the next launch. A revisit after restarting can check whether hot-list and driver-cache warm-up reduce those hitches.

The local `dist/theorder1886-windows/user/last_run.log` snapshot still contains worst-frame records of 905 ms, 1,052 ms and 1,138 ms. The associated `Stall` records report roughly 1.0-1.2 s aggregate driver work; GPU-tick waits are 3-9 ms and host-copy waits are below 1 ms. Frame-stat windows report 1.6-2.6 s synchronous wait and 0.6-2.9 s aggregate worker time. Those worker times overlap and are not exclusive wall time. The log does not preserve the priority environment setting, so it is not a controlled A/B; it does show that substantial hitches remain.

The same log reports 0.3-1.5 ms accumulated compiler queue wait in sampled windows. That is far smaller than synchronous wait and driver work in those windows. The next target is therefore the duration and cacheability of individual driver pipeline builds, not simply raising queue priority or adding compiler threads.

Earlier isolated 130-second cold-route evidence recorded a 1,324.6 ms worst frame; 45 of 50 frames over 100 ms had over 50 ms compilation wait. The packaged-default smoke run later completed 47.9 seconds at 60 FPS median with a 333 ms worst frame, but used a cold private profile and was not a matched gameplay benchmark.

## Recent PS4 Compatibility Reports

- The official [shadPS4 v0.20.0 release](https://github.com/shadps4-emu/shadPS4/releases/tag/v.0.20.0), published about two weeks before this research, lists renderer and texture-cache optimization work. The release also includes presentation-wait and fragment-texture changes. These are upstream changes, not proven fixes for this fork's measured driver-compile stalls.
- The compatibility project's [status definitions and rules](https://github.com/shadps4-compatibility/shadps4-game-compatibility) distinguish `Playable` from `Ingame`; the latter can still include serious graphics, performance, audio, or crash issues. Reports are tied to game version, emulator release, OS, and hardware.
- [Journey, CUSA00470](https://github.com/shadps4-compatibility/shadps4-game-compatibility/issues/3087) is marked playable on Linux with shadPS4 0.19.0, an i9-12900H and RTX 3070 Ti laptop; the reporter says it had no issues. [7 Days to Die, CUSA03455](https://github.com/shadps4-compatibility/shadps4-game-compatibility/issues/3103) is marked playable on Windows with a Ryzen 7 5800X and RTX 5070 after a reported 2m56s test. These are bounded reports, not claims across all configurations.
- [Dead Island 2, CUSA27043](https://github.com/shadps4-compatibility/shadps4-game-compatibility/issues/3123) is marked `Ingame` on Linux/RTX 3070 Ti laptop; its report mentions texture and performance problems and missing CMask. Reaching gameplay is not the same as stutter-free or visually complete.
- The [official shadPS4 X post for Ace Combat 7](https://x.com/shadps4/status/2106320004723401108), dated 2026-10-03, calls v0.19.1 WIP “works perfectly” and says it should be playable on 0.19.0. It is a demonstration, not a controlled frame-time report.
- The [2026-10-25 inFAMOUS First Light post](https://x.com/McGroin83/status/2114245485749764395) links [VantageEX's native PC port](https://github.com/VantageEX/infamous-first-light-pc), based on `iss_pc` and shadPS4-derived Vulkan code. Its own README describes an early playable build, first missions, about 35-45 FPS on Ryzen plus RTX 4070 SUPER, 2-3 minutes of first-start shader preparation, and GPU readbacks that still cost FPS. It also lists pulsing lights in one cutscene. This is a useful architectural comparison and evidence that recent ports still report startup compilation/readback costs, not a claim that those issues are solved.

Reddit search pages were indexed, but direct post retrieval returned verification/HTTP 403 or image-only content. No Reddit claim is used as evidence here.

## Next Experiments

1. Keep queue priority enabled by default and `=0` as rollback. Compare the user's same scene with the exact build, route, and cache state recorded; the previous scripted cinematic A/B is not equivalent to their gameplay test.
2. Revisit the new area after restarting and compare it with the first visit. Keep build, route, settings and profile state recorded; check startup logs for `Pipeline warm-up` and driver-binary hits.
3. Add per-pipeline queue-residence and driver-build timings keyed by the exact pipeline key. Existing totals mix jobs and overlap, obscuring whether the remaining delay is CPU scheduling or the driver call.
4. If driver compilation dominates, capture the repeated exact keys and prewarm those variants before their scene. Keep resource specializations distinct. Then repeat the same route cold and warm and verify driver-cache hits.
5. Try the existing full-cache startup warm-up on a private profile: set `THEORDER_PIPELINE_BINARIES=0`, `THEORDER_BACKGROUND_WARMUP=0` and `THEORDER_PARALLEL_WARMUP=1`. That disables the on-demand-binary startup path and builds stored pipeline entries behind a loading screen, with up to six workers. It trades longer startup for fewer in-game first-use compiles; it cannot prewarm never-seen variants. Compare the same route and report startup duration as well as raw P99 and worst frame.
6. Only test extra/reserved compiler workers if per-key data shows queue residence is material. More concurrent Vulkan pipeline creation can increase CPU and driver contention; the current aggregate queue-wait samples do not justify it.
7. Upstream PRs #5112 and #5219 focus largely on sparse memory/fetch parsing/barrier tracking and texture-cache CPU overhead. They may improve other workloads, but are not the first experiment for these pipeline-driver-dominated hitches.

## REA Coverage

REA opened the port's Windows PE earlier, but no deep native provider was configured. REA rejected the game's `eboot.bin` as an unsupported binary format, so there is no game-binary decompilation evidence in this investigation. The stutter attribution above comes from the checked-in C++ renderer and local frame/performance logs, not reverse-engineered instructions.
