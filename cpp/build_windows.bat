@echo off
setlocal
cd /d "%~dp0"

echo ============================================================================
echo EchoLabyrinth_SFML - Native Windows C++17 / SFML Build Script
echo ============================================================================

where cmake >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERROR] CMake was not found in PATH.
    echo Please install CMake 3.20+ or run from "Developer Command Prompt for VS 2022".
    pause
    exit /b 1
)

echo [1/3] Configuring CMake project in .\build ...
cmake -B build -DCMAKE_BUILD_TYPE=Release
if %errorlevel% neq 0 (
    echo [ERROR] CMake configuration failed.
    pause
    exit /b %errorlevel%
)

echo [2/3] Compiling Release executable ...
cmake --build build --config Release
if %errorlevel% neq 0 (
    echo [ERROR] Compilation failed.
    pause
    exit /b %errorlevel%
)

echo [3/3] Running automated C++ self-test suite ...
if exist "build\Release\EchoLabyrinth_SFML.exe" (
    "build\Release\EchoLabyrinth_SFML.exe" --self-test
    echo.
    echo Launching EchoLabyrinth_SFML.exe ...
    start "" "build\Release\EchoLabyrinth_SFML.exe"
) else if exist "build\EchoLabyrinth_SFML.exe" (
    "build\EchoLabyrinth_SFML.exe" --self-test
    echo.
    echo Launching EchoLabyrinth_SFML.exe ...
    start "" "build\EchoLabyrinth_SFML.exe"
) else (
    echo [WARNING] Executable was built, check the build folder.
)

pause
