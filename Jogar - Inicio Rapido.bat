@echo off
chcp 65001 >nul
title The Order: 1886 - Início Rápido
echo ===============================================================================
echo            Iniciando The Order: 1886 (Vulkan PC Native Port)
echo ===============================================================================
echo.
echo Logotipos de introdução e telas de espera pulados automaticamente.
echo Conecte seu controle ou use o teclado. Bom jogo!
echo.
python run.py
if errorlevel 1 pause
