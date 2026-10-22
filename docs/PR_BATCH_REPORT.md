# Relatório Consolidado de Integração em Lote de PRs

**Branch de Integração:** `integration/prs-batch-20261021`  
**Commit-Base:** `10e44b4 docs: add single-trunk branching/release convention + v0.7.6-rc.1 release notes`  
**Data:** 21 de outubro de 2026  
**Status do Lote:** 15 PRs integrados com sucesso | 0 rejeitados | Compilação 100% verde | 45/45 testes C/C++ aprovados

---

## 1. PRs Integrados no Lote

Todos os 15 Pull Requests abertos no repositório foram integrados sequencialmente na branch experimental local, preservando os commits originais e a autoria de Igor Martins (`@figormartins`):

| PR | Commit de Merge | Título | Issue | Subsistema |
|:---:|:---:|:---|:---:|:---|
| **#48** | `69ae1bb` | fix(input): align Enter with the launcher Options binding | [#38](https://github.com/mateussmeneses/theorder1886-port/issues/38) | Host Input / Pad Map |
| **#49** | `fe48674` | fix(graphics): preserve 32:9 and 5120x1440 settings | [#27](https://github.com/mateussmeneses/theorder1886-port/issues/27) | Display & Graphics Shim |
| **#50** | `1eaa10d` | fix(startup): respect saved language preferences | [#28](https://github.com/mateussmeneses/theorder1886-port/issues/28) | Python Startup / Config |
| **#51** | `8eea357` | fix(controls): expose the full mouse sensitivity range | [#37](https://github.com/mateussmeneses/theorder1886-port/issues/37) | In-Game Overlay UI |
| **#52** | `3429aba` | fix(packaging): include the Python startup script | [#2](https://github.com/mateussmeneses/theorder1886-port/issues/2) | Packaging / Nix |
| **#53** | `c8bc098` | fix(overlay): expose the existing 120 FPS option | [#22](https://github.com/mateussmeneses/theorder1886-port/issues/22) | In-Game Overlay UI |
| **#54** | `809a331` | fix(renderer): avoid pipeline waits for inactive color targets | [#41](https://github.com/mateussmeneses/theorder1886-port/issues/41) | Vulkan Rasterizer / Stalls |
| **#55** | `c732068` | fix(presentation): suspend swapchain use while minimized | [#10](https://github.com/mateussmeneses/theorder1886-port/issues/10) | Presenter & Swapchain |
| **#56** | `9f02dff` | fix(renderer): retire failed graphics compilation jobs | [#40](https://github.com/mateussmeneses/theorder1886-port/issues/40) | Pipeline Cache / CPU 100% |
| **#57** | `9534abf` | fix(renderer): preserve shader-written HTILE metadata | [#26](https://github.com/mateussmeneses/theorder1886-port/issues/26) | Buffer Cache / Shadows |
| **#58** | `b59d4a7` | fix(renderer): classify additive targets using their own blend state | [#45](https://github.com/mateussmeneses/theorder1886-port/issues/45) | Vulkan Rasterizer / Blend |
| **#59** | `8e82c52` | feat(launcher): expose experimental native output resolution | [#35](https://github.com/mateussmeneses/theorder1886-port/issues/35) | Launcher / Native 4K |
| **#60** | `2e02de4` | fix(renderer): use the resolved descriptor after texture rebind | [#29](https://github.com/mateussmeneses/theorder1886-port/issues/29) | Vulkan Rasterizer / Bindings |
| **#61** | `5a2d9c0` | fix(upscaler): retry DLSS after an explicit reselection | [#31](https://github.com/mateussmeneses/theorder1886-port/issues/31) | Temporal Upscaler / DLSS |
| **#62** | `78d1208` | fix(renderer): include custom border colors in sampler cache keys | [#39](https://github.com/mateussmeneses/theorder1886-port/issues/39) | Texture Cache / Samplers |

---

## 2. PRs Não Integrados e Motivos

**Nenhum.** Todos os 15 PRs foram integrados com sucesso.

---

## 3. Conflitos e Resoluções

Houve três arquivos modificados simultaneamente por múltiplos PRs. Todos foram mesclados de forma limpa pela estratégia `ort` sem colisões de texto:
1. `gpu/shim/port_overlay.cpp`:
   - Modificado por **PR #51** (`ControlsTab`, range do slider de sensibilidade).
   - Modificado por **PR #53** (`PerformanceTab`, inclusão de 120 FPS e mensagem dinâmica de fallback).
   - *Resolução:* Mesclagem automática limpa (funções independentes).
2. `gpu/shadps4/video_core/renderer_vulkan/vk_rasterizer.cpp`:
   - Modificado por **PR #54** (`TargetsVisibleSceneAttachment` e `ResolvePendingGraphics`).
   - Modificado por **PR #58** (`BeginRenderingFull` e helper `IsAdditiveColorTarget`).
   - Modificado por **PR #60** (`BindTextures` rebind descriptor).
   - *Resolução:* Mesclagem automática limpa em todos os três pontos sem sobreposição de linhas.
3. `tests/test_launcher_controls.py`:
   - Modificado por **PR #48** (asserções de Enter e Options).
   - Modificado por **PR #59** (asserções de `THEORDER_UPSCALER=off`).
   - *Resolução:* Mesclagem automática limpa (métodos de teste distintos).

---

## 4. Resultado da Compilação

- **Toolchain:** Clang 22.1.7 (`x86_64-w64-windows-gnu`) via MSYS2 CLANG64 + Ninja 1.13.2.
- **Diretório:** `out/gpu`.
- **Artefatos Gerados:**
  - `libtheordergpu.a` (biblioteca estática do renderizador e subsistemas).
  - `theorder-probe.exe` (probe de ELF, patches e inicialização).
  - `theorder-gpu-capabilities.exe` (detector de capacidades Vulkan da GPU).
  - 45 executáveis de testes unitários em `out/gpu/*-test.exe`.
- **Erros de Compilação:** 0 erros. Apenas avisos pré-existentes de `memset`/`memcpy` em estruturas não-triviais.

---

## 5. Resultados dos Testes Automatizados

### 5.1. Testes Nativos C/C++ (`tools/run_tests.sh`)
**Resultado: 45 de 45 testes APROVADOS (100% de sucesso).**
- `mp3-test`, `videodec-test`, `services-test`, `pad-map-test` (PR #48 validado), `audio-downmix-test`, `cpu-check-test`, `isa-emulate-test`, `isa-stub-test`, `tsc-clock-test`.
- `pipeline-binary-cache-test`, `pipeline-binary-initialization-test`, `motion-history-test`, `motion-shader-test`, `buffer-addr64-test`, `buffer-inline-test`, `png-decode-test`.
- `aliasing-test`, `blend-rewrite-test` (PR #58 validado), `depth-stencil-state-test`, `async-compiler-test` (PR #56 validado), `warmup-inbox-test`, `wave-stats-test`, `pass-census-test`.
- `frame-capture-test`, `lighting-test-knob-test`, `amd-paths-test`, `no-msaa-test`, `async-pipeline-cache-test`, `async-fetch-snapshot-test`.
- `ui-composition-test`, `upscaler-support-test` (PR #59 e #61 validados), `perf-overlay-test`, `readback-queue-test`, `wait-diagnostics-test`.
- `graphics-settings-test` (PR #49 validado), `readback-sources-test`, `page-state-test`, `scene-resolution-test`, `taa-shader-test`, `camera-motion-test`, `dma-barriers-test`.
- `renderpass-barrier-test`, `shader-resource-test`, `format-remap-test`, `multiwave-barriers-test`.
- `content-test` (runtime C isolado com addons).

### 5.2. Testes Python
- `tests/test_run_language.py` (PR #50): 6 testes aprovados.
- `tests/test_launcher_upscale.py` (PR #59): 5 testes aprovados.
- `tests/test_launcher_controls.py` (PR #48 e #59): 27 testes aprovados.
- `tests/test_probe.py`: 18 testes executados com sucesso com o runtime no PATH.
- `test_graphics_options.py`: Aprovado.
- *Nota sobre ambiente:* Skips normais para testes dependentes de eboot proprietária mockada e widgets Tkinter sem display headless.

### 5.3. Verificações de Runtime e Inicialização
- **Launcher:** Inicializa corretamente (`Launcher imported successfully, version: 0.7.6-rc.1`).
- **Bootstrap do Jogo:** `python run.py --help` executou normalmente:
  - CUSA00035 01.02 identificada.
  - Backup automático dos saves realizado com sucesso (`user/savedata_backups/20261021-172409`).
  - Patches de 60 FPS aplicados.
  - Resolução de idioma corrigida (`Language: PS4 id 17 -> engine id 17`).
- **Sonda Vulkan:** `theorder-gpu-capabilities.exe` detectou a GPU do usuário:  
  `GPU scene scaling: NVIDIA GeForce RTX 5060 Laptop GPU supports live presets`.

---

## 6. Alterações que Exigem Testes Manuais

1. **Pause Menu no Teclado (PR #48):** Pressionar `Enter` durante o gameplay para checar abertura do menu.
2. **Resolução e Aspect Ratio (PR #49 & #59):** Seleção de 32:9 e 4K nativo no launcher/menu.
3. **Persistência de Idioma PT-BR (PR #50):** Inicialização sem parâmetros pelo `run.py`.
4. **Sensibilidade do Mouse (PR #51):** Ajustar para 400% e verificar velocidade de câmera.
5. **Opção de 120 FPS (PR #53):** Verificar se aparece no menu e se funciona com taxa de atualização adequada.
6. **Stuttering e Pipelines Inativos (PR #54):** Checar se houve redução de engasgos e quedas bruscas de frame.
7. **Minimização de Tela (Alt+Tab) (PR #55):** Minimizar repetidamente sem ocorrência de crash.
8. **Uso de CPU após longo período (PR #56):** Deixar rodando por 20-30 minutos para avaliar CPU load.
9. **Sombras do Sol (PR #57):** Checar sombras em áreas ensolaradas nos Capítulos 1 e 3.
10. **Exposição dos Personagens em NVIDIA (PR #58):** Conferir iluminação de pele e roupas escuras.
11. **Texturas e Polígonos Pretos (PR #60):** Avaliar flickering de texturas.
12. **Reseleção de DLSS (PR #61):** Selecionar DLSS após fallback no menu in-game.
13. **Flares Coloridos no Capítulo 5 (PR #62):** Avaliar skybox e luzes volumétricas.

---

## 7. Bugs Conhecidos que Ainda Não Possuem Correção

- **Issue #42:** Suporte completo a DLSS 4.5 e Frame Generation.
- **Issue #46:** Suporte à descompressão de arquivos de áudio/assets ZAR.
- **Issue #11:** Crash ao atingir o Capítulo 9.
- **Issue #10:** Silhuetas pretas em iGPUs Intel HD Graphics 630 (o PR #55 corrigiu apenas o crash de Alt+Tab/swapchain, a emulação de formatos na Intel integrada permanece pendente).
- **Issue #1:** Falhas de textura em placas Intel Arc.

---

## 8. Possíveis Regressões a Monitorar

- **Consumo de VRAM:** Selecionar resolução 4K nativa (PR #59) eleva o uso de VRAM consideravelmente em comparação com o padrão 1080p.
- **Sensibilidade Excessiva:** O slider em 400% (PR #51) com mouses de alto DPI pode tornar a rotação de câmera incontrolável se não houver ajuste de DPI no mouse.
- **Swapchain sob Minimização:** Garantir que janelas sem foco (mas não minimizadas) continuem renderizando normalmente.
