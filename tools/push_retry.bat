@echo off
REM retry push alternating direct/proxy (checks git's own errorlevel)
setlocal enabledelayedexpansion
set /p T=<"%~dp0.gh_token"
set "URL=https://!T!@github.com/sakustay152/huangpu-sim.git"
for %%m in (direct proxy direct proxy direct proxy direct proxy) do (
    if %%m==direct (
        echo attempt: direct
        "%~dp0mingit\cmd\git.exe" -c http.proxy= push "!URL!" main >"%TEMP%\push_out.txt" 2>&1
    ) else (
        echo attempt: proxy
        "%~dp0mingit\cmd\git.exe" -c http.proxy=http://127.0.0.1:7897 push "!URL!" main >"%TEMP%\push_out.txt" 2>&1
    )
    if not errorlevel 1 (
        findstr /v /i /c:"gho_" <"%TEMP%\push_out.txt"
        echo PUSH-SUCCESS
        exit /b 0
    )
    findstr /v /i /c:"gho_" <"%TEMP%\push_out.txt"
    ping -n 5 127.0.0.1 >nul
)
echo PUSH-FAIL-ALL
exit /b 1
