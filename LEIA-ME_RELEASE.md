# The Order: 1886 — Port nativo para PC (Vulkan): notas da versão 0.1.0

Port em desenvolvimento de **The Order: 1886 (PS4, CUSA00035 versão 01.02)** para Windows. A
documentação completa está em [README.pt-BR.md](README.pt-BR.md) e o registro técnico em
[docs/PROGRESS.md](docs/PROGRESS.md). Nenhum arquivo do jogo está incluído: use o dump da sua
própria cópia.

## Como jogar (pacote Windows)

1. Baixe `theorder1886-windows.zip` e extraia a pasta em qualquer lugar (não rode de dentro do zip).
2. Abra **`The Order 1886.exe`**. Se o Windows avisar ("O Windows protegeu o computador"), clique
   em "Mais informações" e "Executar assim mesmo": o programa não é assinado digitalmente.
3. Na página **Jogo**, escolha a pasta do dump (a que tem `eboot.bin`, `sce_module` e `sce_sys`).
4. Clique em **JOGAR**. A primeira execução demora mais (preparação do jogo e dos shaders).

`Play The Order 1886.exe` inicia o jogo direto com as últimas configurações (útil na Steam). Saves,
cache de shaders e logs ficam na pasta `user\` ao lado do launcher.

Para rodar a partir do código-fonte: `bash build.sh` e `python run.py` (ou os `.bat` da pasta).

## Situação (15/10/2026, RTX 5060 Laptop, Windows 11)

- Abertura, tela de título, menus e cinemática do prólogo funcionam; numa sessão de 4 minutos o
  jogo manteve os 30 FPS originais, com frame time no percentil 99 de cerca de 34,5 ms.
- Mais de 20 execuções automatizadas seguidas sem crash.
- Corrigidos nesta versão:
  - textos dos menus e ícones de botão do controle que não apareciam;
  - crashes no driver da NVIDIA (duas threads gravando no mesmo command buffer, stencil anexado a
    imagens só de profundidade, cópia de textura lida fora do buffer);
  - congelamentos de 1 a 3 segundos causados por descritores de textura inválidos;
  - shaders SPIR-V inválidos.
- Problema conhecido: pequenas travadas enquanto shaders novos são compilados, principalmente nos
  primeiros minutos de cada sessão.
- Ainda não verificado: gameplay depois do prólogo, som, salvar e carregar, placas AMD e Intel.
- 60 FPS: opção experimental no launcher ("Engine frame rate: 60"), com a velocidade do jogo
  correta, mas o prólogo roda a 45–58 FPS.

## Controles

Controle (DualSense, Xbox e outros, via SDL3) e teclado funcionam juntos; a tabela de teclas está
no README. Não há suporte a mouse. **Insert** (ou R3+L2 no controle) abre o menu do port; as
trapaças desse menu são do port de God of War III e aparecem como indisponíveis neste jogo.

## Arquivos (código-fonte)

- `Jogar The Order 1886.bat`, `Jogar - Inicio Rapido.bat`, `Jogar - Modo Performance.bat`:
  atalhos para `python run.py`.
- `launcher/gow3_launcher_win.py`: o launcher (empacotado por `packaging/windows/package.sh`).
- `out/theorder-probe.exe`: o executável do port (gerado por `bash build.sh`).
- `user/savedata/`, `user/cache/CUSA00035/`: saves e cache de shaders e pipelines.
- `patches/TheOrder1886-Orbis.xml`: patches do jogo, todos desligados por padrão.
- `tools/batch_runs.py`, `tools/fault_stacks.py`: testes automatizados e análise de crashes.
