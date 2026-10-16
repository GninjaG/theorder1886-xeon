# The Order: 1886 — Port nativo para PC (Vulkan): notas da versão 0.2.0

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

Novidades da 0.2.0 (medidas em execuções automatizadas de 70 s até o prólogo, cache já montado):

- **Fim do "pisca-pisca" de objetos e efeitos**: pipelines que uma sessão anterior já montou agora
  são criados na hora (1 a 2 ms, do cache) em vez de pular os draws até ficarem prontos. Draws
  pulados por sessão: de 1.600–2.100 para 55–111 (na sessão de 5,5 min da 0.1.0 foram 20.344).
- **Menos travadas de shader**: o cache de permutações agora recarrega todas as variantes do
  shader de iluminação de partículas (antes só as 256 primeiras, e o resto era recompilado a cada
  sessão), e texturas "lixo" de slots inativos não geram mais variantes novas. Compilação no
  thread da GPU por sessão: de 9–11 s para 1,4–2,1 s.
- **Shaders de compute recorrentes** (iluminação de partículas) compilam em segundo plano: o
  quadro não para ~200 ms a cada variante nova (as partículas daquele emissor somem por um
  instante).
- **Disco**: o cache de binários de pipeline (até 2 MB por pipeline no driver NVIDIA) chegou a
  41 GB e encheu o disco (a sessão da 0.1.0 terminou com "No space left on device"). Agora fica
  em até 4 GB (`GOW3_PIPELINE_BINARY_CACHE_MB`), removendo os mais antigos ao iniciar.
- **Crash corrigido** numa sessão longa a 60 FPS ("Unresolvable image overlap"): texturas com
  descritores inválidos no mesmo endereço de uma textura real.
- **Patches removidos**: 900p, 720p e "Letterbox Removal" escreviam no meio de instruções e
  corrompiam o código se ativados.
- 60 FPS (experimental, "Engine frame rate: 60"): 57 FPS de média numa sessão de 240 s do
  prólogo (antes 45–58), com quedas para 41–55 FPS nas cenas mais pesadas.

Problemas conhecidos:

- A primeira sessão monta o cache: objetos podem aparecer com um instante de atraso e há pausas
  curtas. Nas seguintes, uma pausa breve (até ~0,6 s) quando uma cena carrega.
- Raramente um triângulo preto pisca por um quadro (dados de vértice lidos fora de hora; em
  investigação).
- Barras pretas (letterbox 2,4:1): suporte a 16:9, 16:10 e 21:9 em andamento.
- Raro: uma falha na inicialização (thread "Task Kernel", ~1 em 40 execuções).
- Ainda não verificado: gameplay depois do prólogo, som, salvar e carregar, placas AMD e Intel.

## Controles

Controle (DualSense, Xbox e outros, via SDL3) e teclado funcionam juntos; a tabela de teclas está
no README. Não há suporte a mouse. **Insert** (ou R3+L2 no controle) abre o menu do port (tela,
limite de quadros e overlay de desempenho).

## Arquivos (código-fonte)

- `Jogar The Order 1886.bat`, `Jogar - Inicio Rapido.bat`, `Jogar - Modo Performance.bat`:
  atalhos para `python run.py`.
- `launcher/gow3_launcher_win.py`: o launcher (empacotado por `packaging/windows/package.sh`).
- `out/theorder-probe.exe`: o executável do port (gerado por `bash build.sh`).
- `user/savedata/`, `user/cache/CUSA00035/`: saves e cache de shaders e pipelines.
- `patches/TheOrder1886-Orbis.xml`: patches do jogo, todos desligados por padrão.
- `tools/batch_runs.py`, `tools/fault_stacks.py`: testes automatizados e análise de crashes.
