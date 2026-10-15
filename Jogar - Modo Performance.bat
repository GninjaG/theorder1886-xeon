@echo off
chcp 65001 >nul
title The Order: 1886 - Modo Performance Extrema
echo ===============================================================================
echo        The Order: 1886 - Modo Performance Extrema (60 FPS / 1ms Timer)
echo ===============================================================================
echo.
echo Desativando cálculos de física pesada de tecidos (-disablecloth)
echo Desativando molas e oscilações secundárias de câmera (-nocamsprings -nocamadditive)
echo Caching de pipeline binário Vulkan ativado em user/cache/CUSA00035/
echo.
python run.py --perf
if errorlevel 1 pause
