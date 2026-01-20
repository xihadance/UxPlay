#!/bin/bash

# UxPlay Windows EXE Build Script
# This script builds UxPlay for Windows using cross-compilation from WSL (Ubuntu)
# Prerequisites: WSL with Ubuntu, Bonjour SDK installed on Windows host

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

log() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

check_bonjour_sdk() {
    local bonjour_path="$HOME/../mnt/c/Program Files/Bonjour SDK"

    if [ -d "$bonjour_path" ]; then
        log "Bonjour SDK found at: $bonjour_path"
        BONJOUR_SDK_HOME="$bonjour_path"
        return 0
    fi

    # Check alternative locations
    local alt_paths=(
        "/mnt/c/Program Files/Bonjour SDK"
        "/mnt/c/Program Files (x86)/Bonjour SDK"
    )

    for path in "${alt_paths[@]}"; do
        if [ -d "$path" ]; then
            log "Bonjour SDK found at: $path"
            BONJOUR_SDK_HOME="$path"
            return 0
        fi
    done

    error "Bonjour SDK not found!"
    warn "Please install Bonjour SDK for Windows from:"
    warn "  https://www.softpedia.com/get/Programming/SDK-DDK/Bonjour-SDK.shtml"
    warn "  or from Apple's developer site"
    warn ""
    warn "If Bonjour SDK is installed elsewhere, set BONJOUR_SDK_HOME environment variable"
    warn "Example: export BONJOUR_SDK_HOME=\"/mnt/c/Program Files/Bonjour SDK\""
    return 1
}

check_wsl() {
    if ! grep -qEi "(Microsoft|WSL)" /proc/version 2>/dev/null; then
        warn "This script is designed for WSL (Windows Subsystem for Linux)"
        warn "Running on native Linux may have issues with Windows-specific dependencies"
    fi
}

install_wsl_dependencies() {
    log "Updating package lists..."
    sudo apt-get update

    log "Installing cross-compilation toolchain..."
    sudo apt-get install -y \
        gcc-mingw-w64 \
        g++-mingw-w64 \
        mingw-w64-tools \
        cmake \
        ninja-build \
        git \
        wget \
        pkg-config \
        libgstreamer1.0-dev \
        libgstreamer-plugins-base1.0-dev \
        libssl-dev \
        libplist-dev \
        libplist-utils \
        dbus

    log "Cross-compilation toolchain installed successfully"
}

create_toolchain_file() {
    local toolchain_file="windows-toolchain.cmake"

    cat > "$toolchain_file" << 'EOF'
# Cross-compilation toolchain for Windows x64 from WSL
# This file configures CMake to build Windows executables

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# MinGW-w64 cross-compiler
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)

# Bonjour SDK path (adjust if needed)
if(NOT DEFINED BONJOUR_SDK_HOME)
    set(BONJOUR_SDK_HOME "/mnt/c/Program Files/Bonjour SDK")
endif()

# GStreamer paths for Windows (adjust if needed)
set(GSTREAMER_ROOT_DIR "/usr/x86_64-w64-mingw32")
set(GSTREAMER_INCLUDE_DIR "/usr/x86_64-w64-mingw32/include/gstreamer-1.0")
set(GSTREAMER_LIBRARY_DIR "/usr/x86_64-w64-mingw32/lib")

# OpenSSL paths for Windows
set(OPENSSL_ROOT_DIR "/usr/x86_64-w64-mingw32")

# libplist paths for Windows
set(LIBPLIST_ROOT_DIR "/usr/x86_64-w64-mingw32")

# Search paths
set(CMAKE_FIND_ROOT_PATH
    /usr/x86_64-w64-mingw32
    ${BONJOUR_SDK_HOME}
)

# Search behavior
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Compiler flags for Windows
set(CMAKE_C_FLAGS "-static-libgcc -static-libstdc++" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS "-static-libgcc -static-libstdc++" CACHE STRING "" FORCE)
EOF

    log "Toolchain file created: $toolchain_file"
}

configure_build() {
    local source_dir="$1"
    local build_dir="build"

    if [ ! -d "$source_dir" ]; then
        error "Source directory not found: $source_dir"
        return 1
    fi

    log "Configuring UxPlay for Windows..."
    log "Source directory: $source_dir"
    log "Build directory: $build_dir"
    log "Bonjour SDK: $BONJOUR_SDK_HOME"

    # Create build directory
    mkdir -p "$build_dir"
    cd "$build_dir"

    # Configure with CMake
    if [ -f "../windows-toolchain.cmake" ]; then
        cmake "$source_dir" \
            -DCMAKE_TOOLCHAIN_FILE="../windows-toolchain.cmake" \
            -G Ninja \
            -DCMAKE_BUILD_TYPE=Release \
            -DBONJOUR_SDK_HOME="$BONJOUR_SDK_HOME" \
            -DCMAKE_INSTALL_PREFIX="./dist"
    else
        # Fallback: attempt native build
        warn "Toolchain file not found, attempting native build..."
        cmake "$source_dir" \
            -G Ninja \
            -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_INSTALL_PREFIX="./dist"
    fi
}

build_project() {
    log "Building UxPlay..."

    if [ ! -d "build" ]; then
        error "Build directory not found. Run configure first."
        return 1
    fi

    cd build

    # Build with ninja
    ninja

    if [ $? -eq 0 ]; then
        log "Build completed successfully!"

        # Verify executable exists
        if [ -f "uxplay" ] || [ -f "uxplay.exe" ]; then
            log "Executable created: $(ls uxplay* 2>/dev/null | head -1)"
        else
            warn "Executable not found in build directory"
        fi
    else
        error "Build failed!"
        return 1
    fi
}

package_executable() {
    local build_dir="build"
    local package_dir="dist"

    if [ ! -d "$build_dir" ]; then
        error "Build directory not found"
        return 1
    fi

    cd "$build_dir"

    log "Creating distribution package..."
    mkdir -p "$package_dir"

    # Copy executable
    local exe_file=""
    if [ -f "uxplay" ]; then
        exe_file="uxplay"
    elif [ -f "uxplay.exe" ]; then
        exe_file="uxplay.exe"
    fi

    if [ -n "$exe_file" ]; then
        cp "$exe_file" "$package_dir/"
        log "Copied $exe_file to package"
    fi

    # Copy documentation
    cd ..
    if [ -f "README.md" ]; then
        cp "README.md" "$build_dir/$package_dir/README.txt"
        log "Copied README to package"
    fi

    if [ -f "LICENSE" ]; then
        cp "LICENSE" "$build_dir/$package_dir/"
        log "Copied LICENSE to package"
    fi

    # Create package info file
    cat > "$build_dir/$package_dir/BUILD_INFO.txt" << EOF
UxPlay Windows Build
====================
Build Date: $(date)
Build Host: $(uname -a)
Source: $(git rev-parse HEAD 2>/dev/null || echo "unknown")
Compiler: $(x86_64-w64-mingw32-gcc --version | head -1)
CMake: $(cmake --version | head -1)
Ninja: $(ninja --version)

Requirements:
- Bonjour SDK for Windows (Bonjour64.dll)
- GStreamer 1.14+ runtime
- libplist
- OpenSSL

Note: This is a cross-compiled build from WSL.
For best results, build directly on Windows using MSYS2.
EOF

    log "Package created in: $build_dir/$package_dir/"
    log "Contents:"
    ls -la "$build_dir/$package_dir/"
}

show_usage() {
    cat << 'EOF'
Usage: ./build_windows.sh [OPTIONS] [SOURCE_DIR]

Build UxPlay for Windows from WSL using cross-compilation.

Options:
  -h, --help          Show this help message
  -p, --package       Build and create distribution package only
  -c, --clean         Clean build directory
  -d, --dependencies  Install WSL dependencies only
  -v, --verbose       Enable verbose output

Examples:
  ./build_windows.sh                    # Full build
  ./build_windows.sh ./src              # Build from specific source
  ./build_windows.sh -d                 # Install dependencies only
  ./build_windows.sh -c                 # Clean and rebuild

Environment Variables:
  BONJOUR_SDK_HOME    Path to Bonjour SDK (auto-detected if not set)
  GSTREAMER_ROOT_DIR  Path to GStreamer installation (for cross-compilation)

EOF
}

main() {
    local action="full"
    local source_dir="."

    # Parse arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_usage
                exit 0
                ;;
            -p|--package)
                action="package"
                shift
                ;;
            -c|--clean)
                action="clean"
                shift
                ;;
            -d|--dependencies)
                action="dependencies"
                shift
                ;;
            -v|--verbose)
                set -x
                shift
                ;;
            -*)
                error "Unknown option: $1"
                show_usage
                exit 1
                ;;
            *)
                source_dir="$1"
                shift
                ;;
        esac
    done

    # Check WSL
    check_wsl

    # Detect Bonjour SDK
    if ! check_bonjour_sdk; then
        warn "Proceeding without Bonjour SDK (DNS-SD will not work)"
    fi

    case $action in
        dependencies)
            install_wsl_dependencies
            ;;
        clean)
            log "Cleaning build directory..."
            rm -rf build
            rm -f windows-toolchain.cmake
            log "Cleaned"
            ;;
        package)
            if [ ! -d "build" ]; then
                error "Build directory not found. Run full build first."
                exit 1
            fi
            package_executable
            ;;
        full)
            log "=== UxPlay Windows Build ==="
            log "Source: $source_dir"
            log "Date: $(date)"
            log ""

            install_wsl_dependencies
            create_toolchain_file
            configure_build "$source_dir"
            build_project
            package_executable

            log ""
            log "=== Build Complete ==="
            log "Executable: build/uxplay or build/uxplay.exe"
            log "Package: build/dist/"
            log ""
            log "Next steps:"
            log "1. Test the executable in Windows"
            log "2. Install GStreamer runtime plugins if needed"
            log "3. Deploy to Windows users"
            ;;
    esac
}

# Run main function
main "$@"
