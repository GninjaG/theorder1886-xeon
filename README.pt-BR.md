# The Order: 1886 PC Native Port

[English](README.md) | **Português (Brasil)** | [Español](README.es.md)

Port para PC, em desenvolvimento, de **The Order: 1886** (PlayStation 4) para Windows 10 e 11.

A parte de CPU é nativa: o executável original do jogo roda direto no processador do PC (o PS4
usa a mesma arquitetura x86-64) e um runtime escrito para este port substitui as bibliotecas de
sistema do PS4, então não há emulador de CPU. Os gráficos são traduzidos: rodam no núcleo de vídeo
do [shadPS4](https://github.com/shadps4-emu/shadPS4), que converte os comandos e shaders da GPU do
PS4 para Vulkan. Este projeto começou como uma cópia do
[port de God of War III Remastered](https://github.com/FelipePrado0/gow3_pc) e está sendo
adaptado ao motor da Ready at Dawn.

> **Nenhum arquivo do jogo está incluído.** Você precisa de um dump da sua própria cópia.
> Este projeto não tem relação com a Sony Interactive Entertainment, a Ready at Dawn ou o Santa
> Monica Studio. The Order: 1886 é marca registrada da Sony Interactive Entertainment.

## Situação

Verificado no Windows 11 com uma NVIDIA GeForce RTX 5060 Laptop GPU e o dump **CUSA00035,
versão 01.02** (`eboot.bin` sha256 `f742eb17…`):

- O jogo passa pelos logos e pela tela de título e inicia um novo jogo; a cinemática do prólogo é
  renderizada pelo motor (personagens, iluminação e texturas conferidos em dumps de quadros).
- Uma sessão de 4 minutos no prólogo manteve os 30 FPS do próprio jogo em todas as janelas de
  5 segundos, com frame time no percentil 99 de cerca de 34,5 ms, e não houve nenhum crash em
  mais de 20 execuções automatizadas seguidas (`tools/batch_runs.py`).
- A camada de validação do Vulkan mostra uma única classe de aviso restante nessas cenas
  (`magFilter-04553`, sampler linear usado com uma textura de inteiros).

Ainda não verificado: gameplay depois do prólogo, som (as execuções automatizadas não o conferem),
salvar e carregar, placas AMD e Intel, outras regiões e versões do jogo, Linux. O registro do
trabalho, as correções e as pendências estão em [docs/PROGRESS.md](docs/PROGRESS.md).

## Requisitos

- Windows 10 (1903 ou mais novo) ou Windows 11, 64 bits.
- Processador de 64 bits com AVX. Em CPUs sem MOVBE, BMI1 ou LZCNT (Sandy/Ivy Bridge, AMD FX) o
  carregador reescreve as instruções do jogo que as usam para rotinas emuladas e registra uma
  linha `ISA:` no log; CPUs mais antigas param na inicialização com a linha `STOP: this CPU lacks`.
- Placa de vídeo com Vulkan 1.3 e driver atualizado.
- O dump do seu jogo: a pasta com `eboot.bin`, `sce_module` e `sce_sys`. Só o CUSA00035 01.02
  foi testado; o arquivo de patches também lista CUSA00070, CUSA00076, CUSA00100, CUSA00670 e
  CUSA00785.
- [MSYS2](https://www.msys2.org) e [Python 3](https://www.python.org) para compilar (abaixo).

## Compilando

1. Instale o MSYS2 (`winget install MSYS2.MSYS2`) e, no shell **MSYS2 CLANG64**, os pacotes
   listados em [packaging/windows/README.md](packaging/windows/README.md).
2. Clone com os submódulos e compile:

   ```bash
   git clone --recursive https://github.com/mateussmeneses/theorder1886-port
   cd theorder1886-port
   bash build.sh
   ```

   O resultado é `out/theorder-probe.exe` (também copiado como `out/gow3-probe.exe`). Se o build
   disser que não consegue substituir o executável, ainda há um processo do jogo rodando.

## Jogando

A pasta do jogo vem de `THEORDER_GAME_DIR`; sem ela, o `run.py` procura uma pasta `CUSA00035` ao
lado da pasta do projeto.

```powershell
$env:THEORDER_GAME_DIR = 'D:\caminho\CUSA00035'
python run.py
```

Ou dê dois cliques em um dos arquivos `.bat`:

- **`Jogar The Order 1886.bat`**: um menu que inicia o jogo normalmente ou com as opções do
  próprio motor descritas abaixo (atalhos de nível, um modo "performance" mais leve).
- **`Jogar - Inicio Rapido.bat`**: inicia o jogo (`python run.py`).
- **`Jogar - Modo Performance.bat`**: inicia com as opções mais leves do motor.

### Opções do motor

Estas opções existem no executável do jogo e o `run.py` as repassa. O efeito delas na versão
comercial ainda não foi verificado: com `--level`, o jogo continua parando na tela de título.

| `run.py` | Opção do jogo |
|---|---|
| `--level <nome>` | `-startlevel <nome>` (por exemplo `cat_010_streets`, `emb_020_tunnels`, `air_010_rappel`, `pro_020_westminster`) |
| `--checkpoint <nome>` | `-startcheckpoint <nome>` |
| `--no-cloth` | `-disablecloth` |
| `--no-cam-springs`, `--no-cam-additive` | `-nocamsprings`, `-nocamadditive` |
| `--msaa <n>` | `-msaa <n>` |
| `--frontend` | `-frontend` |
| `--perf` | `-disablecloth -nocamsprings -nocamadditive -msaa 1` |

## Controles

Controles (DualSense, Xbox e outros) funcionam pelo SDL3, junto com o teclado.

| Tecla | Botão |
|---|---|
| WASD | analógico esquerdo |
| Setas | analógico direito (câmera) e direcional |
| Espaço ou Enter | ✕ (Cross) |
| Shift esquerdo ou Esc | ○ (Circle) |
| E | □ (Square) |
| Q | △ (Triangle) |
| 1 e 3 | L1 e R1 |
| R e F | L2 e R2 |
| Z e C | L3 e R3 |
| I, K, J, L | direcional |
| Tab e Backspace | clique esquerdo e direito do touchpad |
| Insert | menu do port (R3+L2 no controle) |

O teclado ainda não tem o botão Options. Em controles sem touchpad, View (Back) é o clique
direito do touchpad. `GOW3_KEY_MAP` e `GOW3_PAD_MAP` trocam os botões (`src/runtime_pad.c`).

O menu do port (Insert) tem as opções de tela, limite de quadros e o overlay de desempenho. As
trapaças e os multiplicadores dele vêm do port de God of War III e não fazem nada neste jogo: os
hooks deles não são instalados no The Order.

## Patches

O `patches/TheOrder1886-Orbis.xml` é aplicado na memória ao iniciar; os arquivos do jogo não são
alterados. Todos os patches vêm desligados.

- **Frame Rate Patch - 60 FPS** (experimental): troca as quatro constantes de tempo de quadro de
  30 FPS (33333 µs) do executável 01.02 por 16667; os pontos foram conferidos no binário. O jogo
  também pede apresentação a 30 Hz, então o port precisa apresentar a cada vblank e limitar a 60:

  ```powershell
  Set-Content fps60.ini 'engine_fps=120'   # a lógica do launcher cai para o patch de 60 FPS
  $env:THEORDER_CONFIG = (Resolve-Path fps60.ini); $env:GOW3_CONFIG = $env:THEORDER_CONFIG
  $env:GOW3_FLIP_RATE = '0'; $env:GOW3_FPS_LIMIT = '60'
  python run.py
  ```

  A velocidade do jogo continua correta (os dumps de quadros batem no tempo com uma execução a
  30 FPS). Logos e título ficam em 60 FPS; o prólogo no motor roda a 45-58 FPS com ritmo irregular.
- Pular logos, pular o prólogo (Westminster ou Capítulo 1), dispensar a tela de título,
  resolução 900p e 720p, remover as faixas pretas: escritos em sessões anteriores de
  desenvolvimento e não verificados.

## Configurações e diagnóstico

O núcleo gráfico lê estas variáveis de ambiente:

| Variável | Efeito |
|---|---|
| `GOW3_NO_MSAA=1` / `0` | imagens do jogo sem ou com multisampling (com, por padrão, na NVIDIA) |
| `GOW3_ASYNC_COMPUTE=0` | compila os pipelines de compute na thread da GPU (travadas) |
| `GOW3_LAZY_SHADER_CACHE=0` | traduz todos os shaders de novo em vez de carregar os do cache |
| `GOW3_FLIP_RATE`, `GOW3_FPS_LIMIT` | taxa de apresentação (ver o patch de 60 FPS) |
| `GOW3_VK_VALIDATION=1` | camada de validação do Vulkan (com `VK_ADD_LAYER_PATH` apontando para ela) |
| `GOW3_GPU_LOG=info` | nível do log da GPU |
| `GOW3_PERMUTATION_LOG=1` | por que cada nova permutação de shader foi compilada |
| `GOW3_BIG_TRACK_LOG=1` | quem rastreia faixas de memória acima de 256 MiB |
| `GOW3_PAD_FILE=<arquivo>` | entrada de controle roteirizada para execuções automatizadas |

Os saves e o cache de shaders ficam na pasta `user` do projeto (`user/savedata`,
`user/cache/CUSA00035`).

## Testes

```bash
python -m unittest discover -s tests          # 273 testes, todos passando em 15/10/2026
python tools/batch_runs.py --runs 8 --secs 40 --pad "24:cross 28:cross 32:cross"
python tools/fault_stacks.py out/batch/*.log
```

O `tools/batch_runs.py` inicia o jogo várias vezes, compara condições (`--cond`), aperta botões
(`--pad`, para passar da tela de título) e informa crashes, travamentos e tempo de compilação; o
`tools/fault_stacks.py` simboliza os relatórios de crash.

## Como funciona

| Parte | Onde | O que faz |
|---|---|---|
| Preparação | `scripts/` | Lê o `eboot.bin`, aplica as relocações e liga a `libc` e a `libSceFios2` do próprio jogo numa imagem só |
| Carregador | `src/probe.c` | Carrega a imagem e inicia o código original do jogo |
| Runtime | `src/runtime_*.c` | Reimplementa as funções de sistema do PS4: memória, threads, arquivos, áudio, vídeo, controle, saves e serviços |
| GPU | `gpu/shadps4/` | O núcleo de vídeo do shadPS4 com as correções do cuesta4 e o trabalho dos ports de God of War III e The Order (compilação assíncrona de shaders e pipelines, cache de shaders) |
| Menu do port | `gpu/shim/gow3_overlay.cpp` | O menu Dear ImGui e o overlay de desempenho |
| Patches | `patches/TheOrder1886-Orbis.xml`, `scripts/patches.py` | Aplicados na memória ao iniciar |

O launcher em `launcher/` e o empacotamento para Linux em `packaging/linux/` vêm do port de God
of War III e ainda não foram adaptados a este jogo.

## Créditos

- **Felipe Prado**: o [port de God of War III Remastered para PC](https://github.com/FelipePrado0/gow3_pc)
  do qual este projeto foi copiado.
- **deadinside28**: [bloodborne_pc](https://github.com/deadinside28/bloodborne_pc), o port nativo
  original, e **Supermedo** (Mohammed Albarghouthi), o
  [fork para Windows](https://github.com/Supermedo/bloodborne_pc).
- **cuesta4**: o [fork do shadPS4](https://github.com/cuesta4/shadPS4/tree/eltutz) (branch
  `eltutz`) cujas correções o núcleo de vídeo carrega.
- **Equipe do shadPS4**: o núcleo de vídeo do [shadPS4](https://github.com/shadps4-emu/shadPS4).

Também usados: [LibAtrac9](https://github.com/Thealexbarney/LibAtrac9), [FFmpeg](https://ffmpeg.org),
[SDL3](https://github.com/libsdl-org/SDL), [Dear ImGui](https://github.com/ocornut/imgui),
[sirit](https://github.com/shadps4-emu/sirit), [magic_enum](https://github.com/Neargye/magic_enum),
[miniz](https://github.com/richgel999/miniz), [xbyak](https://github.com/herumi/xbyak),
[Vulkan Memory Allocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator),
[FSR-Vulkan](https://github.com/FireBurn/FSR-Vulkan) e o AMD FidelityFX SDK,
[MSYS2](https://www.msys2.org), [LLVM](https://llvm.org) e [Pillow](https://python-pillow.org).

## Licença

GNU GPL v2 ou posterior ([LICENSE](LICENSE)). Componentes de terceiros mantêm suas próprias
licenças.
