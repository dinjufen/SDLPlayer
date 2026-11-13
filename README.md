# SDL + FFmpeg 学习型播放器

这是一个 C++ 桌面播放器，面向 Windows 与 macOS。解码、解复用和同步使用共享的 FFmpeg 核心（`player::Player`）；产品界面是 Qt 6.9 Quick / QML。原先的 SDL 自绘界面作为遗留目标 `SDLPlayerSdl` 保留，主程序名为 `SDLPlayer`。

## 当前能力

- 打开本地媒体文件
- 支持带音频的视频播放
- 支持纯音频播放
- 播放 / 暂停
- 显示当前进度 / 总时长
- 鼠标拖动进度条 seek
- 倍速切换（0.5x–2.0x）
- 显示基础媒体信息
- 深色 Qt Quick 播放界面（控制条自动显隐、拖放打开、输出音量）
- 遗留 SDL 壳仍可构建，功能与第一阶段对等

## 说明

- 依赖 FFmpeg，因此最终支持的容器和编码范围取决于 FFmpeg 的编译配置。
- 当前首版聚焦本地文件播放与学习型代码结构。
- `m3u8 / HLS` 被保留为下一阶段扩展目标，设计文档中已经预留接入点。

## 推荐环境

- macOS：Apple Clang + Ninja，Apple Silicon，官方 Qt 6.9.x，Homebrew FFmpeg
- Windows：Visual Studio 2022 或 2026，Qt 6.9.x；遗留 SDL 壳仍可用 vcpkg
- CMake 3.25+

推荐版本方向：

- Qt 6.9+（Quick、Quick Controls 2、Quick Dialogs 2、Multimedia）
- FFmpeg 7.x
- SDL2 2.30+（仅遗留壳需要）

详细的 Qt 构建说明见 `docs/qt-shell.md`。

## 在 Mac 上构建 Qt 壳（主程序）

默认使用本机 Qt 6.9.0 路径 `/Users/ding/Qt/6.9.0/macos` 与 Homebrew FFmpeg，不需要 vcpkg：

```bash
cmake --preset macos-qt-debug
cmake --build --preset build-macos-qt-debug
ctest --preset test-macos-qt-debug
./build/macos-qt-debug/SDLPlayer [path-to-media-file]
```

可用 `-DCMAKE_PREFIX_PATH="/path/to/Qt/6.9.0/macos;/opt/homebrew"` 覆盖 Qt 路径。遗留 SDL 程序是同一目录下的 `SDLPlayerSdl`。

## 构建步骤（Windows / 遗留 SDL / vcpkg）


1. 安装并初始化 vcpkg。
2. 设置环境变量，将 `VCPKG_ROOT` 指向你本地的 vcpkg 安装目录：

   ```powershell
   $env:VCPKG_ROOT="<your-vcpkg-path>"
   ```

   这一步必须和后面的 `cmake --preset ...` 在同一个终端会话里执行；如果你开了新终端，需要重新设置。

   如果你不想依赖环境变量，可以复制 `CMakeUserPresets.json.example` 为 `CMakeUserPresets.json`，将其中的 vcpkg 路径改为你的本地路径。

   对于中文本地化的 Visual Studio / MSVC，仓库 preset 已额外设置 `VSLANG=1033`。如果 FFmpeg 因为生成的 `config.h` 中出现中文 `CC_IDENT` 而编译失败，可以尝试通过本地 vcpkg 端口补丁禁用相关滤镜绕过。

3. 在项目根目录执行：

   ```powershell
   # Visual Studio 2026
   cmake --preset msvc2026-debug
   cmake --build --preset build-msvc2026-debug

   # Visual Studio 2022
   # cmake --preset msvc2022-debug
   # cmake --build --preset build-msvc2022-debug

   # 如果使用 CMakeUserPresets.json 中的 local preset（不依赖环境变量）：
   # cmake --preset msvc2026-debug-local
   # cmake --build --preset build-msvc2026-debug-local
   ```

4. 运行自动化测试：

   ```powershell
   # Visual Studio 2026
   ctest --preset test-msvc2026-debug

   # Visual Studio 2022
   # ctest --preset test-msvc2022-debug

   # 如果使用 local preset：
   # ctest --preset test-msvc2026-debug-local
   ```

5. 运行生成的程序：

   ```powershell
   # 主程序（若本机已安装 Qt 6.9+）
   .\build\msvc2026-debug\Debug\SDLPlayer.exe

   # 遗留 SDL 壳
   .\build\msvc2026-debug\Debug\SDLPlayerSdl.exe
   ```

6. macOS（Apple Silicon，推荐）同样使用 vcpkg：

   ```bash
   export VCPKG_ROOT="<your-vcpkg-path>"
   cmake --preset macos-arm64-debug
   cmake --build --preset build-macos-arm64-debug
   ctest --preset test-macos-arm64-debug
   ./build/macos-arm64-debug/SDLPlayerSdl [path-to-media-file]
   ```

   若要构建 Qt 主程序，请改用上面的 `macos-qt-debug` preset。

   Intel Mac 可改用 `macos-x64-debug` / `build-macos-x64-debug` / `test-macos-x64-debug`。

   如果不想依赖 `VCPKG_ROOT`，同样复制 `CMakeUserPresets.json.example` 为 `CMakeUserPresets.json`，并把 vcpkg 路径改成本地路径，然后使用 `macos-arm64-debug-local` 等 `-local` preset。

   当前里程碑输出的是普通可执行文件，不是 `.app` 包。vcpkg 构建时通常会把 `vcpkg_installed/arm64-osx/lib` 写进 rpath；如果把二进制单独拷走后找不到 dylib，把对应库拷到可执行文件同目录，或设置 `DYLD_FALLBACK_LIBRARY_PATH` 指向 vcpkg 的 `lib` 目录。

   Linux 不是产品目标。若只是在 Linux 上跑测试，可以使用 `linux-ninja-debug`（vcpkg）或 `linux-system-debug`（系统包 + 仓库自带的 `cmake/FindFFMPEG.cmake`）。

如果在 `cmake` 阶段看到 “找不到 SDL2 / SDL2_ttf / FFMPEG” 一类错误，通常说明：

- `VCPKG_ROOT` 没有设置
- 还没有先通过 vcpkg 安装并解析 manifest 依赖
- 当前使用的 preset 与本机 Visual Studio 版本不匹配
- 当前 CMake 版本尚不支持所选 Visual Studio generator（例如较老的 CMake 无法识别 VS 2026）
- `VCPKG_ROOT` 为空，导致 toolchain 路径被展开成错误的 `/scripts/buildsystems/vcpkg.cmake`

## 快捷键

- `O`：打开文件
- `Space`：播放 / 暂停
- `Left / Right`：快退 / 快进 5 秒
- `Up / Down`：切换倍速
- `Esc`：退出

## 文档

- Qt 壳构建与架构：`docs/qt-shell.md`
- 设计文档：`docs/design.md`
- 格式支持矩阵：`docs/support-matrix.md`

## 已验证的实现范围

代码层面已经对以下能力进行了实现：

- 音视频解码
- SDL 音频回调输出
- 视频 RGBA 转换与渲染
- 音频时钟驱动的视频同步
- 本地 seek
- 仅音频模式占位界面
- 基础错误提示和状态展示

## 自动化测试

项目通过 CTest 注册 `SDLPlayerTests` 测试可执行文件。当前测试覆盖播放器核心在无媒体状态下的初始状态、控制调用、倍速选择，以及打开不存在文件时的错误返回路径。

测试目标不需要真实媒体样片，也不会创建 SDL 窗口；它复用 `SDLPlayerCore`，用于在后续拆分 UI 与播放器模块时守住基础状态机行为。

## 后续建议

- 为 `m3u8/HLS` 补充网络打开入口和协议层校验
- 补充带样片的媒体打开、seek、EOF 与纯音频回归测试
- 增强音视频同步策略
- 增加播放列表、音量控制、帧步进等功能
- 为 Qt 壳补充播放列表、字幕、硬解与 HLS
