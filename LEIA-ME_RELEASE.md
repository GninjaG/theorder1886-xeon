# The Order: 1886 — Port nativo para PC (Vulkan): notas da versão 0.3.3

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

## Situação (17/10/2026, RTX 5060 Laptop, Windows 11)

Novidades da 0.3.3:

- **Outras versões do jogo não fecham mais na abertura** (issue #3): o port escrevia ganchos de
  diagnóstico e patches nos endereços do eboot 01.02 em qualquer eboot; num eboot 01.00 isso
  corrompia o código do jogo, que fechava logo ao iniciar. Os ganchos agora só entram quando cada
  ponto tem a instrução esperada, e uma linha de patch sem assinatura só vale na 01.02. Os patches
  embutidos (60 FPS e proporção de tela) ganharam assinaturas e podem valer em outras versões com
  o mesmo código. A 01.02 continua sendo a única versão testada.
- **Texturas pretas ou faltando em algumas GPUs** (issue #1, Intel Arc): os workers que preparam
  os draws antes da thread da GPU leem as tabelas de recursos de cada shader; quando essa leitura
  antecipada pegava uma memória já reaproveitada, o tratador de falhas remendava o shader para
  sempre ler zero ali. Agora essa leitura só é descartada e o draw é preparado pela thread da GPU.
  Aqui (NVIDIA) isso nunca aconteceu, nem com o cache vazio; falta a confirmação em Intel.
- Com o modo de 60 FPS, o menu do jogo não diz mais que o patch de 60 FPS está indisponível (e o
  log não diz mais "30 FPS"): o jogo já rodava a 60, só a indicação estava errada.
- O cache de shaders é mantido (sem recompilar depois de atualizar).

Novidades da 0.3.2:

- **Jogabilidade mais fluida em cenas pesadas**: no telhado de Londres logo depois do tutorial
  (~1.430 draws por quadro) o jogo rodava a 23–25 FPS; agora fica em 28–30 (o limite do jogo é
  30). Um quarto do tempo das threads da GPU era gasto esperando o lock do mapa de memória do
  jogo, que no Windows usava o rwlock lento do winpthreads; agora usa o SRWLOCK do próprio
  Windows. Cada draw custa 14–17 µs na thread da GPU em vez de 21.
- Diagnóstico para desenvolvedores: `THEORDER_SAMPLE=<nome da thread>` amostra onde as threads
  gastam tempo (`tools/sample_report.py` mostra o resultado por função), e a estatística de µs por
  draw passou a funcionar no Windows.
- O cache de shaders é mantido (sem recompilar depois de atualizar).

Novidades da 0.3.1:

- **Crash raro ao iniciar corrigido** (~1 em 40 inicializações): um componente do Windows
  (`iertutil.dll`) travava numa thread que o GIO, do GLib, abre ao ser carregado para listar os
  apps da Microsoft Store. O GIO só vinha junto com o decodificador de SVG do FFmpeg, que o jogo
  nunca usa. O pacote agora traz um `librsvg-2-2.dll` mínimo no lugar do original, e GIO, Pango e
  GDK-Pixbuf não são mais carregados.
- **Pausas menores ao carregar cenas**: os pipelines guardados (arquivos de ~2 MB) agora são lidos
  com stdio, cerca de 1,6× mais rápido (3,2–3,4 → 1,8–2,2 ms por pipeline, medido em execuções
  intercaladas); a maior pausa, no início do prólogo (~95 pipelines), caiu de ~490 para ~330 ms.
- Diagnóstico para desenvolvedores: `THEORDER_WATCH=<endereço>:<bytes>` registra cada mudança de
  um trecho da memória do jogo; os relatórios de crash mostram mais registradores e o estado da
  fila de tarefas do motor.
- Desta vez o cache de shaders não é refeito.

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

- Vindo de uma versão anterior à 0.3.0, a primeira sessão refaz o cache de shaders: objetos podem
  aparecer com um instante de atraso e há pausas curtas.
- Raramente um triângulo preto pisca por um quadro. Os vértices na memória do jogo estão certos
  (o motor os escreve uma única vez); a causa está do lado do port, em investigação.
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
