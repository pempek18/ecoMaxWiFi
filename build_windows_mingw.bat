@echo off
REM Build script for Windows using CMake with MinGW
REM This script builds the furnace_reader executable using MinGW

echo ========================================
echo Furnace Reader Build Script (MinGW)
echo ========================================
echo.

REM Check if CMake is available
where cmake >nul 2>&1
if errorlevel 1 (
    echo ERROR: CMake is not installed or not in PATH!
    echo Please install CMake from https://cmake.org/download/
    echo.
    pause
    exit /b 1
)

REM Check if MinGW is available
where g++ >nul 2>&1
if errorlevel 1 (
    echo ERROR: MinGW g++ compiler is not in PATH!
    echo.
    echo Please install MinGW-w64 from:
    echo   https://www.mingw-w64.org/downloads/
    echo.
    echo Or use MSYS2:
    echo   https://www.msys2.org/
    echo.
    echo Make sure g++ is in your PATH after installation.
    echo.
    pause
    exit /b 1
)

echo Creating build directory...
if not exist build mkdir build
cd build

echo.
echo Configuring CMake with MinGW Makefiles...
cmake .. -G "MinGW Makefiles"
if errorlevel 1 (
    echo.
    echo ========================================
    echo CMake configuration failed!
    echo ========================================
    echo.
    echo Possible issues:
    echo   1. MinGW is not properly installed
    echo   2. MinGW is not in your PATH
    echo   3. CMake cannot find the compiler
    echo.
    echo Try running from a MinGW shell or add MinGW to PATH.
    echo.
    pause
    exit /b 1
)

echo.
echo Building project...
cmake --build .
if errorlevel 1 (
    echo.
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo ========================================
echo Build successful!
echo ========================================
echo.
if exist bin\furnace_reader.exe (
    echo Executable: build\bin\furnace_reader.exe
) else (
    echo Executable location: build\bin\
)
echo.
pause

