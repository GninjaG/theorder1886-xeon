The Order: 1886 for PC (theorder1886-port) - version 0.7.6-rc.1 (TEST PRE-RELEASE)
=========================================================

This is an investigation build, NOT a replacement for stable windows-v0.7.5.
It unifies the v0.7.4/v0.7.5 feature line with the flicker RCA and regression-harness work:
every v0.7.5 feature below is preserved, plus the RCA diagnostics and targeted sync fixes.
No before/after gameplay validation establishes that #29, #30 or #39 is fixed.
Extract into a NEW folder and test with COPIES of saves/configs, not your only copy.
The existing launcher upgrade routine can clear its selected profile's shader cache.
Do not use this RC with your original user profile. Keep the stable installation intact.

No game files are included. You need your own copy of The Order: 1886 as a folder that contains
eboot.bin, sce_module and sce_sys. Tested: CUSA00035, version 01.02.

Support the project on Ko-fi: https://ko-fi.com/mateusmeneses

This build runs in 100% PORTABLE mode: all configurations, saves, shaders and caches reside
locally inside this folder. No global %APPDATA% files are created.

UPDATES & IMPROVEMENTS IN v0.7.5:
- Fixed Sun Shadows (Issue #26): Corrected BufferMemoryBarrier2 flags in buffer cache download and included EVSM shadow reduction shader hash 0x8ea0ef5b with HTILE barrier flushing.
- Super Ultrawide 32:9 Support (Issue #27): Added Aspect Ratio 32:9 patch with 3.55555556 aspect ratio, 0.3796875 FOV, and launcher resolutions (3440x1440 and 5120x1440).
- Portuguese Localization Fix (Issue #28): Automatically maps Portuguese to engine id 7 on CUSA00076 and CUSA00035, preventing fallback to English. Full native PT-BR audio/subtitles preserved on CUSA00785.
- Elimination of Black Polygons, Flickers & Combat Stutter (Issues #29 & #30): Reverted invalid depth attachment eGeneral transition, keeping depth buffers in optimal depth layouts (eDepthStencilAttachmentOptimal) with full Hi-Z hardware culling.
- Windows Cooperative CPU Scheduler: Replaced busy-spinning sched_yield() with SwitchToThread() and Sleep(0) fallback in runtime_thread.c, eliminating 100% CPU lock and thread starvation on high-core processors (AMD Ryzen 9 7950X, Threadripper 3990X).
- AMD RDNA 4 Surface Format Fallback (Issue #33): Added dynamic remapping of A2B10G10R10SnormPack32 to A2B10G10R10UnormPack32 when unsupported by host Vulkan drivers, eliminating shiny silver reflection artifacts in dark scenes.
- Official DLC Runtime Loading (Issue #16): Engine automatically passes -usedlc at startup to load accessible DLC costume and weapon packages mounted at /addcont0 and /addcont1.
- 100% Portable Mode: settings.json and theorder.ini reside strictly in the local folder. Stale %APPDATA% entries are automatically purged.
- Aspect Ratio Menu Fix: In 16:9, 16:10, 21:9, and 32:9 the game pause/options menus expand to full screen.
- Support up to 120 FPS Unlocked with smooth micro-pause pacing and FSR 3.1 temporal upscaling.

2. Game configuration INI inside the extracted port folder:
   Delete 'theorder.ini' (or 'order1886.ini')

3. Shader and pipeline cache folder inside the port folder:
   Delete the 'cache' folder inside 'user':
   <PortFolder>\user\cache
   (Your save files in <PortFolder>\user\savedata are safe and will NOT be touched).

Requirements
- Windows 10 (1903 or newer) or Windows 11, 64-bit.
- A 64-bit processor with AVX. Older processors stop at start with a "STOP: this CPU lacks" line
  in the log.
- A graphics card with Vulkan 1.3 and an up-to-date driver.
  Nothing else: Python and the libraries are inside this folder.

Starting
1. Extract this folder anywhere, for example C:\Games\theorder1886-windows (not inside the zip).
2. Open "The Order 1886.exe". If Windows shows "Windows protected your PC", click More info,
   then Run anyway: the program is not digitally signed.
3. On the Game page, choose the game folder (the one with eboot.bin).
4. Back on Home, click PLAY. The first start takes longer while the game image is prepared and
   the shaders are built; later starts reuse the shader cache.

Frame rate: the game runs at its original 30 FPS. "60 (experimental)" under Engine frame rate
applies a 60 FPS patch; the speed of the game stays correct, and the prologue averages about 57
FPS (heavy scenes dip to the 40s).

Aspect ratio: the game's picture is 2.40:1 with black bars. Graphics > Aspect ratio (or the
in-game menu, Image tab) picks 16:9, 16:10 or 21:9 to fill a screen of that shape: the view keeps
its width and shows more above and below. Applied when the game starts again.

While playing
- Insert on the keyboard, or R3 + L2 on the controller, opens the port's menu (display, frame
  limit, performance overlay).
- Keyboard: WASD move, arrows camera, Space or Enter = Cross, Left Shift or Esc = Circle,
  E = Square, Q = Triangle, 1/3 = L1/R1, R/F = L2/R2, Z/C = L3/R3.

Files
- Saves, the shader cache and logs stay in this folder (user\). Reinstalling over it keeps them.
  The pipeline cache keeps at most 4 GB (older entries are removed at start).

---------------------------------------------------------------------------------------------
Português (Brasil)

Nenhum arquivo do jogo está incluído: use o dump da sua própria cópia de The Order: 1886 (pasta
com eboot.bin, sce_module e sce_sys). Testado: CUSA00035, versão 01.02.

Em desenvolvimento. Verificado: logos, tela de título e a cinemática do prólogo, nos 30 FPS
originais e sem crashes numa NVIDIA GeForce RTX 5060 (notebook). Ainda não verificado: gameplay
depois do prólogo, som, salvar, placas AMD e Intel. Problemas conhecidos: a primeira sessão monta o
cache de shaders (objetos podem aparecer com um instante de atraso e há pausas curtas); nas
seguintes há uma pausa breve quando uma cena carrega; raramente um triângulo preto pisca por um
quadro.

Como jogar: extraia a pasta, abra "The Order 1886.exe" (se o Windows avisar, clique em "Mais
informações" e "Executar assim mesmo"), escolha a pasta do jogo na página Jogo e clique em JOGAR.
A primeira execução demora mais (preparação da imagem do jogo e dos shaders).

No jogo: Insert (ou R3 + L2 no controle) abre o menu do port (tela, limite de quadros e overlay de
desempenho).
