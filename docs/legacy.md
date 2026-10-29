# 遗留工程

2026-10-29 的整理把仓库从 Visual Studio 解决方案改造成单一的 CMake 工程。以下子工程不再位于工作区，但仍保留在 Git 历史中。

## 移除清单

| 子工程 | 原用途 | 移除原因 |
| --- | --- | --- |
| `LiveLoginUnit` | 登录窗口原型 | 已被 `src/app/LiveLoginDialog` 取代；使用 Qt 5 早期写法 |
| `LiveBroadcastUnit` | 推流窗口原型 | 已被 `src/app/LivePlatform` 取代 |
| `ConnectionPoolUnit` | MySQL 连接池示例 | 依赖未入库的 mysqlpp / libmysql，主客户端不需要 |
| `LiveServerTestDemo` | 本地认证服务端 | 依赖缺失的 `MD5.h`，无法构建 |
| `QtHttpDownloader` | `AsyncDownloader` 示例 | 组件已移入 `src/core/AsyncDownloader` |
| `ShadowWindowDemo` | 阴影窗口示例 | 组件已移入 `src/widgets/ShadowWidget` |
| `ffmpeg-demo` | FFmpeg 解码 / 预览示例 | 硬编码本机 FFmpeg 4.2.2 开发包头文件路径 |
| `ffmpeg-process` | `QProcess` 调用 FFmpeg 示例 | 逻辑已移入 `src/app/LivePlatform` |
| `PortablePacckage` | 便携打包脚本与 exe | 二进制产物，不再维护 |

同时移除：

- `LivePlatformClient.sln` 与全部 `.vcxproj` / `.filters` / `.user`；
- 入库的 `libmysql.dll`、`mysqlpp.dll`、`InnerServer.exe`、安装包 `PilipiliLivePlatformSetup.exe`；
- 用户数据 `users.json`（含 token）、`userdata.json`、下载的头像；
- 未使用的图片与空的 `LivePlatform.qrc`。

## 如何找回

原始工程在以下标签中完整保留：

```bash
git tag -l
# v1.0.0.1  v1.0.0.2

# 查看某个旧子工程
git ls-tree -r --name-only v1.0.0.2 -- LivePlatformClient/LiveLoginUnit

# 只恢复某个目录到临时位置
git checkout v1.0.0.2 -- LivePlatformClient/LiveLoginUnit
```

本次整理开始前的工作区快照保存在本地标签 `pre-cmake-reorg`（未推送）：

```bash
git show pre-cmake-reorg --stat
```

## 迁移建议

- 需要旧登录逻辑时，优先阅读 `src/app/LiveLoginDialog.cpp` 与 `src/core/utils.cpp`，而不是复制旧工程；
- `LiveServerTestDemo` 的协议仍记录在 [api.md](api.md)，已实现为 [`tools/mock-server`](../tools/mock-server/README.md)；
- `ffmpeg-demo` 中的解码预览思路可以用于“预览开关”功能，但需要重新对接 FFmpeg 7.x。
