@echo off
chcp 65001 >nul
title The Order: 1886 - Início Rápido
echo ===============================================================================
echo            Iniciando The Order: 1886 (Vulkan PC Native Port)
echo ===============================================================================
echo.
echo Execucao original e estavel (todos os patches desativados).
echo Conecte seu controle ou use o teclado. Bom jogo!
echo.
python run.py
if errorlevel 1 pause
