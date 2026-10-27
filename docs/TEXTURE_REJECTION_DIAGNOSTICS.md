# Opt-in texture rejection diagnostics

**Status:** Instrumentation only, not a graphics fix. Does not change any binding
decision or disable rendering work. Initially covers the null-image paths in
Vulkan::Rasterizer::BindTextures; it does not cover every descriptor rejection
from the shader recompiler, binding memo or driver.

## Enable on the affected machine

PowerShell (before launching the port from that same terminal):

```powershell
$env:THEORDER_DIAG_TEXTURE_REJECTS = '1'
$env:THEORDER_GAME_DIR = 'D:\path\CUSA00035'
python run.py
```

Or use the corresponding launcher environment when it forwards these variables.
Run *one repeatable scene*, export the launcher bug report / redacted
`user/last_run.log`, and remove the variable after testing. For a particular
shader hash, optionally set `THEORDER_DIAG_TEXTURE_SHADER=0x12345678`.
The hash is from that run's trace, **not** a universal game-build offset.

The new warning line is:

```
T# null binding: reason=<category> count=<number> shader=0x... slot=<n> address=0x...
```

It includes dimensions, layers, data/number format and guest image byte size
when known. Guest address is a virtual GPU address, not a host user path.
Each category logs its first eight hits, then powers of two (16, 32, 64...).
The reported occurrence counter is **across all shaders**, except when filtering
to one shader. A missing rejection warning does not prove a render path worked.

Reasons include low-address, invalid-data-format, fmask-special,
invalid-array-range, invalid-mip-range, excessive-layers,
compute-fallback-mismatch, unmapped-address, unsupported-vulkan-format,
excessive-dimensions, excessive-texels, excessive-layer-dimensions and
implausible-guest-image.

## What to compare

1. Record exact port release/commit, CUSA/update, eboot SHA-256 (private),
   GPU/driver, output resolution, active upscaler and checkpoint.
2. Run the same scene on the affected GPU using isolated copies of
   user/settings/cache. Do not erase original saves or caches.
3. Compare 0.6.1 and 0.7.9 only with equivalent game/configuration/scene.
4. Pair the log with the final scene capture (same camera) and, when relevant,
   FrameCapture draw/resource IDs. A rejected T# may be intentionally unused,
   especially FMask and fallback-permutation descriptors.
5. If one reason differs, run a narrow guarded test before changing the
   actual descriptor validation. The diagnostic itself does not establish causality.
6. Disable diagnostics before frametime benchmarking: opt-in logging can create
   CPU and I/O overhead. Benchmarks require separate runs and warm/cold baselines.

The initial investigation focuses on #43 and #47 (scene black/white with
audio/UI) and contrasts the affected NVIDIA/AMD GPUs against their last
reported working releases. No reporter retest has been completed here.

Test the sampler in isolation with:

```bash
clang++ -std=c++20 -O2 -Wall -Wextra -Werror -pthread -Igpu/shim \
  tests/test_texture_reject_diag.cpp -o texture-reject-diag-test
./texture-reject-diag-test
```

The full MSYS2 test runner also builds/runs the same standalone test; the
small GitHub CI job checks this header and sampler, **not** the full renderer
compilation or game rendering.

## Aggregating sampled rejects in regression reports

The existing `tools/regression_report.py` parser recognizes `T# null binding`
warnings and includes categories in the manifest and Markdown from
`tools/batch_runs.py`. The analyzer reports per reason the number of logged
lines, the **lower bound** derived from the largest observed sample count, and
up to eight distinct shader/slot examples.

**Never sum sampled log lines as if every rejection were logged.**
The rate limiter emits the first eight occurrences and then powers of two.
The count is per reason across all shaders, **not per shader**. An empty
section can mean disabled diagnostics, truncated logs or no detected rejection;
it cannot prove graphics correctness.
## A/B test: v0.6.1 texture guest-size cap (experimental)

Code archaeology confirmed a **single cap change** between
`windows-v0.6.1` (512 MiB) and `windows-v0.7.0` (256 MiB) in
`Rasterizer::ImplausibleTexture`. It is a candidate for the black/white
scene reports #43/#47/#24, **not a proven root cause**.

The default stays **256 MiB**. To restore **only this one legacy cap**
for one run of a test build:

```powershell
$env:THEORDER_TEXTURE_MAX_MIB = '512'
$env:THEORDER_DIAG_TEXTURE_REJECTS = '1'
python run.py
```

Use `256` or unset the variable for the baseline. Only the exact value
`512` is accepted; other values leave the default at 256. The diagnostic
prints `Texture-size A/B: LEGACY 512 MiB cap active` when used. The rule
rejecting extremely excessive padding remains enabled in both modes;
unmapped, malformed and unsupported descriptors remain rejected. This
experiment can permit larger texture allocations than the default,
so monitor RAM/VRAM and stop if memory pressure or hangs occur. **Do not
benchmark FPS with diagnostic logs enabled.**

To isolate the single-variable change, run the **same build** and
checkpoint with the existing batch runner, using one condition with
`THEORDER_TEXTURE_MAX_MIB=256` and another with `=512` via
`--cond`, isolated `--profile-root` and `--data-root`, the same
game/eboot/update/GPU/driver/settings/camera and repeat count.
Record whether the resulting `T# null binding` events with
`reason=implausible-guest-image` change and capture final output.
An unchanged log is not proof no textures are missing: the renderer has
other guards and may use memoized bindings.

**Do not recommend 512 as a default unless a matched image capture
demonstrates recovery without new corruption or memory issues.**
