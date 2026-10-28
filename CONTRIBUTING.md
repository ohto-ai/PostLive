# 贡献指南

感谢关注 PostLive。这是一个学习项目，欢迎提交 Issue 与 Pull Request，但请先阅读以下约定。

## 开发环境

- Windows 10 / 11 x64
- Visual Studio 2022（Desktop development with C++）
- CMake ≥ 3.21
- Qt 5.15.2，MSVC 2019 64-bit 套件
- FFmpeg（调试推流时需要）

构建步骤见 [docs/building.md](docs/building.md)。提交前至少确认 Release 构建通过：

```powershell
$env:QT_ROOT = "C:/Qt/5.15.2/msvc2019_64"
cmake --preset windows-x64
cmake --build --preset release
```

## 代码风格

- C++17，格式参考 [.clang-format](.clang-format)（4 空格缩进，但现有文件使用 Tab）。
- 保持现有命名与目录约定；新增通用控件放 `src/widgets/`，业务逻辑放 `src/core/` 或 `src/app/`。
- 不要在仓库中提交构建产物、安装包、用户 token、头像缓存或 `third_party/ffmpeg/`。
- 修改 FFmpeg 命令时同步更新 [docs/streaming.md](docs/streaming.md)。

## 提交信息

建议使用 [Conventional Commits](https://www.conventionalcommits.org/zh-hans/)：

```text
feat: 增加码率设置
fix: 修复遮罩窗口还原后不显示
docs: 更新 RTMP 部署说明
build: 升级 Qt 版本检测
```

一个 PR 尽量只做一件事；重构与功能改动分开提交。

## Pull Request

1. 从 `master` 创建分支。
2. 在 PR 描述中说明动机、改动范围与验证方式。
3. 如果改动涉及 UI，附带截图或录屏。
4. 确认 CI 通过。
5. 未完成的功能请更新 [TODO.md](TODO.md)，不要在 UI 中留下无法使用的入口。

## Issue

- Bug 请提供系统版本、Qt 版本、复现步骤、日志 / dump（`PostLive.dmp`）。
- 功能建议请说明使用场景，并尽量对应到 [TODO.md](TODO.md) 中的条目。

## 行为准则

请保持友善、就事论事。对外部服务、账号、推流地址等敏感信息做脱敏处理。
