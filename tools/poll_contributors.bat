@echo off
for /l %%i in (1,1,3) do (
    ping -n 8 127.0.0.1 >nul
    curl.exe --ssl-no-revoke -sS -H @tools\.tok_header "https://api.github.com/repos/sakustay152/huangpu-sim/contributors" -o tools\c1.json
    findstr /c:"\"login\"" tools\c1.json
    echo ---
)
