@echo off
REM Build script for Windows using CMake
REM This script builds the furnace_reader executable

echo ========================================
echo Furnace Reader Build Script
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

echo Creating build directory...
if not exist build mkdir build
cd build

echo.
echo Attempting to detect available generators...
echo.

REM Try Visual Studio 2022 first
echo Trying Visual Studio 2022...
cmake .. -G "Visual Studio 17 2022" -A x64 >nul 2>&1
if not errorlevel 1 (
    echo Successfully configured with Visual Studio 2022
    goto :build
)

REM Try Visual Studio 2019
echo Trying Visual Studio 2019...
cmake .. -G "Visual Studio 16 2019" -A x64 >nul 2>&1
if not errorlevel 1 (
    echo Successfully configured with Visual Studio 2019
    goto :build
)

REM Try Visual Studio 2017
echo Trying Visual Studio 2017...
cmake .. -G "Visual Studio 15 2017" -A x64 >nul 2>&1
if not errorlevel 1 (
    echo Successfully configured with Visual Studio 2017
    goto :build
)

REM Try MinGW
echo Trying MinGW Makefiles...
cmake .. -G "MinGW Makefiles" >nul 2>&1
if not errorlevel 1 (
    echo Successfully configured with MinGW
    goto :build
)

REM Try NMake (requires Visual Studio Build Tools)
echo Trying NMake Makefiles...
cmake .. -G "NMake Makefiles" >nul 2>&1
if not errorlevel 1 (
    echo Successfully configured with NMake
    goto :build
)

REM If all failed, show error
echo.
echo ========================================
echo ERROR: No suitable compiler found!
echo ========================================
echo.
echo Please install one of the following:
echo   1. Visual Studio 2017/2019/2022 (with C++ desktop development)
echo   2. MinGW-w64 (https://www.mingw-w64.org/)
echo   3. Visual Studio Build Tools
echo.
echo For MinGW, make sure it's in your PATH or use build_windows_mingw.bat
echo.
pause
exit /b 1

:build
echo.
echo Building project...
cmake --build . --config Release
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
if exist bin\Release\furnace_reader.exe (
    echo Executable: build\bin\Release\furnace_reader.exe
) else if exist bin\furnace_reader.exe (
    echo Executable: build\bin\furnace_reader.exe
) else (
    echo Executable location: build\bin\
)
echo.
pause

