# Checklist de Validação Manual em Gameplay (v0.7.6 Estabilizada)

**Branch de Validação:** `stabilization/v0.7.6-fixes`  
**Base:** `integration/prs-batch-20261021` + Correções de Estabilização Pós-Batch  
**Ambiente Principal do Usuário:** GPU NVIDIA GeForce RTX, Windows 11  
**Instruções de Preenchimento:** Marque com um `[X]` o status de cada teste após a execução no jogo.

---

## 1. Novos Testes de Estabilização Pós-Batch (v0.7.6)

### TESTE A — Conflito entre Mouse, Teclado e Menu Overlay (`Insert` / `R3+L2`)
- **Problema Abordado:** Teclas `Q`/`E` pulavam abas indevidamente; `Espaço` disparava botões; `Escape` fechava menu mesmo com menus suspensos abertos; fechar o menu enquanto segurava teclas ou botões do mouse fazia a arma disparar ou o personagem andar sozinho.
- **Condição / Local:** Durante o jogo ou menu inicial.
- **Passos para Testar:**
  1. Durante o gameplay, pressione a tecla `Insert` para abrir o menu do port.
  2. Mova o mouse livremente pelo menu sem tocar no teclado e selecione opções apenas clicando com o mouse.
  3. Com o menu aberto, pressione as teclas `Q`, `E` e `Espaço`.
  4. Abra um menu suspenso (dropdown, ex.: *Output Resolution* ou *Upscaler*) e pressione `Escape`.
  5. Enquanto segura o botão esquerdo do mouse (LMB) ou a tecla `W`, pressione `Insert` para fechar o overlay e em seguida solte o botão/tecla.
- **Critérios de Aprovação:**
  - As teclas `Q` e `E` **NÃO** devem mudar de aba (abas só trocam por clique de mouse, `Ctrl+Tab` ou `PageUp/Down`).
  - A tecla `Espaço` **NÃO** deve acionar botões ou checkboxes aleatórios no menu.
  - O primeiro `Escape` deve apenas fechar o menu suspenso aberto, sem fechar o overlay inteiro.
  - Ao fechar o menu com `Insert`, a arma do personagem **NÃO** deve disparar e o personagem **NÃO** deve andar sozinho sem que a tecla seja pressionada novamente.
- **Status do Teste:**
  - [ ] **PASSOU**
  - [ ] **FALHOU**
  - [ ] **NÃO TESTADO**

---

### TESTE B — Eliminação de Microtravamentos (Stutters) em Combates e Gameplay
- **Problema Abordado:** Picos severos de frametime e microstutters ao girar a câmera ou entrar em combates com iluminação dinâmica (gerados pelo hashing excessivo em `vk_pipeline_cache`).
- **Condição / Local:** Capítulo 1 (beco de Whitechapel, tiroteio e fuga na carruagem) ou qualquer área de combate.
- **Passos para Testar:**
  1. Inicie o jogo e avance até um combate com inimigos disparando armas e fumaça na tela.
  2. Gire a câmera rapidamente em 360 graus enquanto se movimenta e atira.
  3. Mantenha um medidor de FPS/Frametime visível (ex.: MSI Afterburner / RTSS ou overlay da GPU).
- **Critérios de Aprovação:**
  - O frametime deve se manter estável sem travamentos súbitos periódicos de 50ms a 100ms.
  - O uso de CPU não deve ficar travado em 100% durante o tiroteio.
  - A rotação de câmera em áreas abertas deve permanecer suave e sem soluços de renderização.
- **Status do Teste:**
  - [ ] **PASSOU**
  - [ ] **FALHOU**
  - [ ] **NÃO TESTADO**

---

### TESTE C — Resposta Suave e Precisa da Câmera no Mouse (1:1 Raw Look)
- **Problema Abordado:** Câmera do mouse pesada, lenta, com sensação de "flutuação no gelo" ao parar e deadzone que ignorava micromovimentos.
- **Condição / Local:** Gameplay a pé com controle livre da câmera.
- **Passos para Testar:**
  1. Abra o menu in-game (`Insert`) e vá para a aba **Controls**.
  2. Teste os novos sliders:
     - **Horizontal Sensitivity (X)**: ajuste entre 50% e 200%.
     - **Vertical Sensitivity (Y)**: ajuste individualmente.
     - **Invert Y-Axis**: marque e desmarque a opção.
  3. Volte ao jogo e faça micromovimentos sutis com o mouse (movimento de poucos milímetros).
  4. Realize movimentos rápidos e secos de parada (flicks) para a esquerda e para a direita.
- **Critérios de Aprovação:**
  - Micromovimentos devem responder imediatamente, sem zona morta que impeça pequenos ajustes de mira.
  - Ao parar a mão bruscamente, a câmera deve parar instantaneamente, sem continuar deslizando por vários frames.
  - Flicks rápidos devem girar a câmera proporcionalmente à distância percorrida pelo mouse na mesa.
  - As opções de sensibilidade independente de X, Y e inversão de Y devem funcionar como configurado.
- **Status do Teste:**
  - [ ] **PASSOU**
  - [ ] **FALHOU**
  - [ ] **NÃO TESTADO**

---

### TESTE D — Seleção e Funcionamento dos Upscalers (DLSS, FSR 3.1, FSR 4, FSR 4.1.1, TAA)
- **Problema Abordado:** Opções de upscaler estavam bloqueadas/desativadas no launcher e in-game; DLSS não permitia nova tentativa após falha inicial.
- **Condição / Local:** Launcher (`port_launcher.exe`) e Menu In-Game (`Insert` -> aba *Image*).
- **Passos para Testar:**
  1. Abra o launcher do port e verifique se o dropdown **Temporal Upscaler** está ativo com as opções: `DLSS`, `FSR 3.1`, `FSR 4`, `FSR 4.1.1`, `TAA` e `Off`.
  2. Inicie o jogo com `DLSS` selecionado (em GPU NVIDIA RTX).
  3. No menu in-game (`Insert` -> aba *Image*), altere entre `DLSS`, `FSR 3.1` e `TAA`.
  4. Caso o DLSS reverta por qualquer motivo, selecione `DLSS` novamente no dropdown do menu.
- **Critérios de Aprovação:**
  - As opções de upscaling estão totalmente acessíveis tanto no launcher quanto no overlay.
  - Ao alternar para DLSS em placa NVIDIA suportada, o pipeline NGX é inicializado corretamente.
  - Ao selecionar novamente o DLSS após erro temporário, o sistema tenta reinicializar sem travar a renderização.
  - A imagem permanece nítida sem artefatos severos de desfoque ou ghosting excessivo.
- **Status do Teste:**
  - [ ] **PASSOU**
  - [ ] **FALHOU**
  - [ ] **NÃO TESTADO**

---

### TESTE E — Resoluções Nativas de Saída até 4K (720p, 1080p, 1440p, 4K, 32:9)
- **Problema Abordado:** Resolução de saída ficava presa em 1080p, ignorando a seleção de 1440p ou 4K.
- **Condição / Local:** Launcher e Menu In-Game (`Insert` -> aba *Image*).
- **Passos para Testar:**
  1. No launcher, na aba **Graphics**, selecione **Native output resolution**: `3840 × 2160 (4K)` (ou `2560 × 1440`).
  2. Inicie o jogo e confira o tamanho da janela e a nitidez do HUD e da geometria.
  3. Abra o menu in-game (`Insert` -> aba *Image*) e verifique o valor exibido em **Output Resolution**.
  4. Alterne a resolução pelo menu in-game e observe a adaptação da apresentação.
- **Critérios de Aprovação:**
  - O jogo inicia com a janela e o swapchain Vulkan criados nas dimensões reais da resolução configurada (ex.: 3840x2160 ou 2560x1440).
  - Em monitores 1440p ou 4K, a imagem não apresenta borrão de upscale linear simples de 1080p para a tela.
  - A opção selecionada é salva e recarregada corretamente do `theorder.ini`.
- **Status do Teste:**
  - [ ] **PASSOU**
  - [ ] **FALHOU**
  - [ ] **NÃO TESTADO**

---

## 2. Testes de PRs Anteriores (Histórico Consolidado)

### PR #48 — Mapeamento da Tecla Enter para o Menu de Pausa
- **Resultado Anterior:** **PASSOU**
- **Status Atual:**
  - [X] **PASSOU**

---

### PR #50 — Persistência do Idioma Português-Brasil (PT-BR)
- **Resultado Anterior:** N/A (Não validado conclusivamente)
- **Passos para Testar:**
  1. No launcher, selecione o idioma Português (Brasil) e salve.
  2. Execute o jogo via `run.bat` ou `python run.py`.
- **Status Atual:**
  - [ ] **PASSOU**
  - [ ] **FALHOU**
  - [ ] **NÃO TESTADO**

---

### PR #51 — Sensibilidade Máxima do Mouse (Slider até 400%)
- **Resultado Anterior:** **PASSOU** (Slider agora complementado com o TESTE C para calibração de resposta real).
- **Status Atual:**
  - [X] **PASSOU**

---

### PR #53 — Opção de 120 FPS no Menu In-Game
- **Resultado Anterior:** **PASSOU**
- **Status Atual:**
  - [X] **PASSOU**

---

### PR #55 — Minimização de Janela (Alt+Tab) sem Falha de Apresentação
- **Resultado Anterior:** **PASSOU**
- **Status Atual:**
  - [X] **PASSOU**

---

### PR #56 — Aposentadoria de Falhas Gráficas e Estabilidade de CPU
- **Resultado Anterior:** **PASSOU** (Complementado pelo TESTE B que removeu o custo de hash no hot path).
- **Status Atual:**
  - [X] **PASSOU**

---

### PR #57 — Sombras do Sol e Metadados HTILE
- **Resultado Anterior:** N/A (Não validado conclusivamente)
- **Passos para Testar:** Observar sombras projetadas em áreas externas ensolaradas (Capítulo 1 e terraço do Capítulo 3).
- **Status Atual:**
  - [ ] **PASSOU**
  - [ ] **FALHOU**
  - [ ] **NÃO TESTADO**

---

### PR #58 — Exposição de Personagens e Níveis de Preto (NVIDIA)
- **Resultado Anterior:** N/A (Não validado conclusivamente)
- **Passos para Testar:** Verificar cenas internas com lâmpadas e áreas escuras (sem rostos com superexposição branca).
- **Status Atual:**
  - [ ] **PASSOU**
  - [ ] **FALHOU**
  - [ ] **NÃO TESTADO**

---

### PR #60 — Eliminação de Polígonos Pretos e Flickering de Textura
- **Resultado Anterior:** **PASSOU**
- **Status Atual:**
  - [X] **PASSOU**

---

### PR #62 — Flares Verdes e Azuis (Cores de Borda Customizadas)
- **Resultado Anterior:** **PASSOU**
- **Status Atual:**
  - [X] **PASSOU**

---

### PR #49 — Proporção de Tela 32:9 e Resolução 5120x1440
- **Resultado Anterior:** N/A (Depende de monitor ultrawide 32:9)
- **Status Atual:**
  - [ ] **PASSOU**
  - [ ] **FALHOU**
  - [ ] **NÃO TESTADO**

---

### PR #52 — Empacotamento Linux (Nix/AppImage)
- **Status:** [X] **PENDENTE (Requer ambiente Linux / Steam Deck)**

### Hardware Específico (Intel HD 630 / Arc)
- **Status:** [X] **PENDENTE (Requer hardware Intel específico)**
