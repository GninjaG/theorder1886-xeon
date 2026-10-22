# Process memory profiling

2026-10-22: corrected the overlay's measurement semantics, not a claimed RAM reduction.

| Display / source | Scope |
| --- | --- |
| RAM private / `GetProcessMemoryInfo`, `PROCESS_MEMORY_COUNTERS_EX::PrivateUsage` | Process private commit |
| Working set / `WorkingSetSize` | Resident process pages, including shareable pages |
| Peak working set / `PeakWorkingSetSize` | Process lifetime peak, not peak private commit |
| CPU / `GetProcessTimes` divided by monotonic elapsed time and logical CPUs | Process, normalized to machine capacity |
| GPU percentage / PID-filtered PDH 3D-engine counters | Available process engines only; unavailable is `n/a` |
| Vulkan allocations / VMA heap statistics `allocationBytes` | Renderer-managed allocations, device and host heaps; not physical VRAM or driver overhead |
| GPU adapter / `VK_EXT_memory_budget` heap usage and budget | Adapter totals, explicitly NOT this game's VRAM |

Binary units are labeled GiB. RAM no longer divides Working Set by installed
system RAM, and no adapter total is labeled as process VRAM.
Advanced metrics are available in the Overlay tab and persist as `overlay_advanced`.
Unavailable process VRAM stays `n/a`. Reserved virtual address space, private-commit
peak, shared GPU memory and per-thread CPU still require separate measurement.
Do not add Vulkan, private commit, working set and adapter usage together.

Sampling occurs on the existing background sampler every 500 ms. Set
`THEORDER_PERF_LOG=1` to start it independently of overlay visibility and log every
five seconds. `tools\batch_runs.py` records these samples and their observed maxima.
Negative log values mean unavailable and become JSON `null`, not zero usage.
The Linux `/proc/self/statm` virtual-size field is no longer mislabeled as private commit;
Linux private commit remains unavailable through this helper.

## Measurements (title menu only)

`out\checkpoint-20261022\metrics.json`: 65-second isolated cold-profile run,
13 samples, final display captured and confirmed as the title menu.

| Observed maximum | MiB |
| --- | ---: |
| Process private commit | 4,679.61 |
| Process working set | 3,230.66 |
| Renderer VMA allocations | 1,708.01 |
| Adapter-wide usage | 2,138.58 |

These are different quantities, not components of a single sum. They neither
explain nor disprove the user's approximately 11 GB gameplay observation.

The subsequent 300-second isolated prologue run (`extended.json`) reached the
first Triangle QTE. Observed maxima: private commit 8,045.28 MiB, Working Set
5,822.41 MiB, renderer VMA allocations 2,444.96 MiB, adapter-wide usage
3,091.71 MiB. These are sampled maxima, not simultaneous components or proof of
a leak. It is a different workload from the menu above, not a before/after
memory reduction. Process-dedicated VRAM remains unavailable.

The subsequent rebuilt-runtime 330-second run (`qte.json`) peaks at
8,656.93 MiB private commit and 6,414.86 MiB Working Set. It also carries more
accumulated cache data and attempts additional QTE input; the difference is not
an isolated allocation regression or evidence of a leak.

Before/after gameplay allocation profiles, leak classification and a 30-60 minute
soak remain NOT TESTED. No guest allocation limits or texture eviction policies
were changed without that evidence.

Regression coverage: `perf-overlay-test` checks labels, missing values, advanced
formatting and persistence. It reserves 256 MiB with `MEM_RESERVE` and verifies
that reservation is not counted as equivalent private commit. Python report tests
distinguish process samples from adapter totals and preserve unavailable values.
