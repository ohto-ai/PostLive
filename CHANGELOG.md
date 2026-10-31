# Changelog

本项目遵循 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.1.0/)，版本号沿用原有 Git 标签。

## [Unreleased]

### Added

- **直播页推倒重做（OBS 式场景编辑器）**：源列表（摄像头 / 整个屏幕 / 窗口 / 本地文件）、合成画布（拖拽移动、四角等比缩放、中心 / 边缘吸附参考线、右键菜单、方向键微调、双击居中）、控制 dock（开播 / 停止、直播时长、码率、静音）。
- **采集引擎**：WGC（Windows Graphics Capture）为主、DXGI 桌面复制与 PrintWindow 为后备，多屏逐显示器选择；修复加速窗口（Chrome 等）黑屏。
- **进程内编码（SceneEncoder）**：合成画布 BGRA→YUV420P→openh264，麦克风 dshow 采集→重采样→AAC，本地 HLS（2s 分片环形缓冲）与 RTMP/FLV 双输出；时间戳由应用生成，预览与推流同源。
- **音频电平表与运行时静音**；推流码率实时显示。
- **本地观看**：构建自动部署播放页，官方 nginx 静态分发即可浏览器观看（模板见 `tools/nginx/`）。
- 可写数据目录迁移至 `AppData/Roaming/ohto-ai/PostLive`（旧文件自动迁移）。
- 分类日志文件（`postlive.*` 分类）写入 AppData。
- Cloudflare Workers 认证服务端（`worker/`，D1 + KV，PBKDF2 加盐哈希），CI 跑其单元测试。
- 无头视觉验证：`POSTLIVE_SMOKE_TEST=login/platform` + `POSTLIVE_SNAPSHOT` 可脱离屏幕生成界面截图。

### Changed

- **UI**：登录页与直播页换用 PostLive.Re 设计（深色扁平主题），启动动画、内嵌错误提示（不再使用模态弹窗）。
- 采集 / 编码不再依赖 Qt Multimedia；设备枚举改用 FFmpeg avdevice。
- FFmpeg 7.x SDK 头文件与导入库入库（`third_party/ffmpeg/sdk`，LGPL-3.0）；进程内解码 / 编码与推流共用运行时 DLL。
- 版本号升至 1.1.0.0。

### Fixed

- 加速窗口（Chrome 等）采集黑屏（gdigrab → PrintWindow/WGC）。
- 缩放手柄锚点漂移导致的“拖过头突然变大”。
- 拖过画布边缘时源被压缩的问题。
- 退出时音频 fifo 排空溢出导致的堆损坏（约 20% 退出崩溃）。
- 重复添加同一摄像头导致的冻结。

### Removed

- `ffmpeg.exe` 子进程与 stdin 管道推流（ffmpeg 7 会把实时管道输入积压到 EOF，无法实时观看）。
- Qt Multimedia 依赖（`QCamera` / `QAudioDeviceInfo` 等）。

### Added

- 认证改用 QtNetwork 异步请求，支持 `http://` / `https://`；Qt 5.15 的 HTTPS 通过 OpenSSL 1.1.x 提供。
- QtNetwork 使用 Windows 系统代理设置。
- token 使用 Windows DPAPI 加密存储，旧的明文 token 会在下次保存时迁移。
- 新增 `POSTLIVE_SMOKE_TEST=1` 冒烟退出入口。
- 本地 mock 认证服务端 `tools/mock-server`（`PostLiveMockServer`），支持 login / profile / query / heartbeat。
- 设置窗口：认证服务器、官网、水印开关，保存到 `application.json`。
- 认证地址可配置；`createAuthClient()` 每次登录读取 `auth_server`，连接 / 读取超时 3 s / 5 s。
- 自动登录可靠性：token 失效清理、编辑账号 / 密码或取消勾选可中止。
- 摄像头 / 麦克风热插拔刷新（3 秒轮询，推流期间暂停）。
- 托盘显示 / 隐藏；退出前优雅停止 FFmpeg。
- 遮罩窗口最小化 / 还原事件过滤修复。
- 主窗口推流参数：编码器 / 码率 / 帧率下拉框，并持久化到 `application.json`。
- 主窗口预览开关；推流时自动停止预览，结束后恢复。
- 推流状态栏、FFmpeg stderr / `errorOccurred` 错误提示。
- `application.json` / `users.json` 缺失或损坏时回退到内置默认值（qrc）。
- 保存配置 / 用户数据失败时提示。
- 完整的 CMake 构建：`CMakeLists.txt`、`CMakePresets.json`、`cmake/DeployRuntime.cmake`。
- GitHub Actions Windows 构建与 `windeployqt` 打包产物。
- Issue / Pull Request 模板与 Dependabot 配置。
- 文档：构建、架构、依赖、API、数据库、推流、FFmpeg 命令、RTMP 服务端、遗留工程。
- `TODO.md`、`CONTRIBUTING.md`、`THIRD_PARTY_NOTICES.md`。
- 首次构建时从 `config/*.default.json` 生成运行时配置。

### Changed

- 登录客户端改为异步 QtNetwork 请求，支持运行时切换认证服务器。
- 水印可关闭；关闭时不再传入 logo 输入与 overlay 滤镜。
- 停播改为 `q` → 3 秒后 terminate → 2 秒后 kill 的优雅退出。
- 分辨率统一为 FFmpeg 的 `WxH` 格式（`×` → `x`）。
- 推流命令显式设置 `-pix_fmt yuv420p`，并按选择追加编码器 / 码率 / 帧率参数。
- 源码重组为 `src/app`、`src/core`、`src/widgets`、`src/resources`、`src/platform`。
- 第三方头文件移动到 `third_party/`。
- 可执行文件与安装包名称统一为 `PostLive`。
- 程序启动时把工作目录切换到可执行文件目录；`application.json`、`users.json`、`assets/`、`ffmpeg.exe` 等路径不再依赖启动位置。
- 未处理异常生成的 dump 文件改名为 `PostLive.dmp`。
- 文档从根目录与 `doc/` 移入 `docs/`，统一为小写文件名。
- Inno Setup 脚本移动到 `packaging/`，移除硬编码的 `E:\WorkSpace\...` 路径。

### Removed

- 旧 Visual Studio 解决方案与 `.vcxproj` / `.sln` 文件。
- 遗留子工程：`LiveLoginUnit`、`LiveBroadcastUnit`、`ConnectionPoolUnit`、`LiveServerTestDemo`、`QtHttpDownloader`、`ShadowWindowDemo`、`ffmpeg-demo`、`ffmpeg-process`、`PortablePacckage`。
- 入库的构建产物、安装包、MySQL / mysqlpp DLL、用户 token、下载头像与 `userdata.json`。
- 未使用的 `LIVE-install.png`、`avatar-thatboy.png` 与空的 `LivePlatform.qrc`。
- 127 MB 的 FFmpeg 开发包不再作为仓库内容（本地目录保留并被忽略）。

### Fixed

- CMake 构建下缺少 `UNICODE` / `_UNICODE` 导致的 `CreateFile` 编译错误。

## [1.0.0.2] - 2020-11-18

- 更新安装包。
- 修复直播开启后意外关闭的问题。
- 完成麦克风设备切换、分辨率切换与右键菜单。

## [1.0.0.1] - 2020-11-17

- 首个可运行版本。
