# 打包

使用 [Inno Setup 6](https://jrsoftware.org/isinfo.php) 生成 Windows 安装包。

## 前置条件

- Release 构建已完成；
- 已执行 `deploy-qt`，使 `build/windows-x64/Release/` 中包含 Qt DLL 与插件；
- 已把 FFmpeg 放入 `third_party/ffmpeg/`，或在配置时指定 `POSTLIVE_FFMPEG_DIR`；
- `iscc.exe` 在 `PATH` 中（默认位置：`C:\Program Files (x86)\Inno Setup 6\ISCC.exe`）。

## 构建安装包

```powershell
$env:QT_ROOT = "C:/Qt/5.15.2/msvc2019_64"

cmake --preset windows-x64
cmake --build --preset release
cmake --build build/windows-x64 --config Release --target deploy-qt

iscc packaging/PostLive.iss
```

产物：`dist/PostLive-1.0.0.2-setup.exe`。

脚本使用 `SourcePath` 定位仓库根目录，因此从任意目录调用 `iscc` 都可以。

## 安装包内容

- `PostLive.exe`；
- Qt 运行时（由 `windeployqt` 复制）；
- FFmpeg 运行时（`ffmpeg.exe` 与 `av*.dll` / `sw*.dll`，如果构建目录中存在）；
- `assets/pilipili-logo.png`；
- 默认 `application.json` / `users.json`（仅在目标文件不存在时写入）。

## 注意

- 安装包内置 FFmpeg 时，请遵守其许可证（LGPL / GPL，取决于构建）并附带源码获取方式。
- 使用 HTTPS 认证服务器时，需要 OpenSSL 1.1.x DLL。配置 `-DPOSTLIVE_OPENSSL_DIR=...` 后 CMake 会复制到构建目录，安装包会一并包含；请附带 OpenSSL 许可证。
- 程序使用动态 CRT（`/MD`），目标机器需要安装 VC++ 2015-2022 Redistributable；发布前应把 `vc_redist.x64.exe` 一起分发，或评估改用静态 CRT。
- 当前安装包没有数字签名；发布前建议配置代码签名证书。
- CI 目前只生成 zip，不生成安装包；自动化打包见 [TODO.md](../TODO.md)。
