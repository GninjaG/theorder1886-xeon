# The Order: 1886 PC Port — Release windows-v0.7.6-rc.1

This release unifies the two development lines onto a single `master` branch — every v0.7.4/v0.7.5 feature is preserved — and adds a renderer flicker **root-cause investigation** with targeted synchronization fixes, stable diagnostic IDs, and a regression test harness. The flicker fixes target confirmed GPU hazards and are backed by red/green regression tests; **in-game visual validation of #29/#30/#39 and AMD wave32 behavior is still pending real-hardware capture and is not claimed as resolved**.

---

### Highlights & Key Improvements

#### 1. Render-Pass Barrier Hazard — Confirmed Root Cause (Issues #29, #30)
* Fixed an illegal `vkCmdPipelineBarrier2` buffer barrier recorded *inside* an open dynamic-rendering instance in `gpu/shadps4/video_core/buffer_cache/buffer_cache.cpp` (`VUID-vkCmdPipelineBarrier2-None-09553`/`09554`). The render pass now ends before the pre-transfer barrier, restoring producer/consumer ordering so transfers no longer observe stale buffer data. Proven red/green by the new `renderpass-barrier` regression test.

#### 2. Depth-Store Write-After-Write Hazard — Confirmed Root Cause (Issues #29, #30, #26)
* Promoted the barrier source scope for depth images in `Image::GetBarriers` (`gpu/shadps4/video_core/texture_cache/image.cpp`) to include `LATE/EARLY_FRAGMENT_TESTS` + `DEPTH_STENCIL_ATTACHMENT_WRITE`, so a depth layout transition now waits on the depth store instead of racing it (`SYNC-HAZARD-WRITE-AFTER-WRITE`). Color-image barriers are unchanged.

#### 3. Pending-Pipeline Draws No Longer Silently Dropped — Confirmed Behavior (Issue #30, frame spikes in #29)
* Added a key-scoped `PipelineCache::WaitGraphics(key)` so a visible draw whose pipeline is still compiling waits on exactly **its own** compile rather than being silently skipped. This trades object pop-in/out for a bounded compilation stall; no effect is disabled and no cache is cleared.

#### 4. Diagnostic Stable IDs & Regression Harness
* Every image/buffer/frame/draw/dispatch now carries a stable diagnostic ID so a Vulkan validation or synchronization message self-identifies the exact resource and draw. New regression tests (`renderpass-barrier`, `format-remap`, `multiwave-barriers`) are registered in the test suite and demonstrated red-without-fix / green-with-fix.

#### 5. Everything from v0.7.5 — Preserved
* Sun shadow fixes (#26), 32:9 Super Ultrawide (#27), European Portuguese fallback (#28), AMD RX 9000 normal-map format fallback (#33), official DLC via `-usedlc` (#16), 60/120 FPS, FSR 3.1/FSR 4, and the full launcher reset system are all carried over intact.
* The v0.7.5 multiwave LDS-barrier default is kept, with `THEORDER_LDS_BARRIERS_MULTIWAVE` exposed as an explicit diagnostic override (`=0` off, `=1` on, unset = v0.7.5 default).

---

### What Is Still Pending Verification

* No in-game before/after capture yet confirms that #29, #30 or #39 is visually resolved. The confirmed fixes above are validated by synchronization-validation tests, not by gameplay footage.
* AMD / Chapter-5 behavior and the signed-format (#39) fallback need device-specific runtime evidence on AMD hardware; visual correctness must not be inferred from compile or logic tests.
* No FPS improvement, full-campaign stability, or issue closure is claimed by this build.

---

### Package Contents
- `The Order 1886.exe`: Standalone graphical launcher with configuration manager, DLC management, control mapping, and graphics options.
- `Play The Order 1886.exe`: Fast-launch runner executable.
- `bin/`: Optimized 64-bit runtime binaries (`theorder-probe.exe`, `theorder-gpu-capabilities.exe`) with bundled MSYS2/MinGW Clang64 dependencies.
- `patches/`: Community and widescreen patches including 21:9 and 32:9 ultrawide, 60+/120 FPS unlock, and letterbox removal.
- `scripts/`: Python orchestration utilities and configuration scripts.

---

### How to Play
1. Extract `theorder1886-windows.zip` into a **new** folder (keep any stable install intact).
2. Run `Play The Order 1886.exe` (or `The Order 1886.exe`).
3. Select your decrypted game folder (`CUSA00035` updated to v1.02).
4. Use **copies** of your saves/settings when testing this build.

---

## O que mudou na v0.7.6-rc.1 (Português)

Esta versão unifica as duas linhas de desenvolvimento em um único branch `master` — todos os recursos da v0.7.4/v0.7.5 foram preservados — e adiciona uma **investigação de causa-raiz** do flicker do renderer, com correções de sincronização direcionadas, IDs de diagnóstico estáveis e testes de regressão. As correções de flicker atacam hazards de GPU confirmados e têm testes red/green; **a validação visual em gameplay de #29/#30/#39 e o comportamento em AMD wave32 ainda dependem de captura em hardware real e não são declarados como resolvidos**.

### Destaques
1. **Barreira dentro do render pass — causa-raiz confirmada (#29, #30):** corrigida barreira ilegal de buffer emitida dentro de um passe de dynamic-rendering no buffer cache (VUIDs 09553/09554); o render pass agora encerra antes da barreira, eliminando leitura de dados obsoletos. Teste `renderpass-barrier` prova red/green.
2. **Write-after-write no depth — causa-raiz confirmada (#29, #30, #26):** a transição de layout de profundidade agora espera o depth-store (escopo `LATE/EARLY_FRAGMENT_TESTS` + `DEPTH_WRITE` em `Image::GetBarriers`), eliminando o hazard de profundidade instável. Cor inalterada.
3. **Draws com pipeline compilando não somem mais — comportamento confirmado (#30, picos da #29):** novo `WaitGraphics(key)` faz o draw visível esperar o **próprio** compile em vez de ser descartado silenciosamente. Troca o "pop-in/out" por um stall limitado; nenhum efeito foi desativado e nenhum cache foi limpo.
4. **IDs de diagnóstico estáveis e harness de regressão:** cada image/buffer/frame/draw/dispatch tem ID estável, então mensagens de validação/sync do Vulkan identificam o recurso e o draw exatos. Novos testes (`renderpass-barrier`, `format-remap`, `multiwave-barriers`) registrados na suíte.
5. **Tudo da v0.7.5 preservado:** sombras do sol (#26), 32:9 (#27), fallback PT-PT (#28), fallback de formato AMD RX 9000 (#33), DLC via `-usedlc` (#16), 60/120 FPS, FSR 3.1/FSR 4 e o sistema de reset do launcher. O default multiwave do v0.7.5 foi mantido, com `THEORDER_LDS_BARRIERS_MULTIWAVE` como override de diagnóstico.

### Ainda pendente de verificação
- Nenhuma captura antes/depois em gameplay confirma ainda que #29, #30 ou #39 está visualmente resolvido. As correções confirmadas acima são validadas por testes de synchronization validation, não por footage de jogo.
- Comportamento em AMD / Chapter 5 e o fallback de formato com sinal (#39) precisam de evidência em hardware AMD. Não se deve inferir correção visual a partir de testes de compilação ou lógica.
- Esta build não declara ganho de FPS, estabilidade de campanha completa nem fechamento de issues.

### Como Jogar
1. Extraia `theorder1886-windows.zip` em uma pasta **nova** (preserve a instalação estável).
2. Execute `Play The Order 1886.exe` (ou `The Order 1886.exe`).
3. Selecione a pasta do jogo descriptografado (`CUSA00035` atualizado para v1.02).
4. Use **cópias** dos seus saves/configurações ao testar esta build.

---

### ☕ Support the Project / Apoie o Projeto

If you are enjoying the port and want to support continued development, consider buying me a coffee:

[![Support on Ko-fi](https://img.shields.io/badge/Ko--fi-Buy%20Me%20a%20Coffee-ff5e5b?style=for-the-badge&logo=ko-fi&logoColor=white)](https://ko-fi.com/mateusmeneses)

👉 **[https://ko-fi.com/mateusmeneses](https://ko-fi.com/mateusmeneses)** ☕❤️
