# The Order: 1886 PC Port — Mouse Input & Camera Aim Rework

**Data:** 22 de Outubro de 2026  
**Subsistema:** `src/runtime_pad.c`, `src/mouse_stick.h`, `tests/test_mouse_stick.c`, `tests/test_pad_map.c`  
**Objetivo:** Eliminar o comportamento de emulação de analógico e prover resposta 1:1 de câmera para mouse e teclado.

---

## 1. Problema Diagnosticado

Nas versões anteriores (incluindo v0.7.6 e PRs #51/#52), a conversão de entrada do mouse em movimentos de câmera utilizava a seguinte função em `src/runtime_pad.c`:
```c
static double mouse_impulse_x, mouse_impulse_y;
static void mouse_to_stick(float dx, float dy, float speed_x, float speed_y, int *out_x, int *out_y) {
    ...
    if (mag > (MAX_DEFLECTION - DEADZONE)) {
        double excess = mag - (MAX_DEFLECTION - DEADZONE);
        mouse_impulse_x = dir_x * excess * 0.75;
        mouse_impulse_y = dir_y * excess * 0.75;
        ...
    }
}
```

### Principais falhas observadas:
1. **Inércia Residual Artificial (`mouse_impulse`):**
   Ao fazer uma movimentação rápida ("flick") com o mouse, o excesso de deflexão era armazenado em `mouse_impulse_x/y` e distribuído nos frames subsequentes com amortecimento de 0.75. Isso causava uma sensação de "arraste" ou "flutuação" ("floatiness"), fazendo a câmera continuar girando por 2 a 4 frames após a mão do jogador já ter parado completamente.
2. **Degrau Súbito na Deadzone (Jerk):**
   Qualquer movimento mínimo do mouse adicionava diretamente `DEADZONE (24.0)` à magnitude, gerando um salto brusco de posição que destruía a precisão da micro-mira (sniping).
3. **Dependência de Taxa de Quadros:**
   Em 30 FPS vs 60 FPS vs 120 FPS, as amostras chegavam com deltas distintos, alterando a sensibilidade percebida.

---

## 2. Solução Implementada

Inspirada no modelo de física vetorial e anti-deadzone do `iss_pc` (`mouse_stick.h`), a rotina `mouse_to_stick` foi completamente reformulada:

1. **Remoção Absoluta de Inércia:**
   Eliminados `mouse_impulse_x` e `mouse_impulse_y`. No exato milissegundo em que o mouse para (`dx == 0, dy == 0`), a deflexão de rotação vai imediatamente para `0` (Zero Drift / Instant Rest State).
2. **Elevação Suave da Deadzone (Anti-Deadzone Exponencial):**
   ```c
   float normalized = fminf(mag / (MAX_DEFLECTION - DEADZONE), 1.0f);
   normalized = powf(normalized, 0.90f);
   float stick_mag = DEADZONE + (MAX_DEFLECTION - DEADZONE) * normalized;
   ```
   Movimentos sutis ultrapassam a barreira da deadzone do motor (~24/128) de forma gradual e contínua, permitindo micro-ajustes finos de mira sem sobressaltos.
3. **Sensibilidade Independente X e Y:**
   Escalamento angular separado para movimentos horizontais e verticais, permitindo conforto tanto em monitores widescreen quanto ultrawide (16:9, 21:9, 32:9).
4. **Tratamento de Sub-Ruído:**
   Deltas menores que `0.0001f` são suprimidos para evitar micro-tremores de sensores ópticos em repouso.

---

## 3. Testes Automatizados e Validação

Dois conjuntos de testes foram executados com sucesso:
- `out/test_mouse_stick`: Valida velocidade linear, aceleração, compensação de deadzone e inversão de eixo Y.
- `out/gpu/pad-map-test.exe`: Valida mapeamento de botões, neutralização de analógicos após fechamento do menu/overlay (`hold_after_capture`), bypass instantâneo de ruído e separação de sensibilidade X/Y.
- Ambos os testes executam em **menos de 1 segundo** sem blocos interativos.
