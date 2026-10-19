# Backlog de Desenvolvimento — The Order: 1886 PC Port

Este documento consolida o planejamento estratégico de engenharia reversa, otimizações de engine e novas funcionalidades para as próximas versões do port.

---

## 1. Otimizações de Renderização & Vulkan Backend

### 1.1 Pipeline State Object (PSO) Pre-Warming & Caching Avançado
- **Objetivo**: Eliminar completamente micro-stutters durante a transição de zonas e carregamento dinâmico de shaders de iluminação.
- **Implementação**:
  - Implementar pipeline compilation thread pool persistente desacoplado do scheduler principal.
  - Implementar hashing unificado de SPIR-V com desduplicação antecipada de pipelines equivalentes de forward/deferred lighting.

### 1.2 Otimizações de VRAM & Gerenciamento de Buffers
- **Objetivo**: Reduzir ainda mais a alocação de pico de memória de vídeo para permitir placas de entrada (4 GB VRAM) rodarem a 1080p sem risco de OOM.
- **Implementação**:
  - Aliasing de render targets temporários pós-VSM e pós-bloom.
  - Compactação de buffers de geometria intermediários (`VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT` com sub-alocador customizado estilo VMA).

### 1.3 Calibração Completa do FSR 3.1 / FSR 4 & DLSS
- **Objetivo**: Ajuste de matrizes de projeção reversa, jittering sub-pixel e vetores de movimento para o upscaler temporal FSR 3.1 / FSR 4 e DLSS.
- **Implementação**:
  - Integração de vetores de câmera (camera jitter cancellation) diretamente nos compute shaders de pré-pass do FSR.
  - Ajuste de Reactive Mask e Composition Mask para evitar fantasmas (ghosting) nas roupas de lã e bordas dos personagens durante movimentos rápidos.

---

## 2. Física, Simulação & CPU Engine

### 2.1 Otimização de Simulação de Tecido (Cloth Physics)
- **Objetivo**: Aumentar a taxa de quadros em cenas com muitos personagens (fardas, capas e casacos de tecido pesado).
- **Implementação**:
  - Investigar dispatch de computação de física de vértices no Vulkan em vez de emulação por CPU guest.
  - Adicionar controle de LOD de simulação de tecidos no menu in-game / launcher para CPUs de menor contagem de núcleos.

### 2.2 Sincronização e Pacing de Quadros a 120+ FPS
- **Objetivo**: Garantir frametime estável e frame-pacing perfeito em telas de alta taxa de atualização (120 Hz, 144 Hz, 165 Hz, 240 Hz).
- **Implementação**:
  - Implementar `VK_KHR_present_wait` e `VK_GOOGLE_display_timing` para presentation clock de alta precisão.
  - Refinar o clamp do delta time do motor de física para evitar aceleração de cutscenes quando rodando a taxas acima de 60 FPS.

---

## 3. Áudio, Input & Usabilidade

### 3.1 Rebinding Dinâmico in-game (Overlay)
- **Objetivo**: Permitir remapear teclas de teclado/mouse e botões de gamepad diretamente pelo menu suspenso (`Insert` / `R3+L2`), sem precisar fechar o jogo e reabrir o launcher.
- **Implementação**:
  - Interceptar eventos SDL no loop do overlay e atualizar `THEORDER_KEY_MAP` e `THEORDER_PAD_MAP` em tempo de execução.

### 3.2 Suporte a Áudio Espacial 3D & 7.1 Surround
- **Objetivo**: Mapeamento nativo dos canais de áudio multicanal do PS4 (7.1 PCM) para endpoints estéreo/surround do Windows (WASAPI / XAudio2 via SDL3).
- **Implementação**:
  - Configuração de downmix HRTF com atenuação de distância configurável.

### 3.3 Interface HUD Adaptativa para Ultrawide (21:9 e 32:9)
- **Objetivo**: Opção para manter os elementos de HUD ancorados nos cantos da tela ou centralizados na área 16:9 tradicional.
- **Implementação**:
  - Hook nos comandos de desenho de UI guest (`DrawTexturedQuad` / ortho 2D) com offset configurável pelo usuário.

---

## 4. Ferramentas, Mods & Comunidade

### 4.1 Gerenciador de Mods com Visualizador de Arquivos WAD
- **Objetivo**: Facilitar a instalação de texturas HD, mods de dublagem e substituição de modelos 3D.
- **Implementação**:
  - Adicionar no Launcher uma visualização dos arquivos dentro dos pacotes `.wad`.
  - Sistema de resolução de conflitos de arquivos soltos com prioridade visual.

### 4.2 Auto-Update & Integrador Steam Deck / Bazzite
- **Objetivo**: Instalação e atualização em 1 clique para usuários de Steam Deck e Linux gaming handhelds.
- **Implementação**:
  - Script de pós-instalação para detecção de Proton / Steam Flatpak e configuração de Gamepad SDL com layouts de botões Steam Deck pré-mapeados.

---

## 5. Histórico e Prioridades para os Próximos Ciclos

| Prioridade | Funcionalidade / Otimização | Módulo Principal | Ciclo Alvo |
| :---: | :--- | :--- | :---: |
| 🔴 **Alta** | Calibração de Jittering e Reactive Mask para FSR 3.1 | `gpu/shadps4/video_core` | v0.7.5 |
| 🔴 **Alta** | Rebinding in-game de botões no menu suspenso | `gpu/shim/port_overlay` | v0.7.5 |
| 🟡 **Média** | Otimização de Cloth Physics em multithreading | `gpu/shadps4/core` | v0.7.6 |
| 🟡 **Média** | Ancoragem de HUD em Ultrawide 21:9 / 32:9 | `scripts/hud_layout.py` | v0.7.6 |
| 🟢 **Baixa** | Visualizador de WADs e Mod Manager avançado | `launcher/` | v0.8.0 |
| 🟢 **Baixa** | Integração dedicada para Steam Deck / Flatpak | `packaging/` | v0.8.0 |
