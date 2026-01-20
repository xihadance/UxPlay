# Cross-compilation toolchain for UxPlay Windows build from WSL
#
# This file configures CMake to cross-compile UxPlay for Windows x64
# from a WSL (Windows Subsystem for Linux) environment.
#
# Usage:
#   cmake .. -DCMAKE_TOOLCHAIN_FILE=../windows-toolchain.cmake -G Ninja
#

# Target system
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Check if we're in WSL
execute_process(
    COMMAND grep -qi "microsoft" /proc/version
    RESULT_VARIABLE IS_WSL
)

if(NOT IS_WSL EQUAL 0)
    message(WARNING "This toolchain is designed for WSL. Running on native Linux may have issues.")
endif()

# MinGW-w64 cross-compiler (for WSL Ubuntu)
# Note: Install with: sudo apt install gcc-mingw-w64 g++-mingw-w64 mingw-w64-tools
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)

# Bonjour SDK location (on Windows host)
# Adjust this path if your Bonjour SDK is installed elsewhere
if(NOT DEFINED BONJOUR_SDK_HOME)
    set(BONJOUR_SDK_HOME "/mnt/c/Program Files/Bonjour SDK")
endif()

# Verify Bonjour SDK exists
if(EXISTS "${BONJOUR_SDK_HOME}")
    message(STATUS "Bonjour SDK found at: ${BONJOUR_SDK_HOME}")
else()
    message(WARNING "Bonjour SDK not found at: ${BONJOUR_SDK_HOME}")
    message(WARNING "DNS-SD will not work properly without Bonjour SDK")
    message(WARNING "Install from: https://www.softpedia.com/get/Programming/SDK-DDK/Bonjour-SDK.shtml")
endif()

# GStreamer paths for cross-compilation
# These assume you have installed the development packages for cross-compilation
# Install with: sudo apt install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev
set(GSTREAMER_ROOT_DIR "/usr/x86_64-w64-mingw32")
set(GSTREAMER_INCLUDE_DIR "${GSTREAMER_ROOT_DIR}/include/gstreamer-1.0")
set(GSTREAMER_LIBRARY_DIR "${GSTREAMER_ROOT_DIR}/lib")

# OpenSSL paths
# MSYS2 provides OpenSSL, but for cross-compilation from WSL we use system OpenSSL
# Install with: sudo apt install libssl-dev
set(OPENSSL_ROOT_DIR "/usr/x86_64-w64-mingw32")

# libplist paths
# Install with: sudo apt install libplist-dev
set(LIBPLIST_ROOT_DIR "/usr/x86_64-w64-mingw32")

# Search paths for find_* commands
set(CMAKE_FIND_ROOT_PATH
    /usr/x86_64-w64-mingw32
    ${BONJOUR_SDK_HOME}
    ${OPENSSL_ROOT_DIR}
    ${LIBPLIST_ROOT_DIR}
)

# Search behavior
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)      # Don't search for programs in target paths
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)       # Search libraries only in target paths
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)       # Search headers only in target paths
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)       # Search packages only in target paths

# Compiler flags
set(CMAKE_C_FLAGS_INIT
    "-static-libgcc -static-libstdc++"
    CACHE STRING "Initial C compiler flags"
)
set(CMAKE_CXX_FLAGS_INIT
    "-static-libgcc -static-libstdc++"
    CACHE STRING "Initial C++ compiler flags"
)

# Windows-specific flags
add_compile_definitions(
    _WIN32
    __MINGW64__
    __USE_MINGW_ANSI_STDIO=1
)

# Link libraries for Windows
set(CMAKE_EXE_LINKER_FLAGS_INIT
    "-static-libgcc -static-libstdc++"
    CACHE STRING "Initial executable linker flags"
)

# Enable large file support
add_compile_definitions(
    _LARGEFILE64_SOURCE
    _FILE_OFFSET_BITS=64
)

# Package-specific configurations
set(PKG_CONFIG_EXECUTABLE "pkg-config")
set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH ON)

# Adjust pkg-config search path for cross-compilation
set(ENV{PKG_CONFIG_PATH} "/usr/x86_64-w64-mingw32/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
set(ENV{PKG_CONFIG_LIBDIR} "/usr/x86_64-w64-mingw32/lib/pkgconfig")

# Message output
message(STATUS "=== UxPlay Windows Cross-Compilation Toolchain ===")
message(STATUS "Target: Windows x64")
message(STATUS "Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "Bonjour SDK: ${BONJOUR_SDK_HOME}")
message(STATUS "=================================================")

# Additional instructions
if(NOT DEFINED ENV{WSL_DISTRO_NAME})
    message(WARNING "This toolchain is designed for WSL. On native Linux, results may be inconsistent.")
endif()
