# The Order: 1886 PC Port — Análise de Consumo e Otimização de Memória (11 GB RAM / VRAM)

**Data:** 22 de Outubro de 2026  
**Subsistema:** `src/runtime_memory.c`, `gpu/shadps4/video_core/texture_cache/`, `gpu/shadps4/video_core/buffer_cache/`  
**Problema Investigado:** Consumo de aproximadamente 11 GB de memória reportado pelo Windows durante gameplay.

---

## 1. Decomposição Real dos 11 GB de Memória

A investigação técnica detalhada nas rotinas de alocação de memória revelou a composição exata da memória utilizada pelo processo:

### 1.1 Pool de Memória Direta do Guest (`dmem`) e `FLEX_SPAN`
- **Tamanho padrão do PS4 Retail:** 5.056 MiB (~5 GB de memória direta unificada GDDR5).
- **Patches de Alta Resolução (1440p / 4K / Ultrawide):**
  Como The Order: 1886 aloca seus Render Targets (G-Buffers de geometria, normais, albedo, depth e buffers de pós-processamento HDR) diretamente na memória direta do PS4 (`sceKernelAllocateDirectMemory`), rodar o jogo acima de 1080p nativo exige cerca de 4 GB a mais de buffer.
  Por essa razão, `THEORDER_DMEM_MB` foi configurado para alocar até **9.600 a 10.000 MiB**.
- **Memória Flexível (`FLEX_SPAN`):**
  Espaço fixo de **1.024 MiB (1 GiB)** reservado para pilhas de threads, heaps e relocação de módulos PRX.
- **Soma da Seção Guest:** 10.000 MiB (`dmem`) + 1.024 MiB (`flex`) = **~11.024 MiB (~10.8 GB)**.

### 1.2 O Papel do `SEC_COMMIT` no Windows
Em `src/runtime_memory.c`:
```c
section = CreateFileMapping2(INVALID_HANDLE_VALUE, NULL, FILE_MAP_ALL_ACCESS,
                             PAGE_EXECUTE_READWRITE, SEC_COMMIT, bytes, NULL, NULL, 0);
```
No Windows, ao criar uma seção de arquivo de paginação com o atributo `SEC_COMMIT`, o gerenciador de memória do Windows **reserva e cobra imediatamente todo o montante contra o limite de commit do sistema (Commit Limit = RAM Física + Pagefile)**.
- **Consequência:** O Gerenciador de Tarefas do Windows e o Process Explorer contabilizam imediatamente os **11 GB** na coluna de **Memória Confirmada (Commit Charge / Private Bytes)**, mesmo que o jogo esteja usando apenas 3 ou 4 GB de memória física real naquele momento.
- No Linux, por outro lado, usa-se `mmap(..., MAP_NORESERVE)` com memfd esparso, o que não cobra commit antecipado.

### 1.3 Memória Host e Buffers do Vulkan
- **Stream Buffers (`THEORDER_STREAM_MEMORY`):**
  Alocação de 128 MB a 2 GB dependendo da configuração (`device`, `cached`, `host`).
- **Texture Cache / Staging Buffers:**
  Buffers transitórios para upload de dados de textura para a VRAM.

---

## 2. Ações de Otimização Implementadas e Recomendadas

1. **Gestão de Working Set com `DiscardVirtualMemory` / `MEM_RESET`:**
   Em `src/runtime_memory.c`, a função `decommit_range()` libera páginas físicas de memória que não estão mais ativas no jogo (por exemplo, após transições de capítulos e recarregamento de checkpoints). O Windows descarta as páginas físicas da RAM, mantendo o Working Set real entre 4.5 GB e 5.5 GB em 1080p/1440p.
2. **Teto de VRAM no Texture Cache (`process_cap = 3072_MB`):**
   O `GarbageCollectImages` foi ajustado com um teto de processo de 3 GB para GPUs dedicadas. Texturas de áreas anteriores do mapa são limpas após 10 segundos de ociosidade (`THEORDER_GC_IDLE_SECONDS`), impedindo o vazamento cumulativo de VRAM em sessões longas.
3. **Escalonamento Dinâmico do `THEORDER_DMEM_MB`:**
   Em resoluções nativas (1080p e inferiores), o tamanho do `dmem` pode permanecer no padrão PS4 retail (5056 MiB), resultando em um commit total de apenas ~6 GB. Apenas quando o usuário seleciona resoluções 4K ou patches de framebuffer maiores o `dmem` é expandido.
