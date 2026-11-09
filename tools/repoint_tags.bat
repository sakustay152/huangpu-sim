@echo off
set "G=tools\mingit\cmd\git.exe"
%G% cat-file tag c-v0.1.0-trade | findstr /v /b /c:"object " /c:"type " /c:"tag " /c:"tagger " > tmp_msg1.txt
%G% tag -f -a c-v0.1.0-trade -F tmp_msg1.txt 9a1e704
if errorlevel 1 exit /b 1
%G% cat-file tag c-v0.2.0-jobs | findstr /v /b /c:"object " /c:"type " /c:"tag " /c:"tagger " > tmp_msg2.txt
%G% tag -f -a c-v0.2.0-jobs -F tmp_msg2.txt 10cd38c
if errorlevel 1 exit /b 1
%G% cat-file tag c-v0.3.0-study | findstr /v /b /c:"object " /c:"type " /c:"tag " /c:"tagger " > tmp_msg3.txt
%G% tag -f -a c-v0.3.0-study -F tmp_msg3.txt dbcdf25
if errorlevel 1 exit /b 1
%G% cat-file tag c-v0.3.1-eventbus | findstr /v /b /c:"object " /c:"type " /c:"tag " /c:"tagger " > tmp_msg4.txt
%G% tag -f -a c-v0.3.1-eventbus -F tmp_msg4.txt 4c5d09e
if errorlevel 1 exit /b 1
%G% cat-file tag c-v0.4.0-goal | findstr /v /b /c:"object " /c:"type " /c:"tag " /c:"tagger " > tmp_msg5.txt
%G% tag -f -a c-v0.4.0-goal -F tmp_msg5.txt e0aef2c
if errorlevel 1 exit /b 1
%G% cat-file tag c-v1.0.0 | findstr /v /b /c:"object " /c:"type " /c:"tag " /c:"tagger " > tmp_msg6.txt
%G% tag -f -a c-v1.0.0 -F tmp_msg6.txt c6be873
if errorlevel 1 exit /b 1
del tmp_msg1.txt tmp_msg2.txt tmp_msg3.txt tmp_msg4.txt tmp_msg5.txt tmp_msg6.txt 2>nul
echo TAGS-REPOINTED
%G% for-each-ref refs/tags/c-* --format="%(refname:short) -> %(objectname:short)"
