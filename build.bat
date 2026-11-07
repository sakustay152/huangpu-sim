@echo off
REM ============================================================
REM One-click build (MSVC direct, no CMake needed)
REM Output in build\: huangpu_sim.exe / huangpu_simulate.exe / huangpu_tests.exe
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

if not exist build mkdir build
del /q build\*.obj build\*.exe build\*.lib 2>nul

REM ---- response file: src\*.cpp ----
set "RSP=build\sources.rsp"
type nul > %RSP%
for /r src %%f in (*.cpp) do @echo "%%f">>%RSP%

echo [1/3] Compiling rule layer (src/ -> static lib) ...
cl /nologo /std:c++17 /utf-8 /EHsc /W3 /O2 /Iinclude /Ithird_party /c @%RSP%
if errorlevel 1 goto :fail
move /y *.obj build\ >nul
lib /nologo /out:build\huangpu_core.lib build\*.obj
if errorlevel 1 goto :fail

echo [2/3] Linking game and simulator ...
cl /nologo /std:c++17 /utf-8 /EHsc /W3 /O2 /Iinclude /Ithird_party main\main_console.cpp build\huangpu_core.lib /Fe:build\huangpu_sim.exe
if errorlevel 1 goto :fail
del /q main_console.obj 2>nul
cl /nologo /std:c++17 /utf-8 /EHsc /W3 /O2 /Iinclude /Ithird_party main\main_simulate.cpp build\huangpu_core.lib /Fe:build\huangpu_simulate.exe
if errorlevel 1 goto :fail
del /q main_simulate.obj 2>nul

echo [3/3] Building tests ...
cl /nologo /std:c++17 /utf-8 /EHsc /W3 /O2 /Iinclude /Ithird_party tests\test_main.cpp build\huangpu_core.lib /Fe:build\huangpu_tests.exe
if errorlevel 1 goto :fail
del /q test_main.obj 2>nul

echo.
echo Build OK. Run:
echo   cd build  then  huangpu_sim.exe        (game demo)
echo   cd build  then  huangpu_simulate.exe   (headless simulator)
echo   cd build  then  huangpu_tests.exe      (rule tests)
exit /b 0

:fail
echo [ERROR] Build failed
exit /b 1
