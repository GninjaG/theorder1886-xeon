# The Order: 1886 PC Native Port

[English](README.md) | [Português (Brasil)](README.pt-BR.md) | **Español**

Port para PC, en desarrollo, de **The Order: 1886** (PlayStation 4) para Windows 10 y 11.

La parte de CPU es nativa: el ejecutable original del juego corre directamente en el procesador
del PC (la PS4 usa la misma arquitectura x86-64) y un runtime escrito para este port reemplaza las
bibliotecas de sistema de la PS4, así que no hay emulador de CPU. Los gráficos se traducen: corren
en el núcleo de vídeo de [shadPS4](https://github.com/shadps4-emu/shadPS4), que convierte los
comandos y shaders de la GPU de la PS4 a Vulkan. Este proyecto empezó como una copia del
[port de God of War III Remastered](https://github.com/FelipePrado0/gow3_pc) y se está adaptando
al motor de Ready at Dawn.

> **No se incluye ningún archivo del juego.** Necesitas un volcado de tu propia copia.
> Este proyecto no está afiliado a Sony Interactive Entertainment, Ready at Dawn ni Santa Monica
> Studio. The Order: 1886 es una marca de Sony Interactive Entertainment.

## Estado

Comprobado en Windows 11 con una NVIDIA GeForce RTX 5060 Laptop GPU y el volcado **CUSA00035,
versión 01.02** (`eboot.bin` sha256 `f742eb17…`):

- El juego pasa por los logos y la pantalla de título e inicia una partida nueva; la cinemática
  del prólogo se renderiza en el motor (personajes, iluminación y texturas revisados en volcados
  de fotogramas).
- Una sesión de 4 minutos del prólogo mantuvo los 30 FPS propios del juego en cada ventana de
  5 segundos, con un tiempo de fotograma en el percentil 99 de unos 34,5 ms, y ningún fallo en
  más de 20 ejecuciones automatizadas seguidas (`tools/batch_runs.py`).
- La capa de validación de Vulkan muestra una sola clase de aviso restante en estas escenas
  (`magFilter-04553`, un sampler lineal usado con una textura de enteros).

Aún sin comprobar: el juego después del prólogo, el sonido (las ejecuciones automatizadas no lo
comprueban), guardar y cargar, tarjetas AMD e Intel, otras regiones y versiones del juego, Linux.
El registro del trabajo, las correcciones y los pendientes están en
[docs/PROGRESS.md](docs/PROGRESS.md).

## Requisitos

- Windows 10 (1903 o más reciente) o Windows 11, 64 bits.
- Procesador de 64 bits con AVX. En CPUs sin MOVBE, BMI1 o LZCNT (Sandy/Ivy Bridge, AMD FX) el
  cargador reescribe las instrucciones del juego que las usan con rutinas emuladas y registra una
  línea `ISA:`; las CPUs más antiguas se detienen al iniciar con la línea `STOP: this CPU lacks`.
- Tarjeta gráfica con Vulkan 1.3 y controlador actualizado.
- El volcado de tu juego: la carpeta con `eboot.bin`, `sce_module` y `sce_sys`. Solo se probó
  CUSA00035 01.02; el archivo de parches también lista CUSA00070, CUSA00076, CUSA00100, CUSA00670
  y CUSA00785.
- [MSYS2](https://www.msys2.org) y [Python 3](https://www.python.org) para compilar (abajo).

## Compilación

1. Instala MSYS2 (`winget install MSYS2.MSYS2`) y, en el shell **MSYS2 CLANG64**, los paquetes de
   [packaging/windows/README.md](packaging/windows/README.md).
2. Clona con los submódulos y compila:

   ```bash
   git clone --recursive https://github.com/mateussmeneses/theorder1886-port
   cd theorder1886-port
   bash build.sh
   ```

   El resultado es `out/theorder-probe.exe` (también copiado como `out/gow3-probe.exe`). Si la
   compilación no puede reemplazar el ejecutable, todavía hay un proceso del juego en marcha.

## Jugar

La carpeta del juego se toma de `THEORDER_GAME_DIR`; sin ella, `run.py` busca una carpeta
`CUSA00035` junto a la carpeta del proyecto.

```powershell
$env:THEORDER_GAME_DIR = 'D:\ruta\CUSA00035'
python run.py
```

O haz doble clic en uno de los archivos `.bat` (sus menús están en portugués):

- **`Jogar The Order 1886.bat`**: un menú que inicia el juego normalmente o con las opciones del
  propio motor descritas abajo (atajos de nivel, un modo "rendimiento" más ligero).
- **`Jogar - Inicio Rapido.bat`**: inicia el juego (`python run.py`).
- **`Jogar - Modo Performance.bat`**: lo inicia con las opciones más ligeras del motor.

### Opciones del motor

Estas opciones existen en el ejecutable del juego y `run.py` las pasa. Su efecto en la versión
comercial aún no está comprobado: con `--level`, el juego sigue deteniéndose en la pantalla de
título.

| `run.py` | Opción del juego |
|---|---|
| `--level <nombre>` | `-startlevel <nombre>` (por ejemplo `cat_010_streets`, `emb_020_tunnels`, `air_010_rappel`, `pro_020_westminster`) |
| `--checkpoint <nombre>` | `-startcheckpoint <nombre>` |
| `--no-cloth` | `-disablecloth` |
| `--no-cam-springs`, `--no-cam-additive` | `-nocamsprings`, `-nocamadditive` |
| `--msaa <n>` | `-msaa <n>` |
| `--frontend` | `-frontend` |
| `--perf` | `-disablecloth -nocamsprings -nocamadditive -msaa 1` |

## Controles

Los mandos (DualSense, Xbox y otros) funcionan con SDL3, junto con el teclado.

| Tecla | Botón |
|---|---|
| WASD | stick izquierdo |
| Flechas | stick derecho (cámara) y cruceta |
| Espacio o Enter | Cruz |
| Shift izquierdo o Esc | Círculo |
| E | Cuadrado |
| Q | Triángulo |
| 1 y 3 | L1 y R1 |
| R y F | L2 y R2 |
| Z y C | L3 y R3 |
| I, K, J, L | cruceta |
| Tab y Retroceso | clic izquierdo y derecho del panel táctil |
| Insert | el menú del port (R3+L2 en el mando) |

El teclado aún no tiene el botón Options. En mandos sin panel táctil, View (Back) es el clic
derecho del panel. `GOW3_KEY_MAP` y `GOW3_PAD_MAP` cambian los botones (`src/runtime_pad.c`).

El menú del port (Insert) tiene las opciones de pantalla, límite de fotogramas y la superposición
de rendimiento. Sus trucos y multiplicadores vienen del port de God of War III y no hacen nada en
este juego: sus hooks no se instalan en The Order.

## Parches

`patches/TheOrder1886-Orbis.xml` se aplica en memoria al iniciar; los archivos del juego no se
modifican. Todos los parches vienen desactivados.

- **Frame Rate Patch - 60 FPS** (experimental): cambia las cuatro constantes de tiempo de
  fotograma de 30 FPS (33333 µs) del ejecutable 01.02 por 16667; los puntos se comprobaron en el
  binario. El juego además pide presentar a 30 Hz, así que el port debe presentar en cada vblank y
  limitar a 60:

  ```powershell
  Set-Content fps60.ini 'engine_fps=120'   # la lógica del lanzador recurre al parche de 60 FPS
  $env:THEORDER_CONFIG = (Resolve-Path fps60.ini); $env:GOW3_CONFIG = $env:THEORDER_CONFIG
  $env:GOW3_FLIP_RATE = '0'; $env:GOW3_FPS_LIMIT = '60'
  python run.py
  ```

  La velocidad del juego sigue siendo correcta (los volcados de fotogramas coinciden en el tiempo
  con una ejecución a 30 FPS). Logos y título se mantienen a 60 FPS; en una sesión de 240 s del
  prólogo cada ventana de 5 s quedó entre 41 y 60 FPS (57 de media), con las caídas donde el hilo de
  comandos de la GPU está ocupado todo el fotograma (600 a 700 draws por fotograma).
- Saltar logos, saltar el prólogo (Westminster o Capítulo 1), cerrar la pantalla de título:
  escritos en sesiones anteriores de desarrollo y sin comprobar. Los parches de 900p, 720p y de
  quitar las franjas negras de esa época se retiraron: sus direcciones caían en medio de
  instrucciones (allí no estaban el tamaño ni la proporción de la imagen) y activarlos corrompía
  el código.

## Ajustes y diagnóstico

El núcleo gráfico lee estas variables de entorno:

| Variable | Efecto |
|---|---|
| `GOW3_NO_MSAA=1` / `0` | imágenes del juego sin o con multisampling (con, por defecto, en NVIDIA) |
| `GOW3_ASYNC_COMPUTE=1` | compila los pipelines de compute en segundo plano (menos tirones, pero los menús pueden perder textos e iconos de botones) |
| `GOW3_LAZY_SHADER_CACHE=0` | vuelve a traducir todos los shaders en vez de cargar los de la caché |
| `GOW3_FLIP_RATE`, `GOW3_FPS_LIMIT` | tasa de presentación (ver el parche de 60 FPS) |
| `GOW3_VK_VALIDATION=1` | capa de validación de Vulkan (con `VK_ADD_LAYER_PATH` apuntando a ella) |
| `GOW3_GPU_LOG=info` | nivel del registro de la GPU |
| `GOW3_PERMUTATION_LOG=1` | por qué se compiló cada nueva permutación de shader |
| `GOW3_BIG_TRACK_LOG=1` | quién rastrea rangos de memoria de más de 256 MiB |
| `GOW3_PAD_FILE=<archivo>` | entrada de mando programada para ejecuciones automatizadas |

Las partidas guardadas y la caché de shaders están en la carpeta `user` del proyecto
(`user/savedata`, `user/cache/CUSA00035`).

## Pruebas

```bash
python -m unittest discover -s tests          # 273 pruebas, todas pasan el 15/10/2026
python tools/batch_runs.py --runs 8 --secs 40 --pad "24:cross 28:cross 32:cross"
python tools/fault_stacks.py out/batch/*.log
```

`tools/batch_runs.py` inicia el juego varias veces, compara condiciones (`--cond`), pulsa botones
(`--pad`, para pasar la pantalla de título) e informa fallos, bloqueos y tiempo de compilación;
`tools/fault_stacks.py` simboliza los informes de fallo.

## Cómo funciona

| Parte | Dónde | Qué hace |
|---|---|---|
| Preparación | `scripts/` | Lee `eboot.bin`, aplica sus reubicaciones y enlaza la `libc` y la `libSceFios2` del propio juego en una sola imagen |
| Cargador | `src/probe.c` | Carga la imagen e inicia el código original del juego |
| Runtime | `src/runtime_*.c` | Reimplementa las funciones de sistema de la PS4: memoria, hilos, archivos, audio, vídeo, mando, partidas guardadas y servicios |
| GPU | `gpu/shadps4/` | El núcleo de vídeo de shadPS4 con las correcciones de cuesta4 y el trabajo de los ports de God of War III y The Order (compilación asíncrona de shaders y pipelines, caché de shaders) |
| Menú del port | `gpu/shim/gow3_overlay.cpp` | El menú Dear ImGui y la superposición de rendimiento |
| Parches | `patches/TheOrder1886-Orbis.xml`, `scripts/patches.py` | Se aplican en memoria al iniciar |

El lanzador de `launcher/` y el empaquetado para Linux de `packaging/linux/` vienen del port de
God of War III y aún no se adaptaron a este juego.

## Créditos

- **Felipe Prado**: el [port de God of War III Remastered para PC](https://github.com/FelipePrado0/gow3_pc)
  del que se copió este proyecto.
- **deadinside28**: [bloodborne_pc](https://github.com/deadinside28/bloodborne_pc), el port nativo
  original, y **Supermedo** (Mohammed Albarghouthi), su
  [fork para Windows](https://github.com/Supermedo/bloodborne_pc).
- **cuesta4**: el [fork de shadPS4](https://github.com/cuesta4/shadPS4/tree/eltutz) (rama `eltutz`)
  cuyas correcciones lleva el núcleo de vídeo.
- **Equipo de shadPS4**: el núcleo de vídeo de [shadPS4](https://github.com/shadps4-emu/shadPS4).

También se usan: [LibAtrac9](https://github.com/Thealexbarney/LibAtrac9), [FFmpeg](https://ffmpeg.org),
[SDL3](https://github.com/libsdl-org/SDL), [Dear ImGui](https://github.com/ocornut/imgui),
[sirit](https://github.com/shadps4-emu/sirit), [magic_enum](https://github.com/Neargye/magic_enum),
[miniz](https://github.com/richgel999/miniz), [xbyak](https://github.com/herumi/xbyak),
[Vulkan Memory Allocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator),
[FSR-Vulkan](https://github.com/FireBurn/FSR-Vulkan) y el AMD FidelityFX SDK,
[MSYS2](https://www.msys2.org), [LLVM](https://llvm.org) y [Pillow](https://python-pillow.org).

## Licencia

GNU GPL v2 o posterior ([LICENSE](LICENSE)). Los componentes de terceros conservan sus propias
licencias.
