@echo off
setlocal enabledelayedexpansion
set "G=tools\mingit\cmd\git.exe"
set "SW=Skywich <177953465+Skywich@users.noreply.github.com>"
set "LL=larryll123 <243148895+larryll123@users.noreply.github.com>"

%G% checkout -B rewrite2 6ec01ec
if errorlevel 1 exit /b 1

REM 1) PR-1 trade error codes - Skywich
%G% log -1 --format=%%at 9a1e704 > tmp_d.txt
set /p D=<tmp_d.txt
%G% log -1 --format=%%B 9a1e704 > tmp_m.txt
%G% cherry-pick -n 9a1e704
%G% commit --author="%SW%" --date=@!D! -F tmp_m.txt
if errorlevel 1 exit /b 1

REM 2) PR-2 JobSystem - Skywich
%G% log -1 --format=%%at 10cd38c > tmp_d.txt
set /p D=<tmp_d.txt
%G% log -1 --format=%%B 10cd38c > tmp_m.txt
%G% cherry-pick -n 10cd38c
%G% commit --author="%SW%" --date=@!D! -F tmp_m.txt
if errorlevel 1 exit /b 1

REM 3) PR-3 StudySystem - Skywich
%G% log -1 --format=%%at dbcdf25 > tmp_d.txt
set /p D=<tmp_d.txt
%G% log -1 --format=%%B dbcdf25 > tmp_m.txt
%G% cherry-pick -n dbcdf25
%G% commit --author="%SW%" --date=@!D! -F tmp_m.txt
if errorlevel 1 exit /b 1

REM 4) EventBus unsubscribe fix - larryll123
%G% log -1 --format=%%at 4c5d09e > tmp_d.txt
set /p D=<tmp_d.txt
%G% log -1 --format=%%B 4c5d09e > tmp_m.txt
%G% cherry-pick -n 4c5d09e
%G% commit --author="%LL%" --date=@!D! -F tmp_m.txt
if errorlevel 1 exit /b 1

REM 5) PR-4 GoalTracker - larryll123
%G% log -1 --format=%%at e0aef2c > tmp_d.txt
set /p D=<tmp_d.txt
%G% log -1 --format=%%B e0aef2c > tmp_m.txt
%G% cherry-pick -n e0aef2c
%G% commit --author="%LL%" --date=@!D! -F tmp_m.txt
if errorlevel 1 exit /b 1

REM 6) final-week message fix - Skywich
%G% log -1 --format=%%at c6be873 > tmp_d.txt
set /p D=<tmp_d.txt
%G% log -1 --format=%%B c6be873 > tmp_m.txt
%G% cherry-pick -n c6be873
%G% commit --author="%SW%" --date=@!D! -F tmp_m.txt
if errorlevel 1 exit /b 1

REM 7) build.bat follow-up - Skywich
%G% log -1 --format=%%at 9acab88 > tmp_d.txt
set /p D=<tmp_d.txt
%G% log -1 --format=%%B 9acab88 > tmp_m.txt
%G% cherry-pick -n 9acab88
%G% commit --author="%SW%" --date=@!D! -F tmp_m.txt
if errorlevel 1 exit /b 1

REM 8) tooling commit - sakustay152 (keep original author)
%G% cherry-pick 8690afa
if errorlevel 1 exit /b 1

del tmp_d.txt tmp_m.txt 2>nul
echo ===AUTHORS===
%G% log rewrite2 -8 --format="%%h %%an %%s"
echo ===TREE-CHECK===
%G% diff 8690afa rewrite2 --stat
echo REWRITE2-OK
