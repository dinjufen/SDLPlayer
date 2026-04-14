# Copilot Instructions

## Build, run, test, and lint

- This project targets **Windows + MSVC only** and uses **C++20**, **CMake 3.25+**, and **vcpkg**.
- In PowerShell, either set `VCPKG_ROOT` in the same terminal session before configuring:

```powershell
$env:VCPKG_ROOT="<your-vcpkg-path>"
cmake --preset msvc2026-debug
cmake --build --preset build-msvc2026-debug
```

- VS 2022 presets are also supported:

```powershell
cmake --preset msvc2022-debug
cmake --build --preset build-msvc2022-debug
```

- To avoid relying on `VCPKG_ROOT`, copy `CMakeUserPresets.json.example` to `CMakeUserPresets.json`, set the local vcpkg path there, and use the `-local` presets such as:

```powershell
cmake --preset msvc2026-debug-local
cmake --build --preset build-msvc2026-debug-local
```

- Run the app from the generated output directory. It accepts an optional media file path:

```powershell
.\build\msvc2026-debug\Debug\SDLPlayer.exe [path-to-media-file]
```

- There is **no automated test suite** and **no lint configuration** in this repository.
- There is no single-test command because no test framework is checked in.

## High-level architecture

- `src\main.cpp` is the entire UI layer: SDL startup, window/renderer creation, event loop, self-drawn controls, progress drag-to-seek, Win32 open-file dialog, system font loading, and optional command-line file open.
- `src\player\Player.h` exposes a small `player::Player` API and `MediaInfo` struct. The public class is a thin pimpl wrapper around `Player::Impl`.
- `src\player\Player.cpp` contains the playback engine and all FFmpeg/SDL audio integration. `Player::Impl` owns the format/codec contexts, SDL audio device, decoded audio/video queues, current video texture, and the worker thread lifecycle.
- Playback runs on three execution contexts:
  1. Main thread: UI loop and `Player::Update()`
  2. Worker thread: `DecodeLoop()`, packet reads, decode, seek execution, queue filling
  3. SDL audio callback: `FillAudioBuffer()`, PCM consumption, master clock updates
- Audio is the master clock when audio exists. The audio callback updates `lastKnownClockSeconds`, and the main thread selects the decoded video frame closest to that clock.
- For media without audio, playback time comes from a wall-clock anchor (`noAudioAnchorPositionSeconds` + `noAudioAnchorWallClock`) scaled by the current speed.
- Seeking is asynchronous from the UI point of view: the main thread only sets an atomic request, and the worker thread performs `av_seek_frame`, flushes decoders, clears both queues, resets the clock anchor, and recreates the resampler on the next audio frame.
- Speed changes are implemented by changing the `libswresample` output sample rate while keeping the SDL device fixed at **48 kHz stereo S16**. This changes speed and pitch. When audio is present, changing speed triggers a seek so the resampler can be rebuilt cleanly.
- At EOF, the worker thread flushes both decoders, waits for audio/video queues to drain, then auto-pauses with status text `"Playback finished"`.
- `docs\design.md` and `docs\support-matrix.md` describe the intended playback model and supported format scope; use them when changing playback behavior or format expectations.

## Key conventions

- Keep FFmpeg-facing engine code in `src\player\Player.cpp`. FFmpeg headers are included there inside a single `extern "C"` block.
- The `player` namespace contains engine code; file-local helpers live in anonymous namespaces.
- The locking model is deliberate and should not be changed casually:
  - `audioMutex`: audio queue only
  - `videoMutex`: video queue and displayed frame
  - `clockMutex`: no-audio wall-clock anchor
  - `stateMutex`: status text
- The SDL audio callback should only acquire `audioMutex`.
- Never hold multiple locks at once except `audioMutex` + `videoMutex`, always in that order, in `QueuesDrained()`.
- UI rendering is asset-free and self-drawn. Fonts are loaded from Windows system fonts (`msyh.ttc`, `segoeui.ttf`, `arial.ttf`) rather than from repository assets.
- Video frames are converted to `AV_PIX_FMT_BGRA` with `libswscale` and uploaded to an `SDL_PIXELFORMAT_BGRA32` streaming texture.
- The executable is built as a `WIN32` target, so do not assume a console-oriented app flow.
- `CMakeUserPresets.json` is intended as untracked developer-local configuration and is gitignored.
