# The Order: 1886 — Port nativo para PC (Vulkan): notas da versão 0.3.0

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

## Situação (16/10/2026, RTX 5060 Laptop, Windows 11)

Novidades da 0.3.0:

- **Crash na jogabilidade corrigido**: os construtores estáticos do jogo rodavam duas vezes (o
  loader chamava o inicializador e o próprio `_start` do jogo também), e os pools de memória do
  motor (1,6 GB) eram alocados em dobro. Ao carregar o save no ponto do tutorial de cobertura, a
  memória direta acabava e o thread de streaming travava o jogo. Com o mesmo save, agora roda
  7 minutos com 4,2 de 5,3 GB em uso.
- **Jogabilidade "azul/estourada" corrigida**: depois do prólogo a imagem virava uma cor chapada
  azul-clara só com o HUD. Um vertex shader sem recursos reaproveitava uma variante compilada com
  outro deslocamento de user data e desenhava a limpeza de um buffer de neblina fora da tela; o
  buffer guardava a tela de carregamento e estourava a exposição.
- **Proporção da imagem** (launcher: Gráficos e configurações rápidas; menu do jogo: Imagem):
  Original (2,40:1 com faixas pretas), **16:9**, **16:10** ou **21:9** preenchem a tela sem
  faixas. A visão mantém a largura original e mostra mais em cima e embaixo (Vert+), e o port
  recorta as faixas que sobram em telas 16:10 e 21:9. Custo a 60 FPS no prólogo: ~1%.
- Shaders: as multiplicações "legacy" do GCN (0 × Inf = 0) e os rcp/rsq/log com clamp agora
  seguem o hardware. O cache de shaders é refeito uma vez na primeira sessão.
- Limpeza: tudo que restava do port de God of War III saiu (cheats, patches, fontes, ícones);
  o launcher só oferece opções que funcionam no The Order. As variáveis de ambiente passaram de
  `GOW3_*` para `THEORDER_*` e o arquivo de configurações de `gow3.ini` para `theorder.ini`
  (movido sozinho na primeira execução).

Problemas conhecidos:

- A primeira sessão depois da atualização refaz o cache de shaders: objetos podem aparecer com um
  instante de atraso e há pausas curtas.
- Raramente um triângulo preto pisca por um quadro (um passe de tela cheia lê vértices que o
  motor reescreve a cada quadro; em investigação).
- Ainda não verificado: a maior parte da campanha depois do primeiro capítulo, som, placas AMD e
  Intel.

## Controles

Controle (DualSense, Xbox e outros, via SDL3) e teclado funcionam juntos; a tabela de teclas está
no README. Não há suporte a mouse. **Insert** (ou R3+L2 no controle) abre o menu do port (tela,
limite de quadros e overlay de desempenho).

## Arquivos (código-fonte)

- `Jogar The Order 1886.bat`, `Jogar - Inicio Rapido.bat`, `Jogar - Modo Performance.bat`:
  atalhos para `python run.py`.
- `launcher/port_launcher_win.py`: o launcher (empacotado por `packaging/windows/package.sh`).
- `out/theorder-probe.exe`: o executável do port (gerado por `bash build.sh`).
- `user/savedata/`, `user/cache/CUSA00035/`: saves e cache de shaders e pipelines.
- `patches/TheOrder1886-Orbis.xml`: patches do jogo, todos desligados por padrão.
- `tools/batch_runs.py`, `tools/fault_stacks.py`: testes automatizados e análise de crashes.
