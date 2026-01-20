# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What is UxPlay?

UxPlay is an open-source AirPlay mirroring server for Linux, macOS, *BSD, and Windows. It acts like an AppleTV, allowing iOS/iPadOS/macOS devices to mirror their screens (with audio) to a computer running UxPlay.

## Build System

UxPlay uses **CMake** (v3.10+ on Linux, v3.13+ on macOS) as its build system.

### Common Build Commands

```bash
# Basic build (in-source)
cmake .
make
sudo make install
sudo make uninstall  # to remove

# Recommended: out-of-source build
mkdir build
cd build
cmake ..
make
sudo make install

# Clean build (after modifying source)
rm -rf build
mkdir build
cd build
cmake ..
make
```

### CMake Build Options

| Option | Purpose |
|--------|---------|
| `-DNO_X11_DEPS=ON` | Build without X11 dependency |
| `-DUSE_X11=ON` | Force X11 usage even if not detected |
| `-DNO_MARCH_NATIVE=ON` | Disable `-march=native` optimization |
| `-DGST_MACOS=ON` | Force macOS GStreamer compatibility |

### Platform-Specific Builds

**Linux/macOS/BSD:**
- Requires GStreamer 1.14+ (1.20+ recommended)
- Requires OpenSSL 1.1.1+
- Requires libplist 2.0+
- Requires dnssd implementation (Avahi on Linux, Bonjour on macOS)

**Windows (MSYS2/MinGW-64):**
- Use MSYS2 UCRT64 environment
- Requires Bonjour SDK for DNS-SD
- Uses ninja instead of make

## Project Architecture

### High-Level Structure

```
UxPlay/
├── CMakeLists.txt              # Root CMake configuration
├── uxplay.cpp                  # Main entry point and application logic
├── lib/                        # Core AirPlay protocol library
│   ├── CMakeLists.txt          # libairplay static library build
│   ├── raop.h/.c               # RAOP (AirPlay) protocol implementation
│   ├── dnssd.h/.c              # DNS-SD service discovery
│   ├── httpd.h/.c              # HTTP server for AirPlay
│   ├── raop_rtp.h/.c           # RTP (video/audio streaming)
│   ├── raop_rtp_mirror.h/.c    # Mirror mode RTP handling
│   ├── raop_ntp.h/.c           # NTP time synchronization
│   ├── pairing.h/.c            # AirPlay pairing/encryption
│   ├── crypto.h/.c             # Crypto utilities (FairPlay)
│   ├── airplay_video.h/.c      # Video streaming storage
│   ├── lib/playfair/           # FairPlay encryption (3rd party)
│   └── lib/llhttp/             # HTTP parser (3rd party)
├── renderers/                  # GStreamer-based renderers
│   ├── CMakeLists.txt          # renderers static library build
│   ├── video_renderer.h/.c     # H264/H265 video rendering via GStreamer
│   └── audio_renderer.h/.c     # Audio rendering via GStreamer
└── README.md                   # Comprehensive user documentation
```

### Key Data Flow

1. **Client Connection**: iOS device discovers UxPlay via DNS-SD (mDNS)
2. **Handshake**: Pairing, encryption setup, pin/password authentication
3. **RAOP Stream**: Video and audio data received via RTP packets
4. **Processing**: Video decoded by GStreamer pipeline, audio processed
5. **Rendering**: GStreamer sinks display video/audio

### Core Components

#### uxplay.cpp (Main Application)
- Command-line argument parsing
- GStreamer initialization
- Main event loop (GLib/GMainLoop)
- Callback registration for audio/video processing
- Configuration management

#### lib/raop.c (RAOP Protocol)
- Manages RAOP connections and state
- HTTP server for control commands
- RTP session management
- Callbacks for video/audio processing (registered from uxplay.cpp)

#### renderers/video_renderer.c
- GStreamer pipeline construction and management
- Supports h264parse, decodebin, videoconvert, various sinks
- Optional HLS streaming support (YouTube)
- Cover art display
- Video RTP to external pipeline forwarding (-vrtp)

#### renderers/audio_renderer.c
- GStreamer pipeline for audio playback
- Supports ALAC (Apple Lossless) and AAC
- Audio RTP forwarding (-artp)
- Volume control

## Development Workflow

### Adding New Features

1. **Understand existing patterns**: Study callback structure in `lib/raop.h` and `uxplay.cpp`
2. **Modify callbacks**: Add new callbacks in `raop_callbacks_s` struct
3. **Implement in main**: Add handler functions in `uxplay.cpp`
4. **Update parsers**: If adding command-line options, modify `parse_arguments()` in `uxplay.cpp`

### Debugging

- Enable debug logging with `-debug` option
- GStreamer debug: `GST_DEBUG=3 uxplay`
- Video/audio dump: `-vd` and `-ad` options
- Check logs for connection issues, decoding problems

### Testing

The project does not have automated unit tests. Manual testing requires:
- iOS/iPadOS/macOS device with AirPlay client capability
- Network with mDNS/DNS-SD support (or Bluetooth LE beacon)
- Verify screen mirroring and audio playback

### Common Development Areas

1. **Adding new codecs**: Modify `video_renderer.c` and `audio_renderer.c`
2. **Adding new sinks**: Update default videosink/audiosink options
3. **Protocol enhancements**: Modify `lib/raop_rtp.c` and related handlers
4. **New command-line options**: Update `parse_arguments()` and global config vars in `uxplay.cpp`

## Dependencies

### Runtime Dependencies
- **GStreamer 1.14+** (video/audio rendering)
  - Core, base, good plugins
  - Optional: gstreamer-libav for H265 decoding
- **OpenSSL 1.1.1+** (crypto, TLS)
- **libplist 2.0+** (Apple plist handling)
- **DNS-SD implementation**:
  - Linux/*BSD: Avahi (avahi-compat-libdns_sd) or mDNSResponder
  - macOS: Bonjour (built-in)
  - Windows: Bonjour SDK

### Build Dependencies
- C++ compiler (g++, clang, or MinGW-64)
- CMake
- pkg-config
- Development headers for dependencies

## Key Files to Reference

- **lib/raop.h** - Public API for RAOP protocol (callbacks, structures)
- **lib/stream.h** - Video/audio data structures (video_decode_struct, audio_decode_struct)
- **renderers/video_renderer.h** - Video renderer API
- **renderers/audio_renderer.h** - Audio renderer API
- **lib/dnssd.h** - DNS-SD service discovery API

## Important Notes

- The main loop uses **GLib/GMainLoop** (not a simple while loop)
- Audio and video are processed in separate threads via RTP callbacks
- GStreamer pipelines are user-configurable via command-line options
- Security features: pin/password auth, client allow/block lists
- AirPlay2 multi-room audio is NOT supported (use shairport-sync instead)
- Video RTP can be forwarded to external applications (OBS, etc.) via `-vrtp`
- Audio RTP can be forwarded via `-artp`
- HLS streaming (YouTube) requires `-hls` option

## Repository Information

- **Upstream**: https://github.com/FDH2/UxPlay
- **License**: GPLv3
- **Original based on**: RPiPlay, shairplay, playfair
