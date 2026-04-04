# SDL + FFmpeg 学习型播放器

这是一个从零搭建的 C++ 音视频播放器示例工程，面向 Windows + MSVC 学习场景，使用 `SDL2 + FFmpeg` 完成视频渲染、音频输出、媒体解析、进度控制和基础信息展示。

## 当前能力

- 打开本地媒体文件
- 支持带音频的视频播放
- 支持纯音频播放
- 播放 / 暂停
- 显示当前进度 / 总时长
- 鼠标拖动进度条 seek
- 倍速切换（0.5x / 1.0x / 1.25x / 1.5x / 2.0x）
- 显示基础媒体信息
- SDL 自绘简洁 UI，操作习惯参考常见桌面播放器

## 说明

- 依赖 FFmpeg，因此最终支持的容器和编码范围取决于 FFmpeg 的编译配置。
- 当前首版聚焦本地文件播放与学习型代码结构。
- `m3u8 / HLS` 被保留为下一阶段扩展目标，设计文档中已经预留接入点。

## 推荐环境

- Visual Studio 2022 或 2026
- CMake 3.25+
- vcpkg

推荐版本方向：

- SDL2 2.30+
- FFmpeg 7.x

这里选择 SDL2 而不是 SDL3，是为了让首版播放器先基于更成熟、资料更多的 API 跑通核心链路，同时保持依赖版本尽量新且稳定。

## 构建步骤

1. 安装并初始化 vcpkg。
2. 设置环境变量：

   ```powershell
   $env:VCPKG_ROOT="C:\Users\dinju\tools\vcpkg"
   ```

   这一步必须和后面的 `cmake --preset ...` 在同一个终端会话里执行；如果你开了新终端，需要重新设置。

   如果你不想依赖环境变量，也可以使用项目里的 `CMakeUserPresets.json`（已为当前机器写入本机路径），或者参考 `CMakeUserPresets.json.example` 自己调整。

   对于中文本地化的 Visual Studio / MSVC，仓库 preset 已额外设置 `VSLANG=1033`。如果 FFmpeg 仍然因为生成的 `config.h` 中出现中文 `CC_IDENT` 而失败（本机曾在 `vsrc_gfxcapture_winrt.cpp` 触发），当前机器已通过本地 vcpkg 端口补丁禁用 `gfxcapture` 滤镜绕过该问题；这个滤镜用于屏幕捕获，不影响本播放器学习项目的本地媒体播放。

3. 在项目根目录执行：

   ```powershell
   # 当前这台机器推荐：Visual Studio 2026 + local preset
   cmake --preset msvc2026-debug-local
   cmake --build --preset build-msvc2026-debug-local

   # 如果当前终端还没有刷新到 VCPKG_ROOT，可直接使用不依赖环境变量的 local preset：
   # cmake --preset msvc2022-debug-local
   # cmake --build --preset build-msvc2022-debug-local

   # 如果你的机器安装的是 VS 2022，则改用：
   # cmake --preset msvc2022-debug-local
   # cmake --build --preset build-msvc2022-debug-local

   # 如果当前终端已正确拿到 VCPKG_ROOT，也可以改用：
   # cmake --preset msvc2026-debug
   # cmake --build --preset build-msvc2026-debug
   ```

4. 运行生成的程序：

   ```powershell
   # Visual Studio 2026
   .\build\msvc2026-debug\Debug\SDLPlayer.exe

   # Visual Studio 2022
   # .\build\msvc2022-debug\Debug\SDLPlayer.exe
   ```

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

## 后续建议

- 为 `m3u8/HLS` 补充网络打开入口和协议层校验
- 增强音视频同步策略
- 增加播放列表、音量控制、帧步进等功能
- 进一步拆分 UI 与核心播放器模块
