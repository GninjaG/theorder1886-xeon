# The Order: 1886 PC Port - Windows v0.7.7

This release consolidates the changes since `windows-v0.7.6-rc.1`, fixes input
after closing the port settings menu, and corrects launcher update handling.
Lighting, missing shadows and AMD colored artifacts remain under investigation:
this is a retest build, not a claim that those reports are resolved.

## Fixes and improvements

- **Controller after R3+L2 / Insert:** controls held in the overlay are suppressed
  independently until released. A slightly off-center stick no longer blocks
  the entire gamepad. Held analog triggers are also neutralized. A virtual SDL
  controller regression covers drift, held buttons/triggers, independent sticks
  and recovery without reconnecting. Physical-controller retesting is requested.
- **Overlay input lifetime:** stale keyboard, mouse and gamepad state is cleared
  between menu sessions. Fresh input remains usable.
- **Launcher update loop (#64):** release-candidate tags retain their `-rc.N`
  suffix when checking GitHub, instead of becoming a falsely newer stable version.
  The launcher identifies this build as `0.7.7`.
- **Upgrade preservation:** changing launcher version no longer deletes the
  entire shader/cache tree. The renderer uses its own compatibility versions.
  Saves, settings and legacy nested save backups should be retained; keep your
  own backup before testing.
- **Renderer fixes now included in the package:** shader-written HTILE metadata
  is preserved; sampler keys distinguish custom border colors; texture bindings
  use resolved descriptors after rebind; additive blending is classified per
  render target. Failed graphics compilation jobs are retired and inactive color
  targets do not cause unnecessary pipeline waits.
- **Pipeline identity:** compatible compute pipelines can reuse identical
  SPIR-V; motion-buffer addresses no longer unnecessarily specialize pipelines.
  This does not establish a measured FPS improvement or eliminate all stutter.
- **Display/settings:** actual 1600x900 renderer output, preserved 32:9 and
  5120x1440 settings, experimental native output selection, the existing 120 FPS
  option exposed in the overlay, and the full mouse-sensitivity range.
- **Startup/input:** Enter matches the launcher's Options mapping, saved language
  preferences are respected, minimized presentation is suspended, and packaged
  startup includes `run.py`.
- **Diagnostics:** process private commit and working set are separated; WDDM
  process GPU memory is distinguished from adapter-wide usage. Raw frame traces,
  true P95/P99 and pipeline-wait telemetry support further reports.

## Known issues and retesting

Overexposure/crushed blacks (#45), lighting/shadows (#26, #36), colored artifacts
on AMD (#39, #63), black polygons and flicker (#29, #30, #41) are **not declared
fixed**. Reporter logs and retained comparisons do not yet provide a matching
chapter/camera before-and-after validation. No new gamma workaround, blanket
shadow override or experimental lighting flag is enabled by default.

DLSS still requires separately supplied optional components and may fall back
when unavailable. This package does not add DLSS 4.5, FSR 4.1.1 or working Frame
Generation, nor does it guarantee 60/120 FPS or full-campaign stability.

For a useful retest, include the exact chapter/checkpoint and camera, GPU and
driver, output resolution/upscaler, a screenshot or short video and
`user\last_run.log`. For stutter, say whether it persists after revisiting the
same scene with a warm cache. Keep issues open until the affected hardware/scene
has been checked.

## Installation

1. Download `theorder1886-windows.zip` and extract it into a **new folder**.
2. Keep the previous install and back up your saves/settings. Use copies for
   testing; do not delete your shader cache as a routine upgrade step.
3. Open `The Order 1886.exe`, select the dump of your own game, and launch.
   `Play The Order 1886.exe` uses the saved configuration.

The ZIP contains no game dump, saves, player cache or optional proprietary DLSS
runtime. The first encounter with new/changed shaders can still stutter.

## Validation

Windows build and 48 C/C++ suites passed; the Python suite ran 315 tests with
37 skips and no failures. These checks include the virtual-gamepad regression,
release-tag parsing and upgrade-data preservation. Runtime/package smoke results
and the asset SHA-256 are recorded separately in the published release.

## Portugues

Esta versao inclui a correcao do controle apos fechar o menu R3+L2, a correcao do
aviso repetido de atualizacao e os ajustes integrados desde a v0.7.6-rc.1.
O launcher nao apaga mais todo o cache a cada troca de versao.

**Iluminacao, sombras e artefatos coloridos ainda nao sao considerados resolvidos.**
As issues relacionadas receberao pedido de reteste com a nova build. Extraia em
uma pasta nova, mantenha backup dos saves e envie capitulo/local, GPU/driver,
configuracoes, captura do defeito e `user\last_run.log` se o problema continuar.
