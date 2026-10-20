# The Order: 1886 PC Port — Release windows-v0.7.5

This release delivers major visual rendering fixes, comprehensive aspect ratio support up to 32:9 Super Ultrawide, Portuguese localization fallback improvements, thread scheduling optimizations for Windows, and broad hardware compatibility fixes.

---

### Highlights & Key Improvements

#### 1. Fix Sun Shadow Flickering & Dissolving (Issue #26)
* Fixed Vulkan `VkBufferMemoryBarrier2` dependency flags in the buffer cache (`gpu/shadps4/video_core/buffer_cache/buffer_cache.cpp`).
* Added compute shader `0x8ea0ef5b` to compute pipeline caches with barrier flushes before dispatch (`gpu/shadps4/video_core/renderer_vulkan/vk_rasterizer.cpp`), ensuring sun cascaded shadow maps and screen-space shadows are consistently rendered without strobing.

#### 2. Super Ultrawide 32:9 Support (Issue #27)
* Added official 32:9 Aspect Ratio patch to `patches/TheOrder1886-Orbis.xml`.
* Updated launcher resolution dropdown and `scripts/graphics_options.py` to seamlessly configure 32:9 resolutions (e.g. 5120x1440, 3840x1080).

#### 3. Portuguese Localization Fallback (Issue #28)
* Added dynamic fallback to European Portuguese (`PT-PT`, Language ID 7) for European and Asian SKUs (`CUSA00076`, `CUSA00035`) that do not contain Brazilian Portuguese audio/text assets.
* Preserves native `PT-BR` (Language ID 17) on Americas SKUs (`CUSA00785`).

#### 4. Combat Micro-Stutter & Black Polygon Hi-Z Fixes (Issues #29 & #30)
* Reverted unstable `eGeneral` layout transitions on depth-stencil attachments in Vulkan rasterizer (`vk_rasterizer.cpp`), maintaining proper read-only depth layout and eliminating black flickering geometry.
* Integrated Windows cooperative thread yielding with `SwitchToThread()` / `Sleep(0)` in `src/runtime_thread.c`, smoothing frame pacing during intensive combat encounters.

#### 5. AMD RX 9000 Normal Maps & Shiny Silver Fallback (Issue #33)
* Added dynamic texture format remapping fallback from `A2B10G10R10SnormPack32` to `A2B10G10R10UnormPack32` when unsupported by host drivers in `liverpool_to_vk` and `vk_instance.cpp`, fixing silver/white metallic normal reflections on AMD RDNA 4 GPUs.

#### 6. Official DLC Support (Issue #16)
* Added default `-usedlc` guest argument flag in `run.py`, automatically mounting and loading official game add-on content and costumes.

#### 7. Launcher Control Reset & Input Enhancements
* Refined controller and keyboard reset routines in `launcher/port_launcher_win.py` so that clicking "Reset to Defaults" thoroughly restores all gamepad and keyboard mappings.

---

### Package Contents
- `The Order 1886.exe`: Standalone graphical launcher with configuration manager, DLC management, control mapping, and graphics options.
- `Play The Order 1886.exe`: Fast-launch runner executable.
- `bin/`: Optimized 64-bit runtime binaries (`theorder-probe.exe`, `theorder-gpu-capabilities.exe`) with bundled MSYS2/MinGW Clang64 dependencies.
- `patches/`: Community and widescreen patches including 21:9 and 32:9 ultrawide, 60+ FPS unlock, and letterbox removal.
- `scripts/`: Python orchestration utilities and configuration scripts.
