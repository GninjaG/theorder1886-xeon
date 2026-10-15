The Order: 1886 for PC (theorder1886-port) - version 0.1.0
=========================================================

No game files are included. You need your own copy of The Order: 1886 as a folder that contains
eboot.bin, sce_module and sce_sys. Tested: CUSA00035, version 01.02.

This is a work in progress. Checked so far: the logos, the title screen and the prologue cutscene,
at the game's own 30 FPS without crashes on an NVIDIA GeForce RTX 5060 laptop. Not checked yet:
gameplay after the prologue, sound, saving, AMD and Intel graphics cards. Known issue: short
stutters while new shaders are compiled, mostly in the first minutes of each session.

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
applies a 60 FPS patch; the speed of the game stays correct, but the 3D scenes do not hold 60 yet.

While playing
- Insert on the keyboard, or R3 + L2 on the controller, opens the port's menu (display, frame
  limit, performance overlay). Its cheats are God of War III's and show as unavailable here.
- Keyboard: WASD move, arrows camera, Space or Enter = Cross, Left Shift or Esc = Circle,
  E = Square, Q = Triangle, 1/3 = L1/R1, R/F = L2/R2, Z/C = L3/R3.

Files
- Saves, the shader cache and logs stay in this folder (user\). Reinstalling over it keeps them.

---------------------------------------------------------------------------------------------
Português (Brasil)

Nenhum arquivo do jogo está incluído: use o dump da sua própria cópia de The Order: 1886 (pasta
com eboot.bin, sce_module e sce_sys). Testado: CUSA00035, versão 01.02.

Em desenvolvimento. Verificado: logos, tela de título e a cinemática do prólogo, nos 30 FPS
originais e sem crashes numa NVIDIA GeForce RTX 5060 (notebook). Ainda não verificado: gameplay
depois do prólogo, som, salvar, placas AMD e Intel. Problema conhecido: pequenas travadas enquanto
shaders novos são compilados, principalmente nos primeiros minutos de cada sessão.

Como jogar: extraia a pasta, abra "The Order 1886.exe" (se o Windows avisar, clique em "Mais
informações" e "Executar assim mesmo"), escolha a pasta do jogo na página Jogo e clique em JOGAR.
A primeira execução demora mais (preparação da imagem do jogo e dos shaders).

No jogo: Insert (ou R3 + L2 no controle) abre o menu do port. As trapaças desse menu são do God
of War III e aparecem como indisponíveis aqui.
