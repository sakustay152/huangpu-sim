@echo off
REM ============================================================
REM Build static-linked release executables into build-rel\
REM (/MT: statically link the CRT so the exes run on any Windows
REM  machine without the VC++ redistributable)
REM Usage: run from project root (or a git worktree root)
REM ============================================================
setlocal
set "VSDIR=D:\visual studio\produce"
if not exist "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" (
    echo [ERROR] MSVC toolchain not found, edit VSDIR in this script.
    exit /b 1
)
call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if errorlevel 1 (
    echo [ERROR] vcvars64 init failed
    exit /b 1
)

if not exist build-rel mkdir build-rel
del /q build-rel\* 2>nul

set "RSP=build-rel\sources.rsp"
type nul > %RSP%
for /r src %%f in (*.cpp) do @echo "%%f">>%RSP%

set "CFLAGS=/nologo /std:c++17 /utf-8 /EHsc /W3 /O2 /MT /Iinclude /Ithird_party"

echo [1/3] Compiling rule layer (static CRT) ...
cl %CFLAGS% /c @%RSP%
if errorlevel 1 goto :fail
move /y *.obj build-rel\ >nul
lib /nologo /out:build-rel\huangpu_core.lib build-rel\*.obj
if errorlevel 1 goto :fail

echo [2/3] Linking game and simulator ...
cl %CFLAGS% main\main_console.cpp build-rel\huangpu_core.lib /Fe:build-rel\huangpu_sim.exe
if errorlevel 1 goto :fail
del /q main_console.obj 2>nul
cl %CFLAGS% main\main_simulate.cpp build-rel\huangpu_core.lib /Fe:build-rel\huangpu_simulate.exe
if errorlevel 1 goto :fail
del /q main_simulate.obj 2>nul

echo [3/3] Building tests ...
cl %CFLAGS% tests\test_main.cpp build-rel\huangpu_core.lib /Fe:build-rel\huangpu_tests.exe
if errorlevel 1 goto :fail
del /q test_main.obj 2>nul

echo RELEASE-BUILD-OK
exit /b 0

:fail
echo [ERROR] release build failed
exit /b 1
