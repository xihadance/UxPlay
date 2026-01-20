# Skill: Build UxPlay Windows EXE (WSL + MSYS2)

**Name**: `build_windows_exe`
**Description**: Packages UxPlay as a Windows executable using MSYS2/MinGW-w64 toolchain in WSL

## Purpose

This skill automates the process of building UxPlay for Windows from WSL (Ubuntu) using the MSYS2/MinGW-w64 cross-compilation toolchain, as described in the project's README.md.

## Prerequisites

Before using this skill, ensure:

1. **WSL is installed** on Windows (Ubuntu 20.04+ recommended)
2. **Bonjour SDK for Windows v3.0** is installed on the Windows host
   - Default location: `C:\Program Files\Bonjour SDK`
   - Can be downloaded from: https://www.softpedia.com/get/Programming/SDK-DDK/Bonjour-SDK.shtml
   - Or Apple's developer site (requires registration)
3. **Git** is available in WSL (will be installed if not present)

## Usage

```bash
# Clone the repository
git clone https://github.com/FDH2/UxPlay.git
cd UxPlay

# Run the build script
./build_windows.sh
```

Or, if you want to build from a local source directory:

```bash
# From within the UxPlay source directory
./build_windows.sh /path/to/source
```

## What This Skill Does

### 1. Setup WSL Build Environment

Installs required packages in WSL:
- **MSYS2 UCRT64 toolchain** (MinGW-w64 compiler, CMake, ninja)
- **GStreamer development libraries** for Windows cross-compilation
- **libplist** for Apple plist handling
- **OpenSSL** (already included in MSYS2)
- **Git** (if not already installed)

### 2. Configure Cross-Compilation

Sets up CMake with the correct toolchain for Windows target:
- Uses MinGW-w64 cross-compiler
- Points to Bonjour SDK location
- Configures GStreamer paths for Windows

### 3. Build UxPlay

Executes the build process:
```bash
mkdir -p build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../windows-toolchain.cmake
ninja
```

### 4. Package Results

Creates a distributable package containing:
- `uxplay.exe` - The main executable
- `README.txt` - Usage instructions
- `LICENSE` - GPLv3 license
- Required DLLs (GStreamer, OpenSSL, libplist)
- Optional: GStreamer plugin packages

## Build Output

The build produces:
- **`uxplay.exe`** - Main executable in the `build/` directory
- **`build/dist/`** - Optional distributable package with all dependencies

## Post-Build Verification

After building, you should verify the executable works:
1. Copy `uxplay.exe` to your Windows PATH or run from MSYS2 terminal
2. Ensure Bonjour SDK is installed on Windows
3. Install GStreamer runtime plugins:
   ```bash
   # In MSYS2 UCRT64 terminal
   pacman -S mingw-w64-ucrt-x86_64-gst-plugins-good \
             mingw-w64-ucrt-x86_64-gst-plugins-bad \
             mingw-w64-ucrt-x86_64-gst-libav
   ```

## Troubleshooting

### Bonjour SDK Not Found

If CMake cannot find the Bonjour SDK, set the environment variable:
```bash
export BONJOUR_SDK_HOME="/mnt/c/Program Files/Bonjour SDK"
```

Or specify it in the CMake command:
```bash
cmake .. -DBONJOUR_SDK_HOME="/mnt/c/Program Files/Bonjour SDK"
```

### Missing Dependencies

If you encounter missing DLL errors when running uxplay.exe, install additional GStreamer packages:
```bash
pacman -S mingw-w64-ucrt-x86_64-gst-plugins-good \
          mingw-w64-ucrt-x86_64-gst-plugins-bad \
          mingw-w64-ucrt-x86_64-gst-libav
```

### Cross-Compilation Issues

If you encounter cross-compilation errors, check:
1. WSL architecture matches (x86_64)
2. Bonjour SDK is accessible from WSL (`/mnt/c/...`)
3. All MSYS2 packages are up to date:
   ```bash
   pacman -Syu
   ```

## Alternative: Manual Build Steps

If you prefer manual control, here are the steps this skill automates:

### Step 1: Setup MSYS2 in WSL

```bash
# Install MSYS2 UCRT64 toolchain
sudo apt update
sudo apt install -y wget

# Download and extract MSYS2 UCRT64
wget https://github.com/msys2/msys2-installer/releases/download/2024-07-09/msys2-x86_64-20240709.exe
# Note: The .exe needs to be extracted on Windows side, then accessible from WSL
```

**Important**: MSYS2 UCRT64 is designed for Windows. In WSL, you'd typically use native Linux tools or cross-compile.

### Step 2: Install WSL Build Dependencies

```bash
sudo apt update
sudo apt install -y \
    gcc-mingw-w64 \
    g++-mingw-w64 \
    cmake \
    ninja-build \
    git \
    libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    libssl-dev \
    libplist-dev
```

### Step 3: Configure for Cross-Compilation

Create `windows-toolchain.cmake`:
```cmake
# Cross-compilation toolchain for Windows x64
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Compiler
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)

# Bonjour SDK path (adjust as needed)
set(BONJOUR_SDK_HOME "/mnt/c/Program Files/Bonjour SDK")

# Find libraries
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
```

### Step 4: Build

```bash
mkdir -p build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../windows-toolchain.cmake -G Ninja
ninja
```

## Important Notes

1. **Cross-compilation limitations**: WSL + MinGW-w64 cross-compilation works, but some Windows-specific features (like Bonjour DNS-SD) may need careful linking.

2. **Runtime dependencies**: The built `uxplay.exe` will still require:
   - Bonjour SDK (Bonjour64.dll)
   - GStreamer runtime DLLs
   - libplist DLLs
   - OpenSSL DLLs

3. **Distribution**: For distributing to other Windows users, consider:
   - Including all required DLLs in the package
   - Using an installer (NSIS, Inno Setup)
   - Or direct users to install MSYS2 runtime dependencies

4. **Alternative approach**: The official README recommends building directly on Windows using MSYS2, which is simpler and more reliable than cross-compiling from WSL.

## Related Files

- `CMakeLists.txt` - Main CMake configuration
- `README.md` - Detailed Windows build instructions
- `CLAUDE.md` - Project architecture and build information
- `windows-toolchain.cmake` (to be created) - Cross-compilation toolchain file
