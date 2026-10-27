# Upscaling, resolution and frame generation

Updated 2026-10-26. Preserve the existing renderer; do not infer image quality or performance from a setting alone.

The consolidated history includes #59 (native output settings) and #61 (explicit
DLSS retry), plus their launcher/settings/support regression tests.
FSR/DLSS selection, capability checks, temporal input handling and output targets
already exist. Their presence in code or a passing settings test is NOT proof
of successful GPU execution, HUD correctness or image quality in gameplay.

| Feature / hardware | This checkpoint |
| --- | --- |
| NVIDIA RTX 5060 Laptop, normal title-menu presentation | Observed and captured |
| DLSS / FSR quality modes, motion vectors, exposure, ghosting | Gameplay quality NOT TESTED end to end; see 900p checks below |
| Native output 720p/1440p/4K/ultrawide | Existing support preserved; NOT TESTED visually here |
| AMD / Intel rendering and temporal providers | NOT TESTED |
| Frame Generation | Not implemented or advertised as active |

Resolution meanings must stay separate: guest scene/internal targets, temporal
input, temporal output/UI, window and swapchain extent. Do not infer internal
resolution from window size.

The output selector sets the host render/presentation target, not the monitor's
desktop mode. At launch it also supplies the requested window size; changing it
in-game does not change the desktop mode and does not guarantee that the window
is resized. The upscaler preset determines internal scene raster size when a
temporal upscaler is active; Native AA (1.0x) does not reduce that size. With
upscaling off, the scene target follows the selected output target. The in-game
Image tab reports render resolution, target output, window size and active
upscaler separately. Unsupported providers visibly fall back (for example FSR
4 to FSR 3.1 when device/assets are unsupported); check the active provider
before judging a selection as placebo.

Issue #35 reports that the internal resolution and FSR 4.1.1 looked unchanged
beside an external PS5 4K patch comparison. This is a real visual report, not
yet proof of a no-op: the post contains no exact game SKU/PC GPU/build identity,
and the selected output, preset, and active fallback must be read separately.
The host output selector is not the external guest-resolution patch and does
not claim a 4K guest framebuffer. A matched render/output capture and an
Image-tab report are needed before claiming internal 4K or an upscale-quality
difference.

The Windows launcher and overlay now list all eight output sizes supported by
the C++ runtime, including 3440x1440, 3840x1080 and 5120x1440. Existing numeric
indices remain stable; new modes were appended. Synthetic tests cover 3440x1440
and 3840x1080 settings round-trips and selector mapping. This proves the chosen
target reaches the runtime, not correct framing, HUD or image quality on an
ultrawide monitor. Issue #27 remains open for the reporter's gameplay retest.

Warm-up controls are wired to both relevant paths: background warm-up controls
the on-demand recent-session list and full-cache background loading; the
parallel option selects one or multiple background workers. Unit tests cover
the mapping and worker policy. No performance gain is claimed without a matched
first-visit/warm-cache gameplay comparison.

## 1600x900 output checkpoint

Added 900p to the launcher's output list, in-game selector and renderer dimensions.
Existing numeric output indices remain unchanged; 900p is appended as index 5
(diagnostic `THEORDER_PRESET_FILE` uses those indices). INI persistence uses
`output_res=1600x900`, shared by launcher and renderer.

On the RTX 5060 Laptop, isolated title-menu runs capture actual 1600x900
post-process output images (5,760,000 bytes of BGRA), not just selected settings.
The guest display buffer remains 1920x1080; the temporal/UI host path and final
presentation image must not be confused with that guest buffer.

Local manifests under `out\checkpoint-20261022`:

| Evidence | Actual observation |
| --- | --- |
| `900p.json`, 75 s | 1600x900 output capture; FSR 3.1 selected, Quality scene raster 1066x600, half targets 533x300; original UI coordinates 1920x1080 |
| `900p-providers.json`, Native 65 s | 1600x900 output capture; scene raster 1600x900, half targets 800x450; FSR logged off |
| Same manifest, DLSS requested 65 s | Explicit missing `port_dlss.dll` / `nvngx_dlss.dll` warning, fallback to FSR 3.1, 1600x900 output capture; NOT a DLSS execution |
| `900p-live.json`, 85 s | Live 900p -> 1080p -> 900p selection changes scene raster 1066x600 -> 1280x720 -> 1066x600, with no restart or reported guest fault |

All four runs exited 0. The live test's final post-process capture stays at the
900p window extent while the configured temporal/scene output changes: it is not
evidence that the swapchain/window automatically resizes with the selector.
These frontend captures do not validate temporal dispatch quality, HUD/subtitles
in combat, ghosting, or motion/depth correctness. No new provider capability is
claimed. `packaging\windows\build_dlss.sh` requires an NVIDIA SDK checkout and
MSVC to produce the absent optional bridge; the missing bridge was not fabricated.

Validation: renderer config test rejected 900p before the change and now preserves
it through load/save. Launcher tests cover choice and persistence without changing
the saved provider/preset. Full build and 45 C/C++ suites plus Python pass.

Frame Generation is a separate presentation integration, not an upscaler enum.
Prerequisites still include verified motion/depth inputs, HUD separation,
provider/SDK license and device checks, presentation pacing, latency instrumentation,
resize/recreation and actual generated-frame validation.
The existing FSR dependency's frame-interpolation sources do not establish an
active Frame Generation path in this renderer.

New raw frame statistics count guest flips only. They are not generated/displayed
FPS, and no frame-generation performance improvement is claimed.
