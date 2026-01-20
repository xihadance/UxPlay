# UxPlay Windows EXE Build Guide

This guide explains how to build UxPlay as a Windows executable from a WSL (Windows Subsystem for Linux) environment using cross-compilation.

## Quick Start

```bash
# Make the script executable
chmod +x build_windows.sh

# Run the full build process
./build_windows.sh
```

## Prerequisites

### 1. Windows Host Setup

**Bonjour SDK for Windows v3.0** (Required for DNS-SD/mDNS service discovery):
- Download from: https://www.softpedia.com/get/Programming/SDK-DDK/Bonjour-SDK.shtml
- Default installation path: `C:\Program Files\Bonjour SDK`
- Note: This SDK is required for AirPlay discovery. Without it, clients may not find your UxPlay server.

### 2. WSL Setup (Ubuntu 20.04+ recommended)

First, enable WSL and install Ubuntu:
1. Open PowerShell as Administrator:
   ```powershell
   wsl --install -d Ubuntu-22.04
   ```
2. Restart your computer when prompted
3. Set up your Ubuntu user account

Update WSL packages:
```bash
sudo apt update
sudo apt upgrade -y
```

## Build Process

The build script automates everything:

```bash
./build_windows.sh
```

### What This Does:

1. **Installs cross-compilation tools**:
   - MinGW-w64 cross-compiler (gcc, g++)
   - CMake
   - Ninja build system
   - Git

2. **Installs dependencies**:
   - GStreamer development libraries
   - OpenSSL development files
   - libplist development files

3. **Creates a CMake toolchain file** for Windows cross-compilation

4. **Configures and builds UxPlay**:
   - Creates `build/` directory
   - Runs CMake with cross-compilation settings
   - Builds with Ninja

5. **Packages the results**:
   - Creates `build/dist/` with executable and documentation

## Build Options

The `build_windows.sh` script supports several modes:

```bash
# Full build (default)
./build_windows.sh

# Install dependencies only
./build_windows.sh --dependencies

# Clean build directory
./build_windows.sh --clean

# Build from specific source directory
./build_windows.sh /path/to/uxplay/source

# Create package only (assumes build already done)
./build_windows.sh --package
```

## Output Files

After a successful build, you'll find:

```
build/
├── uxplay.exe          # Main executable (or just 'uxplay' if not .exe)
├── build/dist/         # Distribution package
│   ├── uxplay.exe
│   ├── README.txt
│   ├── LICENSE
│   └── BUILD_INFO.txt
└── windows-toolchain.cmake  # CMake toolchain file
```

## Testing the Executable

### Option 1: Test in WSL (Limited)

You can run the executable in WSL with Wine, but some Windows-specific features (like Bonjour) won't work:

```bash
# Install Wine (optional)
sudo apt install wine64

# Run with Wine
wine build/uxplay.exe
```

**Note**: Testing in WSL is limited. For proper testing, you need to run on actual Windows.

### Option 2: Run on Windows

1. Copy the executable to Windows:
   ```powershell
   # From PowerShell, copy from WSL
   wsl cp ~/UxPlay/build/uxplay.exe C:\temp\
   ```

2. Install GStreamer runtime (if not already installed):
   Download from: https://gstreamer.freedesktop.org/download/
   - Select the Windows MSI installer (64-bit)
   - Install "Complete" or at minimum the "Base" and "Good" plugins

3. Install Bonjour SDK (if not already done)
   - Required for mDNS/DNS-SD service discovery
   - Without it, iOS devices may not automatically find the server

4. Run from Windows Command Prompt or PowerShell:
   ```powershell
   # Open Windows Firewall if prompted
   C:\temp\uxplay.exe -n "MyUxPlayServer"
   ```

## Manual Build Steps

If you prefer more control, you can manually execute the build steps:

### 1. Install Dependencies

```bash
sudo apt update
sudo apt install -y \
    gcc-mingw-w64 \
    g++-mingw-w64 \
    mingw-w64-tools \
    cmake \
    ninja-build \
    git \
    libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    libssl-dev \
    libplist-dev
```

### 2. Create Toolchain File

Use the provided `windows-toolchain.cmake` or create your own:
```bash
# Make sure you're in the UxPlay source directory
# The toolchain file is already there if you ran the script
```

### 3. Configure

```bash
mkdir -p build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../windows-toolchain.cmake -G Ninja
```

### 4. Build

```bash
ninja
```

### 5. Package

```bash
mkdir -p dist
cp uxplay dist/
cd dist
# Add README, LICENSE, etc.
```

## Troubleshooting

### Error: "Bonjour SDK not found"

**Solution**: Either:
1. Install Bonjour SDK to the default location (`C:\Program Files\Bonjour SDK`)
2. Set `BONJOUR_SDK_HOME` environment variable:
   ```bash
   export BONJOUR_SDK_HOME="/mnt/c/Path/To/Bonjour SDK"
   ./build_windows.sh
   ```

### Error: Missing GStreamer libraries

**Solution**: Ensure you have the GStreamer development packages:
```bash
sudo apt install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev
```

### Error: Cross-compilation failures

**Solution**: This build uses MinGW-w64 for cross-compilation. Some libraries may not be available in cross-compile form. In that case:

1. **Option A**: Build directly on Windows using MSYS2 (recommended for production)
   - See README.md section "Building UxPlay on Microsoft Windows"

2. **Option B**: Use a different cross-compilation approach (more advanced)

### Windows Defender Blocking

**Solution**: Windows Defender or antivirus may flag the executable. Add an exception:
- Windows Security → Virus & threat protection → Manage settings → Exclusions

## Important Notes

1. **Cross-compilation limitations**: Cross-compiling from WSL is possible but not officially supported. For production use, building directly on Windows (MSYS2) is recommended.

2. **Dependencies**: The built executable still needs runtime DLLs:
   - `Bonjour64.dll` (from Bonjour SDK)
   - GStreamer DLLs (from GStreamer installation)
   - libplist DLLs
   - OpenSSL DLLs

3. **Distribution**: To distribute to other users:
   - Include all required DLLs in your package, OR
   - Provide an installer that installs dependencies, OR
   - Direct users to install MSYS2 runtime packages

4. **Alternative approach**: The official README recommends MSYS2 on Windows, which provides a more integrated and tested build environment.

## Comparison: Build Methods

| Method | Pros | Cons | Recommended For |
|--------|------|------|-----------------|
| **WSL + MinGW** | Use familiar Linux environment | Complex setup, less tested | Development, testing |
| **MSYS2 on Windows** | Officially supported, well-tested | Requires Windows environment | Production builds |
| **Docker** | Reproducible builds | More complex, Docker on Windows | CI/CD, team development |

## Links

- **UxPlay GitHub**: https://github.com/FDH2/UxPlay
- **Bonjour SDK**: https://www.softpedia.com/get/Programming/SDK-DDK/Bonjour-SDK.shtml
- **GStreamer**: https://gstreamer.freedesktop.org/download/
- **MSYS2**: https://www.msys2.org/

## Support

For issues with this build script:
1. Check the troubleshooting section above
2. Verify all prerequisites are installed
3. Check the UxPlay GitHub issues: https://github.com/FDH2/UxPlay/issues
4. For cross-compilation specific issues, refer to WSL and MinGW documentation
