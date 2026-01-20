# UxPlay Windows Build Skill

This skill provides commands to build UxPlay for Windows as an executable file (.exe) using cross-compilation from WSL (Windows Subsystem for Linux).

## Overview

This skill automates the process of:
1. Setting up WSL build environment with MinGW-w64 cross-compiler
2. Installing necessary dependencies (GStreamer, OpenSSL, libplist, Bonjour SDK)
3. Configuring CMake for Windows cross-compilation
4. Building UxPlay with Ninja build system
5. Creating a distributable package with the executable

## Requirements

### Windows Host
- **Bonjour SDK for Windows v3.0** (Required for AirPlay/mDNS discovery)
  - Download from: https://www.softpedia.com/get/Programming/SDK-DDK/Bonjour-SDK.shtml
  - Default location: `C:\Program Files\Bonjour SDK`

### WSL (Ubuntu 20.04+ recommended)
- WSL2 enabled on Windows
- Git
- Internet connection for downloading dependencies

## Usage

### From Claude Code

The skill is automatically available in the UxPlay repository. You can call it with:

```bash
# Full build
build-windows

# Build from specific source directory
build-windows /path/to/source

# Clean and rebuild
build-windows --clean

# Install dependencies only
build-windows --dependencies

# Verbose output
build-windows --verbose
```

### Manual Execution

You can also run the build script directly:

```bash
# From the UxPlay repository root
./build_windows.sh

# With options
./build_windows.sh --clean
./build_windows.sh --dependencies
```

## What Gets Built

### Output Files
```
build/
├── uxplay.exe              # Main Windows executable
├── dist/                   # Distribution package
│   ├── uxplay.exe
│   ├── README.txt
│   ├── LICENSE
│   └── BUILD_INFO.txt
└── windows-toolchain.cmake # CMake toolchain file
```

### Key Components
- **uxplay.exe**: The main executable, built with MinGW-w64 cross-compiler
- **Dependencies**: The executable will require runtime DLLs:
  - Bonjour64.dll (from Bonjour SDK)
  - GStreamer DLLs (from GStreamer installation)
  - libplist and OpenSSL DLLs

## Build Process Details

### Phase 1: Setup WSL Environment
- Installs MinGW-w64 cross-compiler (gcc-mingw-w64, g++-mingw-w64)
- Installs CMake, Ninja, and other build tools
- Installs GStreamer development libraries for Windows target
- Installs OpenSSL and libplist development files

### Phase 2: Configure Cross-Compilation
- Creates a CMake toolchain file (`windows-toolchain.cmake`)
- Sets up MinGW-w64 as the cross-compiler
- Configures paths to Bonjour SDK and other dependencies
- Enables Windows-specific compilation flags

### Phase 3: Build
- Runs CMake to generate build files
- Builds UxPlay with Ninja
- Compiles all source files in the `lib/` and `renderers/` directories
- Links against cross-compiled libraries

### Phase 4: Package
- Creates a distribution directory
- Copies the executable, README, LICENSE, and build info
- Produces a ready-to-use package

## Testing the Executable

### Option 1: Test in WSL with Wine (Limited)
```bash
# Install Wine in WSL
sudo apt install wine64

# Run with Wine
wine build/uxplay.exe
```
**Note**: DNS-SD/mDNS won't work in WSL. Use this only for basic testing.

### Option 2: Run on Actual Windows
1. Copy `build/dist/` to Windows
2. Install prerequisites:
   - **Bonjour SDK**: Required for mDNS/DNS-SD
   - **GStreamer**: Download MSI installer from https://gstreamer.freedesktop.org/download/
3. Run: `uxplay.exe`

## Troubleshooting

### Bonjour SDK Not Found
```
[WARN] Bonjour SDK not found at: /mnt/c/Program Files/Bonjour SDK
```
**Solution**: Install Bonjour SDK or set `BONJOUR_SDK_HOME` environment variable:
```bash
export BONJOUR_SDK_HOME="/mnt/c/Path/To/Bonjour SDK"
```

### Missing Dependencies
```
x86_64-w64-mingw32-gcc: command not found
```
**Solution**: Install MinGW-w64:
```bash
sudo apt install gcc-mingw-w64 g++-mingw-w64
```

### CMake Can't Find GStreamer
```
-- Could NOT find GStreamer
```
**Solution**: Install GStreamer development packages:
```bash
sudo apt install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev
```

### Linking Errors
```
undefined reference to `gst_...`
```
**Solution**: Ensure GStreamer libraries are properly cross-compiled. Consider building directly on Windows with MSYS2 instead.

## Limitations

1. **Cross-compilation complexity**: Building Windows binaries from WSL is more complex than native Windows builds
2. **Runtime dependencies**: The built executable still requires Windows runtime DLLs
3. **Testing**: Some features (like DNS-SD) won't work when testing in WSL
4. **Compatibility**: Some Windows-specific optimizations may not be available

## Alternative: Direct Windows Build

For production builds, the official README recommends building directly on Windows using MSYS2:

1. Install MSYS2 from https://www.msys2.org/
2. Open MSYS2 UCRT64 terminal
3. Install tools: `pacman -S mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-gcc`
4. Install dependencies: `pacman -S mingw-w64-ucrt-x86_64-libplist mingw-w64-ucrt-x86_64-gstreamer`
5. Build: `cmake .. && ninja`

This approach is simpler and more reliable for production use.

## Files in this Skill

- `skill.js`: JavaScript implementation of the skill (for Claude Code)
- `build-windows`: Bash CLI entry point
- `package.json`: Skill metadata and configuration
- `README.md`: This documentation file
- `build_windows.sh`: Main build script (in repository root)
- `windows-toolchain.cmake`: CMake toolchain file (in repository root)
- `build_windows_README.md`: Detailed build guide (in repository root)

## License

This skill is part of UxPlay and is licensed under GPLv3.
