# AGENTS.md

This file provides guidance to Codex (Codex.ai/code) when working with code in this repository.

## Project Overview

SDLPlayer is a C++ media player for Windows and macOS. The FFmpeg player core is shared. The product UI is a Qt 6.9 Quick/QML shell (`SDLPlayer`). The original self-drawn SDL UI remains as the legacy `SDLPlayerSdl` target. The project uses C++20.

## Build Commands

Prerequisites: CMake 3.25+, vcpkg. Windows uses Visual Studio 2022 or 2026; macOS uses Apple Clang + Ninja (Apple Silicon first).

```powershell
# Set VCPKG_ROOT (required per terminal session, unless using local presets)
$env:VCPKG_ROOT="<your-vcpkg-path>"

# Configure + build (VS 2026)
cmake --preset msvc2026-debug
cmake --build --preset build-msvc2026-debug

# Configure + build (VS 2022)
cmake --preset msvc2022-debug
cmake --build --preset build-msvc2022-debug

# Release builds (swap "debug" for "release" in both preset names)
cmake --preset msvc2026-release
cmake --build --preset build-msvc2026-release

# Run (accepts an optional file path argument)
.\build\msvc2026-debug\Debug\SDLPlayer.exe [path-to-media-file]

# Run automated tests after building
ctest --preset test-msvc2026-debug
```

```bash
# macOS Qt shell (primary) — official Qt 6.9 + Homebrew FFmpeg, no vcpkg
cmake --preset macos-qt-debug
cmake --build --preset build-macos-qt-debug
ctest --preset test-macos-qt-debug
./build/macos-qt-debug/SDLPlayer [path-to-media-file]

# macOS legacy SDL shell via vcpkg
export VCPKG_ROOT="<your-vcpkg-path>"
cmake --preset macos-arm64-debug
cmake --build --preset build-macos-arm64-debug
./build/macos-arm64-debug/SDLPlayerSdl [path-to-media-file]
```

To avoid the `VCPKG_ROOT` environment variable, copy `CMakeUserPresets.json.example` to `CMakeUserPresets.json` and set the vcpkg path, then use the `-local` preset variants (e.g. `msvc2026-debug-local` or `macos-arm64-debug-local`).

For Chinese-localized MSVC, presets set `VSLANG=1033` to force English. If FFmpeg fails to compile due to Chinese characters in `config.h`'s `CC_IDENT`, patch the vcpkg port to disable the offending filter.

Automated tests are configured through CTest. `SDLPlayerTests` is a lightweight C++ test executable that currently covers no-media `Player` state, speed selection, and invalid open error handling without requiring sample media or an SDL window.

## Architecture

The codebase keeps a portable player core and a replaceable SDL shell:

### UI layer (`src/main.cpp`)
- SDL window creation, event loop, and all rendering (buttons, progress bar, text, video viewport)
- Applies HiDPI/Retina mapping via `ui::ApplyWindowLogicalScale` so layout and mouse events stay in window coordinates
- Handles keyboard shortcuts and mouse interaction (progress bar drag-to-seek)
- Accepts an optional command-line media path on launch
- Calls into `player::Player` for all media operations

### Platform shims (`src/platform/`)
- `ShowOpenMediaFileDialog`, `OpenUiFont`, and `CommandLineMediaPath` are implemented per OS (Win32 / NSOpenPanel / Linux zenity fallback)
- Fonts come from system paths (Windows: msyh / segoeui / arial; macOS: PingFang / Heiti / Helvetica / SF). Do not copy vendor fonts into the repo.
- Keep `#ifdef` out of the event loop; add new platform files instead.

### Output interfaces (`src/player/IAudioOutput.h`, `src/player/IVideoOutput.h`)
- Player talks only to these interfaces, not to `SDL_Renderer` or SDL audio APIs
- Current backends are `src/output/SdlAudioOutput` and `src/output/SdlVideoOutput`
- Do not add a Qt backend until Phase 2

### Player engine (`src/player/Player.h` + `src/player/Player.cpp`)
- Public API: `Player` class using the pimpl idiom (`Player::Impl`)
- `SetAudioOutput` / `SetVideoOutput` inject backends; tests can construct a `Player` with no outputs
- `Player::Impl` contains all FFmpeg state, queues, and threading logic

### Tests (`tests/player/PlayerTests.cpp`)
- CTest entry point registered as `SDLPlayerTests`
- Uses a small hand-written assertion harness rather than a third-party framework
- Avoids media fixtures so it can run immediately after a normal build

**Threading model (3 threads):**
1. **Main thread** — SDL event loop, UI drawing, calls `Player::Update()` to select the video frame closest to the audio clock
2. **Worker thread** (`DecodeLoop`) — reads packets via `av_read_frame`, decodes audio/video, resamples/converts, pushes to queues; also handles seek execution
3. **Audio output callback** (`IAudioOutput` → `FillAudioBuffer`) — pulls PCM from the audio queue and drives the master clock

**Locking discipline:** Four mutexes in `Player::Impl` — `audioMutex` (audio queue), `videoMutex` (video queue + displayed frame), `clockMutex` (no-audio wall-clock anchor), `stateMutex` (status text). The audio callback acquires only `audioMutex`. Never hold two of these simultaneously except `audioMutex` + `videoMutex` in `QueuesDrained()` (always in that order).

**A/V sync:** Audio playback position is the master clock. Each audio chunk tracks its media timestamp; as the callback consumes bytes it updates `lastKnownClockSeconds`. The main thread uses this clock to select which decoded video frame to display. For audio-less media, a wall-clock anchor + speed multiplier provides the time base.

**Speed change:** Implemented by adjusting `libswresample`'s output sample rate while keeping the SDL device rate fixed at 48 kHz. This changes speed but also changes pitch. When speed changes with audio present, a seek is triggered to recreate the `SwrContext`.

**Seek:** The main thread sets an atomic flag; the worker thread performs `av_seek_frame`, flushes decoders, clears queues, resets the clock anchor, and frees/recreates `SwrContext` on next audio frame.

**EOF:** When `av_read_frame` returns `AVERROR_EOF`, remaining frames are flushed from decoders. Once both queues drain, playback auto-pauses with status "Playback finished".

**Video pixel format:** Decoded frames are converted to `AV_PIX_FMT_BGRA` via `libswscale`, uploaded to an `SDL_PIXELFORMAT_BGRA32` streaming texture.

## Keyboard Shortcuts
- `O` — open file dialog
- `Space` — play / pause
- `Left` / `Right` — seek -5s / +5s
- `Up` / `Down` — increase / decrease speed
- `Esc` — quit

## Key Constants (in `Player.cpp`)
- Audio output: 48 kHz, stereo, S16
- Max audio queue: 3000 ms
- Max video queue: 24 frames
- Supported speeds: 0.5, 0.75, 1.0, 1.25, 1.5, 1.75, 2.0

## Dependencies (via vcpkg manifest)
- `sdl2` (2.30+)
- `sdl2-ttf`
- `ffmpeg` (7.x) — libavformat, libavcodec, libswresample, libswscale, libavutil
- `comdlg32` (Win32 only, linked behind `if(WIN32)`)
- AppKit (macOS only, for `NSOpenPanel`)

## Conventions
- All FFmpeg headers are included inside `extern "C" {}` blocks in Player.cpp
- MSVC compiler flags: `/W4 /permissive- /utf-8`; other compilers use `-Wall -Wextra -Wpedantic`
- The Windows executable is a `WIN32` (no-console) target; macOS currently builds a raw binary, not an `.app` bundle
- `CMakeUserPresets.json` is gitignored (per-developer local config)
- The `player` namespace contains all engine code; anonymous namespaces are used for file-local helpers
- Design docs live in `docs/` (design.md, support-matrix.md)
