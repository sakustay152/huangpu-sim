@echo off
REM Rewrite teammate C's 7 commits with Skywich as author (content and dates unchanged)
cd /d "D:\面向对象程序设计\HuangpuSim-c"
set GIT=..\HuangpuSim\tools\mingit\cmd\git.exe
set GIT_AUTHOR_NAME=Skywich
set GIT_AUTHOR_EMAIL=177953465+Skywich@users.noreply.github.com
%GIT% checkout -B rewrite-c origin/main
if errorlevel 1 exit /b 1
%GIT% cherry-pick 2a07b33 34c5410 d742565 7c93977 821ab1d 4b0697d 5a836ce
if errorlevel 1 exit /b 1
echo ===AUTHORS-AFTER===
%GIT% log rewrite-c -7 --format="%%h %%an ^<%%ae^> %%s"
echo ===TREE-IDENTICAL-CHECK===
%GIT% diff 5a836ce rewrite-c --stat
if errorlevel 1 exit /b 1
echo REWRITE-OK
