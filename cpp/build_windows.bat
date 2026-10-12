@echo off
setlocal enabledelayedexpansion

echo ============================================================================
echo   ECHO LABYRINTH v5.0.1 "RESONANCE" - Native C++17 / SFML 2.6.1 Build
echo ============================================================================
echo.

where cmake >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake was not found in your PATH.
    echo Please install CMake 3.16+ from https://cmake.org/download/ or via Visual Studio Installer.
    exit /b 1
)

echo [1/3] Configuring CMake project (downloading SFML 2.6.1 if first run)...
cmake -S "%~dp0." -B "%~dp0build" -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake configuration failed. Ensure a C++17 compiler ^(Visual Studio 2022 or MinGW-w64^) is installed.
    exit /b 1
)

echo.
echo [2/3] Compiling EchoLabyrinth_SFML (Release)...
cmake --build "%~dp0build" --config Release --parallel 4
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Compilation failed.
    exit /b 1
)

set "EXE_PATH=%~dp0build\bin\Release\EchoLabyrinth_SFML.exe"
if not exist "!EXE_PATH!" (
    set "EXE_PATH=%~dp0build\bin\EchoLabyrinth_SFML.exe"
)

echo.
echo [3/3] Running deterministic 18-suite engine self-test...
"!EXE_PATH!" --self-test
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Self-test verification failed!
    exit /b 1
)

echo.
echo ============================================================================
echo   BUILD SUCCESSFUL!
echo   Executable: !EXE_PATH!
echo ============================================================================
echo.

if /i "%~1"=="--no-run" (
    exit /b 0
)

echo Launching EchoLabyrinth...
"!EXE_PATH!"
endlocal
