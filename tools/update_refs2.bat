@echo off
setlocal enabledelayedexpansion
set "G=tools\mingit\cmd\git.exe"

%G% checkout main
%G% reset --hard rewrite2
%G% branch -f feature/C-player-part-modified 48fc96f
if errorlevel 1 exit /b 1

%G% cat-file tag c-v0.1.0-trade | findstr /v /b /c:"object " /c:"type " /c:"tag " /c:"tagger " > tmp_msg1.txt
%G% tag -f -a c-v0.1.0-trade -F tmp_msg1.txt 39686f6
%G% cat-file tag c-v0.2.0-jobs | findstr /v /b /c:"object " /c:"type " /c:"tag " /c:"tagger " > tmp_msg2.txt
%G% tag -f -a c-v0.2.0-jobs -F tmp_msg2.txt cb3f1e4
%G% cat-file tag c-v0.3.0-study | findstr /v /b /c:"object " /c:"type " /c:"tag " /c:"tagger " > tmp_msg3.txt
%G% tag -f -a c-v0.3.0-study -F tmp_msg3.txt 621316e
%G% cat-file tag c-v0.3.1-eventbus | findstr /v /b /c:"object " /c:"type " /c:"tag " /c:"tagger " > tmp_msg4.txt
%G% tag -f -a c-v0.3.1-eventbus -F tmp_msg4.txt 6e25a0a
%G% cat-file tag c-v0.4.0-goal | findstr /v /b /c:"object " /c:"type " /c:"tag " /c:"tagger " > tmp_msg5.txt
%G% tag -f -a c-v0.4.0-goal -F tmp_msg5.txt 847199b
%G% cat-file tag c-v1.0.0 | findstr /v /b /c:"object " /c:"type " /c:"tag " /c:"tagger " > tmp_msg6.txt
%G% tag -f -a c-v1.0.0 -F tmp_msg6.txt f87301e
%G% tag -f v1.1.0 48fc96f
del tmp_msg1.txt tmp_msg2.txt tmp_msg3.txt tmp_msg4.txt tmp_msg5.txt tmp_msg6.txt 2>nul

%G% branch -D rewrite2
echo ===FINAL-STATE===
%G% log --oneline -9
%G% for-each-ref refs/tags/ --format="%%(refname:short) -> %%(objectname:short)"
echo REFS-OK
