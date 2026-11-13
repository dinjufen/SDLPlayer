# Qt 6 Quick 壳层

产品界面是 Qt 6.9+ Quick / QML。FFmpeg 播放器核心（`player::Player`、`IAudioOutput`、`IVideoOutput`）保持不变。原 SDL 自绘界面仍作为遗留目标 `SDLPlayerSdl` 保留。

## 架构

```
QML (Main.qml + 控制条 / 进度 / 倍速 / 占位)
    └── PlayerController   属性与槽，16ms 定时器调用 Player::Update / RenderVideo
            ├── player::Player     解复用、解码、同步、seek、倍速
            ├── QtAudioOutput      IAudioOutput，QAudioSink 拉取 48 kHz 立体声 S16
            └── QtVideoOutput      IVideoOutput，拷贝 BGRA 帧
                    └── VideoItem  QQuickItem + QSGImageNode，Keep 比例 letterbox
```

音量是 `QAudioSink::setVolume` 的输出增益，没有改播放器引擎。拖放文件、系统文件对话框、快捷键都在 QML 里处理。

## 在 macOS 上构建（推荐）

本机已安装：

- 官方 Qt 6.9.0：`/Users/ding/Qt/6.9.0/macos`（可用 `-DCMAKE_PREFIX_PATH=...` 覆盖）
- Homebrew：`ffmpeg`、`sdl2`、`sdl2_ttf`、`ninja`（Qt 目标不链接 SDL；SDL 仅用于遗留 `SDLPlayerSdl`）

```bash
cd /Users/ding/code/github/SDLPlayer
cmake --preset macos-qt-debug
cmake --build --preset build-macos-qt-debug
ctest --preset test-macos-qt-debug
./build/macos-qt-debug/SDLPlayer [path-to-media-file]
```

Release 把 `debug` 换成 `release`。如果 Qt 不在默认路径：

```bash
cmake --preset macos-qt-debug -DCMAKE_PREFIX_PATH="/path/to/Qt/6.9.0/macos;/opt/homebrew"
```

也可以复制 `CMakeUserPresets.json.example` 为 `CMakeUserPresets.json`（不要提交），使用 `macos-qt-debug-local` 并改其中的 Qt 路径。

FFmpeg 通过仓库里的 `cmake/FindFFMPEG.cmake` 查找 Homebrew 的 `/opt/homebrew` 前缀，不需要 vcpkg。查找时会跳过 Qt 安装目录里自带的 FFmpeg（那是给 Qt Multimedia 用的，主版本可能与 Homebrew 头文件不一致，混用会导致音频重采样失败）。若把二进制单独拷走后找不到 dylib，把 Homebrew 的 FFmpeg 库放到可执行文件旁边，或设置 `DYLD_FALLBACK_LIBRARY_PATH=/opt/homebrew/lib`。

遗留 SDL 壳（同一构建目录，如果找到了 SDL2）：

```bash
./build/macos-qt-debug/SDLPlayerSdl [path-to-media-file]
```

仅构建 SDL / 使用 vcpkg 的旧 preset（`macos-arm64-debug`）现在也会生成 `SDLPlayerSdl`，而不再叫 `SDLPlayer`。

## 快捷键

- `O` 或 `Ctrl/Cmd+O`：打开文件
- `Space`：播放 / 暂停
- `Left` / `Right`：快退 / 快进 5 秒
- `Up` / `Down` 或 `]` / `[`：切换倍速
- `Esc`：关闭倍速菜单
- 可将媒体文件拖到窗口打开

## 后续未包含的工作

本壳层不对齐：HLS / 播放列表 / 字幕 / 硬解。音量目前只是输出增益。
