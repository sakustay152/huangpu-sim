@echo off
setlocal enabledelayedexpansion
set "G=tools\mingit\cmd\git.exe"
set "A=Skywich <177953465+Skywich@users.noreply.github.com>"

%G% branch -D rewrite-c 2>nul
%G% checkout -B rewrite-c origin/main
if errorlevel 1 exit /b 1

for %%s in (2a07b33 34c5410 d742565 7c93977 821ab1d 4b0697d 5a836ce) do (
    %G% log -1 --format=%%at %%s > tmp_date.txt
    set /p D=<tmp_date.txt
    %G% log -1 --format=%%B %%s > tmp_msg.txt
    %G% cherry-pick -n %%s
    if errorlevel 1 exit /b 1
    %G% commit --author="%A%" --date=@!D! -F tmp_msg.txt
    if errorlevel 1 exit /b 1
)
del tmp_date.txt tmp_msg.txt 2>nul
echo ===AUTHORS-AFTER===
%G% log rewrite-c -7 --format="%%h %%an ^<%%ae^> %%s"
echo ===TREE-IDENTICAL-CHECK===
%G% diff 5a836ce rewrite-c --stat
echo REWRITE-OK
