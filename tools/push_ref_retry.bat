@echo off
setlocal enabledelayedexpansion
set /p T=<"%~dp0.gh_token"
set "URL=https://!T!@github.com/sakustay152/huangpu-sim.git"
set "SPEC=%~1"
if "%SPEC%"=="" set "SPEC=main"
for %%m in (direct proxy direct proxy) do (
    if %%m==direct (
        "%~dp0mingit\cmd\git.exe" -c http.proxy= push "!URL!" !SPEC! >"%TEMP%\push_out.txt" 2>&1
    ) else (
        "%~dp0mingit\cmd\git.exe" -c http.proxy=http://127.0.0.1:7897 push "!URL!" !SPEC! >"%TEMP%\push_out.txt" 2>&1
    )
    if not errorlevel 1 (
        findstr /v /i /c:"gho_" <"%TEMP%\push_out.txt"
        echo PUSH-SUCCESS !SPEC!
        exit /b 0
    )
    findstr /v /i /c:"gho_" <"%TEMP%\push_out.txt"
    ping -n 4 127.0.0.1 >nul
)
echo PUSH-FAIL !SPEC!
exit /b 1
