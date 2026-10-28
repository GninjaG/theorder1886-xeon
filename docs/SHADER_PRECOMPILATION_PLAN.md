# The Order: 1886 PC Port — Shader Precompilation & Driver Pipeline Cache Plan

> Superseded by `SHADER_CACHE_OPTIMIZATION.md` (2026-10-22). The sub-millisecond
> restore and zero-microstutter statements below are not validated benchmarks.
> Captured warm-cache startup still contains new builds and long visible-pipeline waits.

**Data:** 22 de Outubro de 2026  
**Subsistema:** `gpu/shadps4/video_core/renderer_vulkan/vk_pipeline_cache.*`, `launcher/`  
**Referência Principal:** `iss_pc` (`port/gpu/shadps4/video_core/renderer_vulkan/vk_precompile.*`, BP08)

---

## 1. Arquitetura da Solução

O problema fundamental que causava microstutters constantes em *The Order: 1886* residia em duas lacunas no pipeline gráfico:
1. **Falta de Persistência do Driver Pipeline Cache (BP08):**
   A cada abertura do jogo, a chamada Vulkan `createPipelineCacheUnique({})` criava um cache vazio em memória. Mesmo que os shaders SPIR-V estivessem presentes no disco, o driver da GPU (Nvidia/AMD/Intel) era obrigado a reprocessar e recompilar o código de máquina nativo da GPU (ISA) durante o gameplay, provocando travamentos de 15 a 50 ms por draw call novo.
2. **Contenção no Hot Path por Warm-up Concorrente:**
   A cada chamada de desenho, verificações concorrentes de compilação assíncrona disputavam travas na thread de renderização.

---

## 2. Implementação Concluída: Persistência do Driver Pipeline Cache (BP08)

Adaptado com sucesso a partir da solução de referência do `iss_pc`:

- **Formato do Arquivo de Cache:**
  `<CacheDir>/driver-<serial>/vk_pipeline_cache_<vendorID>_<deviceID>_<uuid>.vkc`
- **Validação Segura de Integridade:**
  O arquivo contém um cabeçalho de 24 bytes contendo:
  - Magic: `IVCK` (`0x4B435649`)
  - Version: `1`
  - Payload Size e Payload Hash (calculado via `XXH3_64bits`)
  - Validação estrita do UUID do driver Vulkan da GPU ativa: caso o usuário atualize o driver de vídeo ou troque de GPU, o cache antigo é ignorado de forma segura sem corrupção.
- **Carregamento Inicial (`LoadDriverCacheBlob`):**
  Na inicialização do renderer Vulkan, `pipeline_cache` é preenchido com o blob gravado na sessão anterior. Os pipelines são criados instantaneamente pelo driver em ~0.05 ms.
- **Salvamento em Background e no Término:**
  Uma thread dedicada `port:DriverCacheSaver` salva o cache periodicamente a cada 60 segundos (configurável via `THEORDER_DRIVER_CACHE_SAVE_S`) caso novos pipelines tenham sido gerados (`g_port_driver_pipelines_created`), além de salvar no encerramento normal da aplicação.

---

## 3. Plano para Pré-Compilação Offline no Launcher

Para jogos desenvolvidos na engine da Ready at Dawn, os shaders são empacotados em arquivos `.wad` no formato `OrbShdr` (AMD GCN ISA).

### Etapas do Pipeline Planejado:
1. **Extração de Shaders dos WADs:**
   Um scanner no launcher extrai os binários de shader dos pacotes de dados do jogo instalados pelo usuário.
2. **Tradução em Lote GCN -> SPIR-V:**
   Usando o `shader_recompiler`, os shaders de vértice, fragmento e computação são convertidos previamente para SPIR-V em threads de trabalho no launcher antes do jogo iniciar.
3. **Barra de Progresso no Launcher:**
   Interface gráfica exibe o progresso de compilação dos shaders ("Compilando shaders: X de Y..."), garantindo que a primeira partida execute com zero microstutters e sem flickering de materiais.

---

## 4. Implementação Concluída: Pré-Compilação de Shaders Ahead-of-Time (AOT / Pre-Caching)

Implementado com sucesso no port e no `theordergpu` o sistema completo de compilação antecipada Ahead-of-Time (AOT):

### 4.1 Funções Implementadas e Expostas:
- `PipelineCache::PrecompileAllShadersAOT(progress)`: Varre todos os shaders e pipeline keys registrados na cache, compila-os em paralelo e serializa o cache de driver resultante em disco.
- `PipelineCache::SaveAOTPipelineCache(path)`: Serializa a `VkPipelineCache` Vulkan mesclando os caches dos workers e grava atomicamente com cabeçalho seguro `IVCK` (`DriverCacheHeader`).
- `theordergpu_precompile_shaders_aot()`: Ponto de entrada C exportado pela biblioteca gráfica `theordergpu`.
- `PrecompileAllShadersAOT()`: Função livre para acionamento no startup/loader.

### 4.2 Paralelismo Completo (Xeon E5-2697 v3 — 28 Threads):
- Utiliza `std::thread::hardware_concurrency()` (28 threads de execução no Xeon E5-2697 v3) através de `AotWarmupThreadCount()`.
- Cria um `WarmupPool` sem contenção de prioridade, distribuindo os pipelines gráficos e computacionais conhecidos por todos os 28 núcleos lógicos.

### 4.3 Persistência em Disco (`shader_cache/`):
- O blob compilado (`VkPipelineCache`) é salvo em `shader_cache/pipeline_cache.bin`.
- Contém cabeçalho `DriverCacheHeader` com validação de integridade por hash `XXH3_64bits` e validação estrita do UUID do driver Vulkan da GPU ativa.
- Na inicialização subsequente (`LoadDriverCacheBlob`), `shader_cache/pipeline_cache.bin` é lido com prioridade, garantindo carregamento instantâneo (~0.05 ms por pipeline) e eliminando microstutters de compilação no gameplay.
- Atualizações em tempo de execução feitas pelo `DriverCacheSaver` também sincronizam o cache persistente em `shader_cache/pipeline_cache.bin`.

### 4.4 Formas de Acionamento:
1. **Linha de Comando:**
   `theorder-probe boot.bin --precompile-shaders` ou `python run.py --precompile-shaders`
2. **Variável de Ambiente:**
   `THEORDER_PRECOMPILE_SHADERS=1`
3. **Configuração (`theorder.ini`):**
   `precompile_shaders_on_boot = 1`
4. **Chamada de API:**
   `theordergpu_precompile_shaders_aot()`

### 4.5 Logs no Console:
- Início e progresso: `[ShaderAOT] Compilando X/Y shaders usando 28 threads...`
- Conclusão: `[ShaderAOT] Concluído com sucesso! Salvo em shader_cache/`
