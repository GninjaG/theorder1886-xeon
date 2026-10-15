# The Order: 1886 PC Native Port

**English** | [Português (Brasil)](README.pt-BR.md) | [Español](README.es.md)

A work-in-progress PC port of **The Order: 1886** (PlayStation 4) for Windows 10 and 11.

The CPU side is native: the game's original executable runs directly on the PC's processor (the
PS4 uses the same x86-64 architecture), and a runtime written for this port replaces the PS4
system libraries, so there is no CPU emulator. The graphics are translated: they run on the
[shadPS4](https://github.com/shadps4-emu/shadPS4) video core, which converts the PS4 GPU commands
and shaders to Vulkan. This project started as a copy of the
[God of War III Remastered port](https://github.com/FelipePrado0/gow3_pc) and is being adapted to
Ready at Dawn's engine.

> **No game files are included.** You need a dump of your own copy of the game.
> This project is not affiliated with Sony Interactive Entertainment, Ready at Dawn or Santa
> Monica Studio. The Order: 1886 is a trademark of Sony Interactive Entertainment.

## Status

Checked on Windows 11 with an NVIDIA GeForce RTX 5060 Laptop GPU and the dump **CUSA00035,
version 01.02** (`eboot.bin` sha256 `f742eb17…`):

- The game boots through the logos and the title screen into a new game; the prologue cutscene
  renders in the engine (characters, lighting and textures checked in frame dumps).
- A 4-minute run of the prologue held the game's own 30 FPS in every 5-second window, with a
  99th-percentile frame time of about 34.5 ms, and no crash in more than 20 consecutive automated
  runs (`tools/batch_runs.py`).
- The Vulkan validation layer reports a single remaining warning class in these scenes
  (`magFilter-04553`, a linear sampler used with an integer texture).

Not verified yet: gameplay beyond the prologue, sound (the automated runs do not check it),
saving and loading, AMD and Intel graphics cards, other regions and versions of the game, Linux.
The work log, the fixes and the open items are in [docs/PROGRESS.md](docs/PROGRESS.md).

## Requirements

- Windows 10 (1903 or newer) or Windows 11, 64-bit.
- A 64-bit processor with AVX. On CPUs without MOVBE, BMI1 or LZCNT (Sandy/Ivy Bridge, AMD FX)
  the loader rewrites the game's instructions that use them to emulated stubs and logs an `ISA:`
  line; older CPUs stop at start with a `STOP: this CPU lacks` line.
- A graphics card with Vulkan 1.3 and a current driver.
- Your dump of the game: the folder with `eboot.bin`, `sce_module` and `sce_sys`. Only CUSA00035
  01.02 has been tested; the patch file also lists CUSA00070, CUSA00076, CUSA00100, CUSA00670 and
  CUSA00785.
- [MSYS2](https://www.msys2.org) and [Python 3](https://www.python.org) to build it (see below).

## Building

1. Install MSYS2 (`winget install MSYS2.MSYS2`) and, in the **MSYS2 CLANG64** shell, the packages
   listed in [packaging/windows/README.md](packaging/windows/README.md).
2. Clone with the submodules and build:

   ```bash
   git clone --recursive https://github.com/mateussmeneses/theorder1886-port
   cd theorder1886-port
   bash build.sh
   ```

   The result is `out/theorder-probe.exe` (also copied as `out/gow3-probe.exe`). If the build
   says it cannot replace the executable, a game process is still running.

## Playing

The game folder is taken from `THEORDER_GAME_DIR`; without it, `run.py` looks for a `CUSA00035`
folder next to the project folder.

```powershell
$env:THEORDER_GAME_DIR = 'D:\path\CUSA00035'
python run.py
```

Or double-click one of the batch files (their menus are in Portuguese):

- **`Jogar The Order 1886.bat`**: a menu that starts the game normally, or with the engine's own
  options described below (level shortcuts, a lighter "performance" mode).
- **`Jogar - Inicio Rapido.bat`**: starts the game (`python run.py`).
- **`Jogar - Modo Performance.bat`**: starts it with the lighter engine options.

### Engine options

These options exist in the game's executable; `run.py` passes them on. What they do in the retail
build is not verified yet: with `--level`, the game still stops at the title screen.

| `run.py` | Game option |
|---|---|
| `--level <name>` | `-startlevel <name>` (for example `cat_010_streets`, `emb_020_tunnels`, `air_010_rappel`, `pro_020_westminster`) |
| `--checkpoint <name>` | `-startcheckpoint <name>` |
| `--no-cloth` | `-disablecloth` |
| `--no-cam-springs`, `--no-cam-additive` | `-nocamsprings`, `-nocamadditive` |
| `--msaa <n>` | `-msaa <n>` |
| `--frontend` | `-frontend` |
| `--perf` | `-disablecloth -nocamsprings -nocamadditive -msaa 1` |

## Controls

Controllers (DualSense, Xbox and others) work through SDL3, together with the keyboard.

| Key | Button |
|---|---|
| WASD | left stick |
| Arrows | right stick (camera) and d-pad |
| Space or Enter | Cross |
| Left Shift or Esc | Circle |
| E | Square |
| Q | Triangle |
| 1 and 3 | L1 and R1 |
| R and F | L2 and R2 |
| Z and C | L3 and R3 |
| I, K, J, L | d-pad |
| Tab and Backspace | left and right touchpad click |
| Insert | the port's menu (R3+L2 on a controller) |

The keyboard has no Options button yet. On a controller without a touchpad, View (Back) is the
right touchpad click. `GOW3_KEY_MAP` and `GOW3_PAD_MAP` change the buttons (`src/runtime_pad.c`).

The port's menu (Insert) holds the display, frame limit and performance overlay settings. Its
cheats and stat multipliers come from the God of War III port and do nothing in this game: their
hooks are not installed for The Order.

## Patches

`patches/TheOrder1886-Orbis.xml` is applied in memory at start; the game files are not changed.
Every patch is off by default.

- **Frame Rate Patch - 60 FPS** (experimental): sets the four 30 FPS frame-time constants (33333
  µs) of the 01.02 executable to 16667; the sites were checked in the binary. The game also asks
  for a 30 Hz presentation, so the port must present every vblank and cap at 60:

  ```powershell
  Set-Content fps60.ini 'engine_fps=120'   # the launcher logic falls back to the 60 FPS patch
  $env:THEORDER_CONFIG = (Resolve-Path fps60.ini); $env:GOW3_CONFIG = $env:THEORDER_CONFIG
  $env:GOW3_FLIP_RATE = '0'; $env:GOW3_FPS_LIMIT = '60'
  python run.py
  ```

  Game speed stays correct (frame dumps line up in time with a 30 FPS run). The logos and the
  title hold 60 FPS; the in-engine prologue runs at 45-58 FPS with uneven pacing.
- Skip Intro Logos, Skip Prologue (Westminster or Chapter 1), Auto-Dismiss Title Screen,
  900p and 720p resolution, Letterbox Removal: written during earlier development and not verified.

## Settings and diagnostics

The graphics core reads these environment variables:

| Variable | Effect |
|---|---|
| `GOW3_NO_MSAA=1` / `0` | single-sampled or multisampled game images (multisampled by default on NVIDIA) |
| `GOW3_ASYNC_COMPUTE=1` | builds compute pipelines in the background (fewer stalls, but menus can lose text and button icons) |
| `GOW3_LAZY_SHADER_CACHE=0` | translates every shader again instead of loading the cached ones |
| `GOW3_FLIP_RATE`, `GOW3_FPS_LIMIT` | presentation rate overrides (see the 60 FPS patch) |
| `GOW3_VK_VALIDATION=1` | Vulkan validation layer (with `VK_ADD_LAYER_PATH` pointing to it) |
| `GOW3_GPU_LOG=info` | GPU log level |
| `GOW3_PERMUTATION_LOG=1` | why each new shader permutation was compiled |
| `GOW3_BIG_TRACK_LOG=1` | who tracks memory ranges above 256 MiB |
| `GOW3_PAD_FILE=<file>` | scripted controller input for automated runs |

Saves and the shader cache are in the project's `user` folder (`user/savedata`,
`user/cache/CUSA00035`).

## Tests

```bash
python -m unittest discover -s tests          # 273 tests, all passing on 2026-10-15
python tools/batch_runs.py --runs 8 --secs 40 --pad "24:cross 28:cross 32:cross"
python tools/fault_stacks.py out/batch/*.log
```

`tools/batch_runs.py` starts the game repeatedly, can compare conditions (`--cond`), press buttons
(`--pad`, to get past the title screen) and reports faults, freezes and compile time;
`tools/fault_stacks.py` symbolizes the fault reports.

## How it works

| Part | Where | What it does |
|---|---|---|
| Preparation | `scripts/` | Reads `eboot.bin`, applies its relocations and links the game's own `libc` and `libSceFios2` into one image |
| Loader | `src/probe.c` | Loads the image and starts the game's original code |
| Runtime | `src/runtime_*.c` | Reimplements the PS4 system functions: memory, threads, files, audio, video, controller, saves and services |
| GPU | `gpu/shadps4/` | The shadPS4 video core with cuesta4's fixes and the God of War III and The Order ports' work (asynchronous shader and pipeline builds, shader cache) |
| Port menu | `gpu/shim/gow3_overlay.cpp` | The Dear ImGui menu and performance overlay |
| Patches | `patches/TheOrder1886-Orbis.xml`, `scripts/patches.py` | Applied in memory at start |

The launcher in `launcher/` and the Linux packaging in `packaging/linux/` come from the God of War
III port and have not been adapted to this game.

## Credits

- **Felipe Prado**: the [God of War III Remastered PC port](https://github.com/FelipePrado0/gow3_pc)
  this project is copied from.
- **deadinside28**: [bloodborne_pc](https://github.com/deadinside28/bloodborne_pc), the original
  native port, and **Supermedo** (Mohammed Albarghouthi), its
  [Windows fork](https://github.com/Supermedo/bloodborne_pc).
- **cuesta4**: the [shadPS4 fork](https://github.com/cuesta4/shadPS4/tree/eltutz) (branch
  `eltutz`) whose fixes the video core carries.
- **shadPS4 team**: the [shadPS4](https://github.com/shadps4-emu/shadPS4) video core.

Also used: [LibAtrac9](https://github.com/Thealexbarney/LibAtrac9), [FFmpeg](https://ffmpeg.org),
[SDL3](https://github.com/libsdl-org/SDL), [Dear ImGui](https://github.com/ocornut/imgui),
[sirit](https://github.com/shadps4-emu/sirit), [magic_enum](https://github.com/Neargye/magic_enum),
[miniz](https://github.com/richgel999/miniz), [xbyak](https://github.com/herumi/xbyak),
[Vulkan Memory Allocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator),
[FSR-Vulkan](https://github.com/FireBurn/FSR-Vulkan) and the AMD FidelityFX SDK,
[MSYS2](https://www.msys2.org), [LLVM](https://llvm.org) and [Pillow](https://python-pillow.org).

## License

GNU GPL v2 or later ([LICENSE](LICENSE)). Third-party components keep their own licenses.
