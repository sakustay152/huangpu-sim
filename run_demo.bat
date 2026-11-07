@echo off
REM ============================================
REM Run the game demo (interactive)
REM Scripted regression: huangpu_sim.exe < demo_input.txt
REM ============================================
cd /d "%~dp0build"
if not exist huangpu_sim.exe (
    echo [ERROR] huangpu_sim.exe not found. Run build.bat at project root first.
    pause
    exit /b 1
)
huangpu_sim.exe
