# The Order: 1886 PC Port - Windows v0.7.8

This release focuses on the R3+L2 / Insert settings menu, mixed mouse/controller
input, and launcher usability. It also packages the locally tested renderer and
pipeline-queue changes described below. Remaining flicker, lighting problems,
stutters and dialogue subtitles are not declared fixed.

## Highlights

1. **Menu scrolling and focus:** a fixed ImGui window identity keeps focus and
   scroll when mouse/controller prompts change. The right stick scrolls long
   pages; the left stick supports navigation. Mouse-wheel position and inverted
   wheel direction are respected. Source: [port_overlay.cpp](https://github.com/mateussmeneses/theorder1886-port/blob/windows-v0.7.8/gpu/shim/port_overlay.cpp).
2. **Mouse with a controller connected:** idle analog drift no longer changes
   the active input device. Mouse/keyboard takeover releases gamepad navigation
   and gamepad-owned edits. Real input switches device prompts without creating
   a new menu window. Regression: [test_overlay_input.cpp](https://github.com/mateussmeneses/theorder1886-port/blob/windows-v0.7.8/tests/test_overlay_input.cpp).
3. **Extra menu fixes:** Ctrl+Shift+Tab goes backward through tabs. Losing window
   focus clears held/queued input and the R3/L2 chord so an old trigger state
   cannot unexpectedly reopen the menu. Close/reopen isolation remains covered.
4. **Gameplay mouse sampling (#37):** the runtime normalizes mouse counts using
   monotonic timestamps and a 16 ms collection window, preserving the previous
   60 Hz curve. A 300 counts/s regression now produces the same deflection at
   30/60/120/1000 Hz. This is not native 1:1 camera input or a turn-speed unlock.
   Source: [runtime_pad.c](https://github.com/mateussmeneses/theorder1886-port/blob/windows-v0.7.8/src/runtime_pad.c).
5. **Launcher:** update checking is enabled on every window opening, including
   profiles that previously saved it disabled. Checks run in the background;
   installing remains an explicit action. Optional Ko-fi support and a GitHub
   repository link are easier to find without duplicating the action beside
   Play. Source: [port_launcher_win.py](https://github.com/mateussmeneses/theorder1886-port/blob/windows-v0.7.8/launcher/port_launcher_win.py).
6. **Renderer correctness:** color/depth overlap copies report success and
   preserve the guest-dispatch fallback for unsupported copies. The blit path
   supports a multisampled color source. Local tests and prior MSAA/no-MSAA
   smokes passed; this is not confirmation that public black-screen reports
   (#24, #43, #47) are resolved.
7. **Visible pipeline queue:** a required visible graphics pipeline can be
   promoted ahead of still-queued work. Required waits/draws remain intact;
   this cannot preempt a driver compile already running. Set
   `THEORDER_VISIBLE_PIPELINE_PRIORITY=0` to restore FIFO. Existing A/B runs do
   not establish a consistent speedup or eliminate first-visit stutter.
8. **Packaging:** missing optional FSR shader assets no longer prevent building
   a package. Supported providers still need their required assets/device
   capabilities; this does not add universal FSR 4.1.1 or Frame Generation.

## Validation And Known Limits

- Windows CLANG64 build passed. All 48 C/C++ suites passed. Python ran 316 tests
  with 39 skips and no failures.
- The overlay test uses a virtual SDL controller and the real ImGui menu at
  640x480: long-page overflow, normal/inverted wheel, stick scroll, retained
  window/scroll, drift, mouse takeover, tabs, focus loss and hotkey lifetime.
  Physical-device interaction and final-swapchain visuals still need retesting.
- Flicker/black polygons (#29, #30), lighting/shadows (#26, #36, #45), AMD colored
  artifacts (#39, #63), black-screen reports and severe stutter (#68) remain
  open. NVIDIA local checks do not establish AMD/Intel compatibility.
- Dialogue subtitles remain under investigation and were explicitly deferred
  for this release. No save edits, gamma workaround or broad shadow override
  are introduced to hide a symptom. Stable 60/120 FPS and campaign completion
  are not guaranteed.
- Automatic update discovery depends on GitHub's unauthenticated API quota.
   When rate-limited, the background check cannot announce a new version until
   the quota recovers; the manual GitHub release/download page remains available.

For reports, include chapter/checkpoint and camera, GPU/driver, game version,
resolution/upscaler, a matching capture, and a log with private paths redacted.
For stutter, distinguish a first visit from a warm-cache revisit of that scene.

## Package Contents

The Windows ZIP includes the launcher, direct-play launcher, native runtime,
required redistributable DLLs, preparation scripts and patch definitions. It
contains no game dump, saves, player-generated shader caches, private logs or
proprietary DLSS runtime. You must supply your own legally obtained game dump.

## How To Play

1. Extract `theorder1886-windows.zip` into a new folder.
2. Keep your previous installation and back up saves/settings. Do not delete
   your shader cache as a routine upgrade step. Use copies for testing.
3. Open `The Order 1886.exe`, select your game dump, and launch. The direct-play
   executable uses your saved settings.
4. Open the port menu with Insert or R3+L2. Settings changes follow the existing
   save/restart behavior; an update check does not install a release for you.

## Portugues

1. **Scroll do menu R3+L2 / Insert:** a janela mantem sua identidade, foco e
   rolagem quando os prompts mudam entre mouse e controle. O analogico direito
   rola paginas longas; o esquerdo navega. O wheel invertido tambem funciona.
2. **Mouse com controle conectado:** drift parado nao troca o dispositivo ativo.
   Ao usar mouse/teclado, a navegacao e os ajustes ativos do controle sao
   liberados, evitando scroll ou edicao presos. A troca nao recria a janela.
3. **Outros ajustes:** Ctrl+Shift+Tab volta abas; perder o foco limpa teclas,
   botoes e o estado R3/L2. O mouse do gameplay passa a normalizar contagens por
   tempo, preservando a curva de 60 Hz; nao e camera nativa 1:1.
4. **Launcher:** verifica atualizacoes a cada abertura, inclusive com preferencia
   antiga desmarcada. A instalacao continua manual. Os links opcionais de apoio
   no Ko-fi e do repositorio ficaram mais visiveis.
5. **Limites:** legendas foram adiadas. Flicker, iluminacao, sombras, artefatos
   AMD/Intel e stutters ainda precisam de retestes equivalentes; esta release
   nao declara esses relatos corrigidos nem garante 60/120 FPS.

A verificacao automatica depende da quota publica da API GitHub. Se houver
rate limit, use a pagina da release para baixar manualmente; nao e necessario
apagar saves ou caches.

Build Windows e 48 suites C/C++ passaram; a suite Python executou 316 testes,
com 39 skips e nenhuma falha. O teste automatizado usa controle SDL virtual e
o menu ImGui real; ainda falta validacao fisica nos dispositivos dos usuarios.
Extraia em pasta nova e mantenha backup dos saves/configuracoes, sem limpar os
caches como procedimento de atualizacao. Nenhum arquivo do jogo acompanha o ZIP.

## Optional Support / Apoio Opcional

Development support is optional and does not unlock features. / Apoiar e
opcional e nao desbloqueia funcionalidades.

[Support on Ko-fi / Apoiar no Ko-fi](https://ko-fi.com/mateusmeneses)