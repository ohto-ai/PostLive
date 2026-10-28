# Changelog

本项目遵循 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.1.0/)，版本号沿用原有 Git 标签。

## [Unreleased]

### Added

- 完整的 CMake 构建：`CMakeLists.txt`、`CMakePresets.json`、`cmake/DeployRuntime.cmake`。
- GitHub Actions Windows 构建与 `windeployqt` 打包产物。
- Issue / Pull Request 模板与 Dependabot 配置。
- 文档：构建、架构、依赖、API、数据库、推流、FFmpeg 命令、RTMP 服务端、遗留工程。
- `TODO.md`、`CONTRIBUTING.md`、`THIRD_PARTY_NOTICES.md`。
- 首次构建时从 `config/*.default.json` 生成运行时配置。

### Changed

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
