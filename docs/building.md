# 构建指南

## 环境要求

| 组件 | 版本 | 说明 |
| --- | --- | --- |
| Windows | 10 / 11 x64 | 仅支持 Windows；推流使用 DirectShow |
| Visual Studio | 2022（或 2019） | 需要 “使用 C++ 的桌面开发” 工作负载 |
| CMake | ≥ 3.21 | `file(COPY_FILE ... ONLY_IF_DIFFERENT)` 需要 3.21 |
| Qt | 5.15.2，MSVC 2019 64-bit | 需要 Widgets、Network（预览解码用进程内 FFmpeg，不再依赖 Qt Multimedia） |
| FFmpeg | 7.x shared 构建 | `third_party/ffmpeg/bin/` 提供 `ffmpeg.exe`（推流）与 `av*-61.dll` 等运行时 DLL（推流 + 进程内预览解码共用） |

## 安装 Qt

用 Qt Online Installer 安装 `Qt 5.15.2` → `MSVC 2019 64-bit`。
安装后在 Qt Maintenance Tool 中确认至少勾选了：

- Qt 5.15.2 → MSVC 2019 64-bit

FFmpeg SDK（头文件与导入库）已随仓库放在 `third_party/ffmpeg/sdk/`（LGPL-3.0），无需单独安装；运行时 DLL 来自 `third_party/ffmpeg/bin/`，与推流共用，主版本必须保持 7.x（avcodec-61 / avutil-59 / swscale-8）。

## 配置与构建

### 使用 CMake Presets（推荐）

```powershell
$env:QT_ROOT = "C:/Qt/5.15.2/msvc2019_64"

cmake --preset windows-x64
cmake --build --preset release
```

产物：`build/windows-x64/Release/PostLive.exe`。

`debug` 预设：

```powershell
cmake --build --preset debug
```

### 不使用 Presets

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
      -DCMAKE_PREFIX_PATH="C:/Qt/5.15.2/msvc2019_64"
cmake --build build --config Release
```

如果使用 Visual Studio 2019：

```powershell
cmake -S . -B build -G "Visual Studio 16 2019" -A x64 `
      -DCMAKE_PREFIX_PATH="C:/Qt/5.15.2/msvc2019_64"
```

### CMake 选项

| 选项 | 默认值 | 说明 |
| --- | --- | --- |
| `CMAKE_PREFIX_PATH` | 空 | 指向 Qt 5.15.2 的 `msvc2019_64` 目录 |
| `POSTLIVE_FFMPEG_DIR` | `third_party/ffmpeg` | 包含 `ffmpeg.exe` 的目录；也支持其 `bin/` 子目录 |
| `POSTLIVE_OPENSSL_DIR` | 空 | 包含 OpenSSL 1.1.x DLL 的目录，用于 HTTPS 认证 |
| `CMAKE_BUILD_TYPE` | VS 多配置生成器忽略 | 单配置生成器（Ninja）使用 |

修改选项后需要重新配置，例如：

```powershell
cmake -S . -B build -DPOSTLIVE_FFMPEG_DIR="D:/tools/ffmpeg"
```

### HTTPS / OpenSSL

Qt 5.15 在 Windows 上的 QtNetwork 通过 OpenSSL 1.1.x 提供 TLS。使用 `https://` 认证服务器时，需要：

- `libssl-1_1-x64.dll`
- `libcrypto-1_1-x64.dll`

放到 `PostLive.exe` 旁边。可以让 CMake 在构建时复制：

```powershell
cmake -S . -B build -DPOSTLIVE_OPENSSL_DIR="C:/Qt/Tools/QtCreator/bin"
```

默认的 `http://localhost:8877` 不需要 OpenSSL。Qt 6 可以使用 Windows Schannel，不再依赖 OpenSSL 1.1.x。

QtNetwork 会读取 Windows 系统代理设置（`QNetworkProxyFactory::setUseSystemConfiguration`），代理环境下不需要额外配置。

## 构建产物

除了 `PostLive.exe`，`POST_BUILD` 步骤会把以下文件复制到可执行文件旁边：

```text
Release/
├── PostLive.exe
├── application.json          # 首次构建时从 config/application.default.json 创建
├── users.json                # 首次构建时从 config/users.default.json 创建
├── assets/pilipili-logo.png
├── user/avatar/              # 运行时下载头像
├── ffmpeg.exe                # 找到 FFmpeg 时
├── avcodec-*.dll
├── avdevice-*.dll
├── avfilter-*.dll
├── avformat-*.dll
├── avutil-*.dll
├── swresample-*.dll
└── swscale-*.dll
```

`application.json` 与 `users.json` **只在不存在时创建**，重新构建不会覆盖本地窗口位置、用户和 token。要恢复默认值，删除这两个文件后重新构建或手动复制 `config/*.default.json`。

## 运行

### Visual Studio

用 CMake 生成的 `build/windows-x64/PostLive.sln` 打开解决方案。CMake 已设置：

- 启动项目：`PostLive`
- 调试工作目录：`PostLive.exe` 所在目录
- 调试环境 `PATH`：Qt 的 `bin` 目录

按 F5 即可，无需手动配置 Qt 环境。

### 命令行

```powershell
$env:PATH = "C:/Qt/5.15.2/msvc2019_64/bin;$env:PATH"
.\build\windows-x64\Release\PostLive.exe
```

如果希望脱离 Qt 安装目录运行，执行：

```powershell
cmake --build build/windows-x64 --config Release --target deploy-qt
```

该目标调用 `windeployqt`，把 Qt DLL、平台插件、多媒体插件复制到 `Release/`。

### 冒烟测试

设置 `POSTLIVE_SMOKE_TEST=1` 时，程序启动后会在 500 ms 后自动退出，用于 CI 验证配置加载与用户数据保存：

```powershell
$env:POSTLIVE_SMOKE_TEST = "1"
.\build\windows-x64\Release\PostLive.exe
```

### 本地 mock 认证服务端

构建还会生成 `PostLiveMockServer.exe`（开发用，不随安装包发布）：

```powershell
# 从仓库根目录运行；示例账号 test / 123456
.\build\windows-x64\Release\PostLiveMockServer.exe tools\mock-server\users.example.json 8877
```

客户端保持默认 `http://localhost:8877` 即可。详见 [tools/mock-server/README.md](../tools/mock-server/README.md)。

## 安装包

安装包脚本位于 [packaging/PostLive.iss](../packaging/PostLive.iss)，步骤见 [packaging/README.md](../packaging/README.md)：

```powershell
cmake --build build/windows-x64 --config Release --target deploy-qt
iscc packaging/PostLive.iss
```

## 常见问题

### CMake 找不到 Qt5

- 确认 `QT_ROOT` 指向 `.../Qt/5.15.2/msvc2019_64`，而不是 `.../5.15.2`；
- 删除 `build/` 与 `CMakeCache.txt` 后重试；
- 检查 `%QT_ROOT%/lib/cmake/Qt5/Qt5Config.cmake` 是否存在。

### 找不到 Qt5Multimedia / Qt5MultimediaWidgets

在 Qt Maintenance Tool 中为 Qt 5.15.2 勾选 Multimedia 组件，然后重新配置。

### 启动时报缺少 Qt5Core.dll

Qt 的 `bin` 目录不在 `PATH` 中。开发时把 Qt `bin` 加到 `PATH`，发布前执行 `deploy-qt`。

### 登录失败

客户端默认连接 `localhost:8877`，可在设置窗口修改 `auth_server`。仓库包含开发用 mock 服务端；没有服务端时无法登录，协议见 [api.md](api.md)。

### 开播失败 / 没有画面

- 确认 `ffmpeg.exe` 与 `av*.dll` 已复制到 `Release/`；
- 确认摄像头、麦克风名称与 FFmpeg `dshow` 列出的名称一致；
- 检查 RTMP 服务端是否可达，详见 [streaming.md](streaming.md) 与 [rtmp-server.md](rtmp-server.md)。

### Windows 隐私设置阻止摄像头 / 麦克风

设置 → 隐私和安全性 → 摄像头 / 麦克风 → 允许桌面应用访问。
