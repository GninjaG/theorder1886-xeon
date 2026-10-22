# Upscaling, resolution and frame generation

2026-10-22 checkpoint: preserve existing renderer; no unvalidated provider toggle added.

The consolidated history includes #59 (native output settings) and #61 (explicit
DLSS retry), plus their launcher/settings/support regression tests.
FSR/DLSS selection, capability checks, temporal input handling and output targets
already exist. Their presence in code or a passing settings test is NOT proof
of successful GPU execution, HUD correctness or image quality in gameplay.

| Feature / hardware | This checkpoint |
| --- | --- |
| NVIDIA RTX 5060 Laptop, normal title-menu presentation | Observed and captured |
| DLSS / FSR quality modes, motion vectors, exposure, ghosting | NOT TESTED end to end |
| Native output 720p/1440p/4K/ultrawide | Existing support preserved; NOT TESTED visually here |
| AMD / Intel rendering and temporal providers | NOT TESTED |
| Frame Generation | Not implemented or advertised as active |

Resolution meanings must stay separate: guest scene/internal targets, temporal
input, temporal output/UI, window and swapchain extent. Do not infer internal
resolution from window size. Current settings do not enumerate 1600x900; adding
it requires keeping launcher, settings, targets and tests consistent.

Frame Generation is a separate presentation integration, not an upscaler enum.
Prerequisites still include verified motion/depth inputs, HUD separation,
provider/SDK license and device checks, presentation pacing, latency instrumentation,
resize/recreation and actual generated-frame validation.
The existing FSR dependency's frame-interpolation sources do not establish an
active Frame Generation path in this renderer.

New raw frame statistics count guest flips only. They are not generated/displayed
FPS, and no frame-generation performance improvement is claimed.
