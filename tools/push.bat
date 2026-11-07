@echo off
REM ============================================================
REM Push helper for huangpu-sim
REM Uses the device-flow token in tools\.gh_token (gitignored, local only)
REM Usage: tools\push.bat            (push current main)
REM        tools\push.bat <refspec>  (custom push args)
REM Requires: run from project root or anywhere (paths are self-resolved)
REM ============================================================
setlocal enabledelayedexpansion
if not exist "%~dp0.gh_token" (
    echo [ERROR] tools\.gh_token not found. Re-run the device-flow auth first.
    exit /b 1
)
set /p T=<"%~dp0.gh_token"
if "%~1"=="" (
    "%~dp0mingit\cmd\git.exe" push https://!T!@github.com/sakustay152/huangpu-sim.git main
) else (
    "%~dp0mingit\cmd\git.exe" push https://!T!@github.com/sakustay152/huangpu-sim.git %*
)
