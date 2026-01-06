# Build Instructions for Windows

## Prerequisites

You need to install one of the following:

### Option 1: Visual Studio (Recommended for Windows)

1. Download **Visual Studio Community** (free) from: https://visualstudio.microsoft.com/
2. During installation, select **"Desktop development with C++"** workload
3. This includes:
   - MSVC compiler
   - CMake support
   - Windows SDK

### Option 2: MinGW-w64

1. Download from one of these sources:
   - **MSYS2** (recommended): https://www.msys2.org/
     - After installing MSYS2, run: `pacman -S mingw-w64-x86_64-gcc`
   - **MinGW-w64**: https://www.mingw-w64.org/downloads/
   - **WinLibs**: https://winlibs.com/

2. Add MinGW `bin` directory to your PATH:
   - Example: `C:\msys64\mingw64\bin`
   - Or: `C:\MinGW\bin`

3. Verify installation:
   ```bash
   g++ --version
   ```

### Option 3: Visual Studio Build Tools

1. Download **Build Tools for Visual Studio** from: https://visualstudio.microsoft.com/downloads/
2. Select **"C++ build tools"** during installation

## Installing CMake

1. Download CMake from: https://cmake.org/download/
2. During installation, select **"Add CMake to system PATH"**
3. Verify installation:
   ```bash
   cmake --version
   ```

## Building the Project

### Method 1: Using Build Scripts (Easiest)

#### For Visual Studio:
```bash
build_windows.bat
```

#### For MinGW:
```bash
build_windows_mingw.bat
```

### Method 2: Manual CMake Commands

#### Visual Studio 2022:
```bash
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

#### Visual Studio 2019:
```bash
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019" -A x64
cmake --build . --config Release
```

#### MinGW:
```bash
mkdir build
cd build
cmake .. -G "MinGW Makefiles"
cmake --build .
```

### Method 3: Direct Compilation (No CMake)

If CMake is causing issues, you can compile directly:

#### With MinGW:
```bash
g++ -std=c++17 example_windows.cpp communicationWithFurner.cpp -o furnace_reader.exe
```

#### With MSVC (from Developer Command Prompt):
```bash
cl /EHsc /std:c++17 example_windows.cpp communicationWithFurner.cpp /Fe:furnace_reader.exe
```

## Troubleshooting

### Error: "CMAKE_CXX_COMPILER not set"

**Solution 1**: Install a C++ compiler (see Prerequisites above)

**Solution 2**: Specify compiler manually:
```bash
# For MinGW
cmake .. -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER=g++

# For MSVC (from Developer Command Prompt)
cmake .. -G "Visual Studio 17 2022" -A x64
```

**Solution 3**: Use Developer Command Prompt for Visual Studio
- Open "x64 Native Tools Command Prompt for VS 2022" (or similar)
- Run CMake commands from there

### Error: "nmake not found"

This means CMake is trying to use NMake but can't find it.

**Solution**: 
- Use Visual Studio generator instead: `cmake .. -G "Visual Studio 17 2022" -A x64`
- Or install Visual Studio Build Tools
- Or use MinGW: `cmake .. -G "MinGW Makefiles"`

### Error: "g++ not found" (MinGW)

**Solution**:
1. Verify MinGW is installed: `g++ --version`
2. Add MinGW to PATH:
   - Right-click "This PC" → Properties → Advanced system settings → Environment Variables
   - Add MinGW `bin` directory to PATH
3. Restart command prompt

### CMake can't find Visual Studio

**Solution**:
1. Make sure Visual Studio is installed with C++ workload
2. Try specifying the generator explicitly:
   ```bash
   cmake .. -G "Visual Studio 17 2022" -A x64
   ```
3. Or use Visual Studio's built-in CMake support (open folder in VS)

## Output Location

After successful build:
- **Visual Studio**: `build/bin/Release/furnace_reader.exe`
- **MinGW**: `build/bin/furnace_reader.exe`

## Running the Program

1. Connect your RS485 adapter to COM port (e.g., COM3)
2. Run the executable:
   ```bash
   furnace_reader.exe
   ```
3. Or modify `example_windows.cpp` to change the COM port:
   ```cpp
   WindowsSerialPort serial("COM3", 115200);  // Change COM3 to your port
   ```

## Quick Setup Checklist

- [ ] Install CMake
- [ ] Install C++ compiler (Visual Studio OR MinGW)
- [ ] Verify compiler is in PATH (`g++ --version` or open Developer Command Prompt)
- [ ] Run build script or manual CMake commands
- [ ] Check output in `build/bin/` directory

