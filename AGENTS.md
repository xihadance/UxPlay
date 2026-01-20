# AGENTS.md

This file provides guidance to automated coding agents working in this
repository. Keep edits small, prefer CMake out-of-source builds, and avoid
changing generated artifacts unless asked.

## Repo overview

- UxPlay is an AirPlay mirroring server written in C/C++ with GStreamer.
- Key source: `uxplay.cpp`, `lib/`, `renderers/`, `CMakeLists.txt`.

## Build (Linux/WSL/macOS)

```bash
mkdir -p build
cd build
cmake ..
make
```

Optional CMake flags:
- `-DNO_X11_DEPS=ON`
- `-DUSE_X11=ON`
- `-DNO_MARCH_NATIVE=ON`
- `-DGST_MACOS=ON`

## Windows build (MSYS2/MinGW)

Use MSYS2 UCRT64 with CMake + Ninja and Bonjour SDK. See
`build_windows_README.md` for details.

## Development notes

- Main flow: `uxplay.cpp` parses args and wires callbacks into `lib/raop.*`.
- Rendering is handled by `renderers/video_renderer.*` and
  `renderers/audio_renderer.*`.
- There are no automated tests; manual testing requires an AirPlay-capable
  device on a network with mDNS/DNS-SD.

## Editing guidance

- Prefer minimal, targeted changes.
- Follow existing code style and patterns.
- Avoid reformatting unrelated code.
