# The Order: 1886 PC Port - Windows v0.7.9

This release makes the renderer's supported ultrawide output targets selectable,
corrects warm-up controls that did not reach the active on-demand paths, and
clarifies what output resolution and upscaler settings actually control. It does
not claim a stutter reduction, internal 4K rendering, or visual parity with the
PS4; those require matched gameplay captures.

## Highlights

1. **Ultrawide output selection (#27):** `3440x1440` was accepted by the
   launcher but discarded by the C++ settings parser; `3840x1080` was missing
   from the launcher and overlay. Both are now added to the runtime output
   table, and every supported output mode is selectable in the launcher and
   in-game menu. Existing numeric indices remain stable. Synthetic tests verify
   parsing, save/load, selector mapping and launcher persistence. The 32:9
   reporter still needs to verify framing, HUD and sharpness on the actual
   monitor; #27 remains open.
2. **Resolution and upscaler reporting:** the launcher distinguishes the
   host/window output target from the monitor's desktop mode, internal scene
   raster and aspect ratio. The in-game Image page already reports render
   resolution, target output, window size and active provider. Native AA (1.0x)
   does not spatially upscale. Unsupported FSR 4/4.1.1 or DLSS may fall back to
   FSR 3.1; check the active provider before comparing images. This does not
   claim that issue #35's internal-4K request or visual upscaler report is
   resolved.
3. **Warm-up controls:** disabling background warm-up now also stops recent-
   session hot-list warm-up in on-demand mode. The parallel option selects one
   or several workers for background cache builds. Unit tests cover the
   environment mapping and worker-count policy. No performance gain is claimed;
   a matched first-visit/warm-revisit gameplay comparison is still required.

## Validation And Known Limits

- Windows CLANG64 build passed. All 48 C/C++ suites passed. Python ran 316 tests
  with 39 skips and no failures.
- Output settings are validated synthetically; no 21:9/32:9 monitor or
  same-camera PS4 capture was available for visual sign-off.
- The FSR/DLSS code has separate dispatch paths and explicit capability
  fallbacks. This validation does not establish image quality, ghosting, correct
  HUD/subtitle composition or a performance improvement on every GPU.
- First-visit stutters, flicker, lighting/shadows, AMD/Intel compatibility,
  dialogue subtitles and full-campaign stability remain open. No waits or draws
  were removed to improve benchmark numbers; stable 60/120 FPS is not promised.

For a useful report, include game SKU/version, chapter/checkpoint, camera, GPU
and driver, selected aspect/output/upscaler/preset, the in-game Image status and
a matching screenshot. For stutters, compare the same route with cache state
recorded. Redact private paths from logs.

## Package Contents

The Windows ZIP contains the launcher, direct-play launcher, native runtime,
required redistributable DLLs, preparation scripts and patch definitions. It
contains no game dump, saves, player-generated caches, private logs or
proprietary DLSS runtime. Supply your own legally obtained game dump.

## How To Play

1. Extract `theorder1886-windows.zip` into a new folder.
2. Keep the previous install and back up saves/settings. Do not clear shader
   caches as a routine update step.
3. Open `The Order 1886.exe`, select your game dump and launch.
4. Check the in-game Image page for actual render/output/provider values. An
   output target is not a request to change the monitor's desktop mode.

## Portugues

1. **Selecao ultrawide (#27):** `3440x1440` era aceito pelo launcher, mas
   descartado pelo parser C++; `3840x1080` nao aparecia no launcher nem no
   overlay. Os dois modos agora estao na tabela do runtime e nos seletores.
   Testes sinteticos cobrem parse, persistencia e selecao; ainda falta validar
   enquadramento, HUD e nitidez no monitor do reporter. A issue continua aberta.
2. **Resolucao e upscaler:** o launcher agora explica que output/window target,
   modo desktop, raster interno e proporcao sao coisas diferentes. O menu Image
   informa raster, target, tamanho da janela e upscaler ativo. Native AA (1.0x)
   nao reduz o raster. Se o hardware/assets nao suportarem FSR 4/4.1.1 ou DLSS,
   pode ocorrer fallback para FSR 3.1. A issue #35 sobre resolucao interna 4K e
   diferenca visual do upscaler nao foi considerada resolvida.
3. **Warm-up:** desligar o warm-up em background agora tambem para a lista de
   pipelines recentes no modo on-demand. A opcao paralela escolhe um ou varios
   workers. Os testes cobrem o mapeamento e a politica de workers; nao ha ganho
   de desempenho comprovado sem A/B de gameplay equivalente.

Build Windows CLANG64 e 48 suites C/C++ passaram. A suite Python executou 316
 testes, com 39 skips e nenhuma falha. A configuracao de saida foi validada por
testes sinteticos, sem monitor ultrawide ou captura PS4 equivalente. Stutters,
flicker, iluminacao/sombras, compatibilidade AMD/Intel, legendas e estabilidade
da campanha continuam pendentes; nao ha promessa de 60/120 FPS estaveis.

Extraia em uma pasta nova, mantenha backup de saves/configuracoes e nao limpe os
caches como procedimento de atualizacao. O ZIP nao inclui arquivos do jogo.

## Optional Support / Apoio Opcional

Development support is optional and does not unlock features. / Apoiar e
opcional e nao desbloqueia funcionalidades.

[Support on Ko-fi / Apoiar no Ko-fi](https://ko-fi.com/mateusmeneses)
