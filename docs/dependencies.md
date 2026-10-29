# 依赖说明

## 依赖清单

| 依赖 | 版本 / 要求 | 类型 | 是否入库 | 用途 | 许可证 |
| --- | --- | --- | --- | --- | --- |
| Qt | 5.15.2，MSVC 2019 64-bit | 动态库 | 否 | Core / Gui / Widgets / Network / Multimedia / MultimediaWidgets | LGPL-3.0 / GPL / 商业 |
| FFmpeg | 7.x（测试于 `N-114855`，2024-04-18） | 运行时进程 | 否 | DirectShow 采集、编码、RTMP 推流 | LGPL-2.1+ / GPL（取决于构建） |
| cpp-httplib | 0.11.2 | 源码头文件 | 是 | mock 服务端 HTTP | MIT |
| nlohmann/json | 3.11.2 | 源码头文件 | 是 | 配置 / 用户数据 JSON | MIT |
| OpenSSL | 1.1.x（仅 HTTPS 认证） | 动态库 | 否 | QtNetwork TLS | OpenSSL / SSLeay |
| Windows SDK | 10 | 系统库 | 否 | Win32 API、DbgHelp | Microsoft |
| Visual Studio | 2019 / 2022 | 构建工具 | 否 | C++ 编译器、MSBuild | Microsoft |
| CMake | ≥ 3.21 | 构建工具 | 否 | 生成工程 | BSD-3-Clause |
| Inno Setup | 6 | 构建工具 | 否 | Windows 安装包 | Inno Setup License |

## Qt 5.15.2

- 官方安装器：<https://www.qt.io/download-qt-installer>
- 需要的模块：`Core`、`Gui`、`Widgets`、`Network`、`Multimedia`、`MultimediaWidgets`
- 与 Qt 6 的差异：`QCameraInfo`、`QCameraViewfinder`、`QAudioDeviceInfo` 在 Qt 6 中移除；当前代码不能直接编译 Qt 6。
- 运行时通过 `windeployqt` 部署，见 [building.md](building.md)。

## FFmpeg

客户端不链接 libav*，而是用 `QProcess` 调用 `ffmpeg.exe`，因此：

- 不需要 FFmpeg 开发头文件或 `.lib`；
- 需要 `ffmpeg.exe` 以及它依赖的 `avcodec-*.dll`、`avdevice-*.dll`、`avfilter-*.dll`、`avformat-*.dll`、`avutil-*.dll`、`swresample-*.dll`、`swscale-*.dll`；
- 需要支持 `dshow` 输入（Windows 官方 / gyan.dev / BtbN 构建均可）。

获取方式：

1. 从 <https://www.gyan.dev/ffmpeg/builds/> 或 <https://github.com/BtbN/FFmpeg-Builds/releases> 下载 Windows x64 构建；
2. 解压到 `third_party/ffmpeg/`（该目录不提交）；
3. 或在配置时指定 `-DPOSTLIVE_FFMPEG_DIR=/path/to/ffmpeg`。

CMake 会自动查找 `ffmpeg.exe`，并把可执行文件与 `av*.dll` / `sw*.dll` 复制到输出目录。仓库中不会提交 FFmpeg 二进制。

许可证注意：官方与第三方构建可能启用 GPL 组件。若随安装包分发，需要附上对应许可证与源码获取方式。

## cpp-httplib 0.11.2

- 单头文件，位于 `third_party/cpp-httplib/httplib.h`；
- 在 Windows 上通过 `#pragma comment(lib, "ws2_32.lib")` 链接 Winsock；
- 现在只由 `tools/mock-server` 使用；客户端认证已改用 QtNetwork。

## OpenSSL 1.1.x（HTTPS）

- Qt 5.15 在 Windows 上的 QtNetwork 通过 OpenSSL 1.1.x 提供 TLS；
- 需要 `libssl-1_1-x64.dll` 和 `libcrypto-1_1-x64.dll`，放在 `PostLive.exe` 旁边；
- CMake 可通过 `-DPOSTLIVE_OPENSSL_DIR=<dir>` 在构建时复制；
- 仅在使用 `https://` 认证服务器时需要；默认的 `http://localhost:8877` 不需要。
- Qt 6 可以使用 Windows Schannel，不再依赖 OpenSSL 1.1.x。

## nlohmann/json 3.11.2

- 位于 `third_party/nlohmann/`；
- 通过 `from_json` / `to_json` 适配 `QString`、`QStringList`、`QRect`、`WidgetConfigInfo`；
- 仅头文件，无额外链接依赖。

## 系统库

| 库 | 链接方 | 说明 |
| --- | --- | --- |
| `ws2_32` | mock server / QtNetwork | Winsock |
| `dbghelp` | `src/main.cpp` | `MiniDumpWriteDump` |
| `crypt32` | `src/core/utils.cpp` | Windows DPAPI token 加密 |
| `user32` / `gdi32` 等 | Qt / Windows | 由 Qt 与 MSVC 默认链接 |

## 已移除的旧依赖

| 依赖 | 原位置 | 移除原因 |
| --- | --- | --- |
| MySQL / mysqlpp | `ConnectionPoolUnit`、`LiveLoginUnit` | 旧登录 demo 使用，主客户端不依赖；DLL 与头文件未完整入库 |
| FFmpeg 4.2.2 开发包 | `ffmpeg-demo` | 硬编码 `E:\Library\...` 路径，且主客户端只调用 `ffmpeg.exe` |
| `MD5.h` | `LiveServerTestDemo` | 文件缺失，旧测试服务端无法构建 |
| 预编译的 `InnerServer.exe` / 便携包 | `bin/`、`PortablePacckage/` | 二进制产物，不属于源码 |

历史版本可通过 Git 标签 `v1.0.0.1` / `v1.0.0.2` 找回，详见 [legacy.md](legacy.md)。
