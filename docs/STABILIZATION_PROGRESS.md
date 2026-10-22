# The Order: 1886 PC Port — Stabilization & Optimization Progress

**Data:** 22 de Outubro de 2026  
**Branch:** `optimization/advanced-v0.7.7`  
**Status da Suíte de Testes:** 100% aprovado (45/45 testes C/C++ + 305/305 testes Python)

---

## 1. Resumo das Tarefas Executadas

| Subsistema | Problema / Necessidade | Solução Aplicada | Status |
|---|---|---|---|
| **Pipeline Cache Vulkan** | Microstutters graves causados pela recompilação de pipelines a cada execução | Implementada a persistência de blob do driver (BP08 do `iss_pc`), carregando `vk_pipeline_cache_*.vkc` e salvando periodicamente e ao fechar | **CONCLUÍDO** |
| **Input / Mouse Look** | Mouse com inércia e sensação de emulação de analógico | Removido `mouse_impulse`, criado `mouse_stick.h` com anti-deadzone e sensibilidade X/Y independentes | **CONCLUÍDO** |
| **Suíte de Testes** | `pad-map-test.exe` travando em caixa de diálogo de assert no Windows | Corrigida a calibração de velocidade em `mouse_to_stick` e validação sem travas de interface gráfica | **CONCLUÍDO** |
| **Consumo de Memória (11 GB)** | Alocação elevada de RAM e Commit Charge no Windows | Investigada e documentada a causa raiz (`SEC_COMMIT` do `dmem` para resoluções altas) e teto de VRAM de 3 GB | **CONCLUÍDO** |
| **Diagnóstico para Usuários** | Falta de ferramenta padronizada para coleta de relatórios de bugs | Implementado botão "Export Bug Report (.zip)" no launcher (`port_report.py`) com anonimização de dados | **CONCLUÍDO** |
| **Documentação Técnica** | Documentação desatualizada para otimizações avançadas | Criados `PERFORMANCE_BASELINE.md`, `MOUSE_INPUT_REWORK.md`, `MEMORY_OPTIMIZATION.md`, `SHADER_PRECOMPILATION_PLAN.md` e `STABILIZATION_PROGRESS.md` | **CONCLUÍDO** |

---

## 2. Próximos Passos Recomendados

1. **Testes de Longa Duração:**
   Executar jogatina contínua de 30 minutos em áreas densas (Capítulos 1 e 3) para validar o aquecimento do driver cache persistido.
2. **Implementação do Scanner Offline de Shaders no Launcher:**
   Integrar a extração de `OrbShdr` dos arquivos `.wad` para popular os pipelines antes do primeiro início.
