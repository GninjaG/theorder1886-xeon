# The Order: 1886 — Port nativo para PC (Vulkan): notas da versão

Port em desenvolvimento de **The Order: 1886 (PS4, CUSA00035 versão 01.02)** para Windows. A
documentação completa está em [README.pt-BR.md](README.pt-BR.md) e o registro técnico em
[docs/PROGRESS.md](docs/PROGRESS.md). Nenhum arquivo do jogo está incluído: use o dump da sua
própria cópia.

## Como executar

1. Coloque o dump numa pasta `CUSA00035` ao lado da pasta do projeto, ou defina
   `THEORDER_GAME_DIR` com o caminho dela.
2. Dê dois cliques em **`Jogar - Inicio Rapido.bat`** (inicia o jogo) ou em
   **`Jogar The Order 1886.bat`** (menu com opções do motor do jogo; os atalhos de nível ainda
   não foram verificados: o jogo continua parando na tela de título).

## Situação (15/10/2026, RTX 5060 Laptop, Windows 11)

- Abertura, tela de título e cinemática do prólogo funcionam; numa sessão de 4 minutos o jogo
  manteve os 30 FPS originais, com frame time no percentil 99 de cerca de 34,5 ms.
- Mais de 20 execuções automatizadas seguidas sem crash.
- Corrigidos nesta versão:
  - crashes no driver da NVIDIA (duas threads gravando no mesmo command buffer, stencil anexado a
    imagens só de profundidade, cópia de textura lida fora do buffer);
  - travadas de compilação de shaders de compute (agora compilados em segundo plano);
  - congelamentos de 1 a 3 segundos causados por descritores de textura inválidos;
  - shaders SPIR-V inválidos.
- Ainda não verificado: gameplay depois do prólogo, som, salvar e carregar, placas AMD e Intel.
- 60 FPS: patch experimental, com a velocidade do jogo correta, mas o prólogo roda a 45–58 FPS.
  Veja a seção "Patches" do README.

## Controles

Controle (DualSense, Xbox e outros, via SDL3) e teclado funcionam juntos; a tabela de teclas está
no README. Não há suporte a mouse. **Insert** (ou R3+L2 no controle) abre o menu do port; as
trapaças desse menu são do port de God of War III e não funcionam neste jogo.

## Arquivos

- `Jogar The Order 1886.bat`, `Jogar - Inicio Rapido.bat`, `Jogar - Modo Performance.bat`:
  atalhos para `python run.py`.
- `out/theorder-probe.exe`: o executável do port (gerado por `bash build.sh`).
- `user/savedata/`, `user/cache/CUSA00035/`: saves e cache de shaders e pipelines.
- `patches/TheOrder1886-Orbis.xml`: patches do jogo, todos desligados por padrão.
- `tools/batch_runs.py`, `tools/fault_stacks.py`: testes automatizados e análise de crashes.
