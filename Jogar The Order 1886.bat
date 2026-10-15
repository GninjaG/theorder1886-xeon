@echo off
chcp 65001 >nul
title The Order: 1886 - PC Native Port (Vulkan Direct Execution)
color 0F

:menu
cls
echo ===============================================================================
echo                THE ORDER: 1886 - PC NATIVE PORT (VULKAN)
echo                     Ready at Dawn / Sony Santa Monica
echo ===============================================================================
echo.
echo   [1] Iniciar Jogo (Início Rápido - Westminster / Cutscenes Puladas)
echo   [2] Capítulo 1: Ruas de Whitechapel (cat_010_streets)
echo   [3] Capítulo 1: Túneis da Embaixada (emb_020_tunnels)
echo   [4] Capítulo 3: Dirigível Agamemnon (air_010_rappel)
echo   [5] Modo Performance Extrema (Sem física pesada de tecidos, 1ms Quantum)
echo   [6] Selecionar Nível Personalizado
echo   [7] Ver Controles e Mapeamento (DualSense e Teclado)
echo   [8] Ver Estatísticas de Frame Pacing e Logs da Última Partida
echo   [9] Sair
echo.
echo ===============================================================================
set /p choice="Escolha uma opção (1-9): "

if "%choice%"=="1" goto launch_fast
if "%choice%"=="2" goto launch_streets
if "%choice%"=="3" goto launch_tunnels
if "%choice%"=="4" goto launch_airship
if "%choice%"=="5" goto launch_perf
if "%choice%"=="6" goto custom_level
if "%choice%"=="7" goto show_controls
if "%choice%"=="8" goto show_stats
if "%choice%"=="9" goto exit

goto menu

:launch_fast
cls
echo [INFO] Iniciando The Order: 1886 no modo Início Rápido...
echo [INFO] Pulos de logotipos e introdução ativados.
echo [INFO] Controle DualSense ou Teclado pronto.
python run.py
pause
goto menu

:launch_streets
cls
echo [INFO] Iniciando Capítulo 1: Ruas de Whitechapel...
python run.py --level cat_010_streets
pause
goto menu

:launch_tunnels
cls
echo [INFO] Iniciando Capítulo 1: Túneis da Embaixada...
python run.py --level emb_020_tunnels
pause
goto menu

:launch_airship
cls
echo [INFO] Iniciando Capítulo 3: Dirigível Agamemnon...
python run.py --level air_010_rappel
pause
goto menu

:launch_perf
cls
echo [INFO] Iniciando Modo Performance Extrema...
python run.py --perf
pause
goto menu

:custom_level
cls
echo ===============================================================================
echo Níveis disponíveis para teste:
echo   - pro_010_catacombs     (Prólogo: Catacumbas)
echo   - pro_020_westminster   (Prólogo: Westminster)
echo   - cat_010_streets       (Capítulo 1: Ruas)
echo   - emb_020_tunnels       (Capítulo 1: Túneis)
echo   - wch_010_arrival       (Capítulo 2: Whitechapel Chegada)
echo   - wch_020_streets       (Capítulo 2: Ruas de Whitechapel)
echo   - air_010_rappel        (Capítulo 3: Rapel Dirigível)
echo ===============================================================================
set /p lvl="Digite o identificador do nível: "
cls
echo [INFO] Iniciando nível %lvl%...
python run.py --level %lvl%
pause
goto menu

:show_controls
cls
echo ===============================================================================
echo                        MAPEAMENTO DE CONTROLES
echo ===============================================================================
echo.
echo   CONTROLE PLAYSTATION / XBOX (Recomendado):
echo   -----------------------------------------
echo   - Analógico Esquerdo (L3): Mover Sir Galahad
echo   - Analógico Direito (R3):  Controlar Câmera
echo   - Cruz / Botão A:          Interagir / Pular / Escalar
echo   - Círculo / Botão B:       Agachar / Cobertura
echo   - Quadrado / Botão X:      Ataque Corpo a Corpo (Melee)
echo   - Triângulo / Botão Y:     Trocar Arma / Visão Geral
echo   - L2 / LT:                 Mirar com Arma
echo   - R2 / RT:                 Disparar Arma
echo   - L1 / LB:                 Recarregar / Granada
echo   - R1 / RB:                 Fogo Alternativo
echo   - Touchpad / Back:         Diário / Monóculo
echo   - Options / Start:         Pausa / Menu do Jogo
echo   - Insert ou R3+L2:         Menu de Opções Gráficas da Engine (Overlay)
echo.
echo   TECLADO:
echo   -------
echo   - W, A, S, D:              Mover Personagem
echo   - Mouse:                   Controle de Câmera
echo   - Espaço:                  Cruz / A (Interagir)
echo   - Botão Direito Mouse:     L2 (Mirar)
echo   - Botão Esquerdo Mouse:    R2 (Disparar)
echo   - R:                       Recarregar
echo   - Shift Esquerdo:          Correr
echo   - Esc:                     Pausa
echo.
echo ===============================================================================
pause
goto menu

:show_stats
cls
echo ===============================================================================
echo              ESTATÍSTICAS DE FRAME PACING E ÚLTIMA EXECUÇÃO
echo ===============================================================================
echo.
if exist run_video_test.log (
    findstr /C:"Frame stats" /C:"Frame pacing" /C:"Compiler stats" run_video_test.log
) else (
    echo Nenhum log de teste gravado ainda.
)
echo.
echo ===============================================================================
pause
goto menu

:exit
exit /b 0
