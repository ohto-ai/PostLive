# TODO / 路线图

> 最后更新：2026-10-29
> 当前分支：`feat/auth-network`（包含 A2 / B1 / B2 / C1 / C2）；`master` 仍为 `416d3cb`。
> 下次开始前可先合并：
>
> ```bash
> git switch master
> git merge --ff-only feat/auth-network
> ```

## 当前进度

| 区域 | 状态 |
| --- | --- |
| 工程化整理 | ✅ CMake / 目录 / 文档 / CI / GitHub 模板 |
| A 功能补齐 | ✅ 除品牌统一、遮罩窗口实机复测 |
| B 认证客户端侧 | ✅ API 地址、mock server、QtNetwork HTTPS、DPAPI token |
| B 服务端 / 部署 | ⏳ 需要决策 |
| C 工程质量 | 🔄 C1（单元测试 + 日志）、C2（CI 安装包 + OpenSSL）完成；其余见 P2 |
| 调研 | ✅ Cloudflare、DirectUI |

## 下次优先做

1. **实机验证**：遮罩窗口、设置窗口、登录/设置内嵌错误提示、`log/PostLive.log`、CI 安装包安装流程
2. 定 B 的后端方向（Cloudflare Workers / 自建服务端 + 密码哈希）

## P1 需要决策 / 外部依赖

- [ ] **凭证安全（服务端）**：密码仍是 MD5。需要改 API 协议 + 真实服务端；`tools/mock-server` 同步加盐哈希。涉及 `src/app/LiveLoginDialog.cpp`、`tools/mock-server/main.cpp`、`docs/api.md`。
- [ ] **Cloudflare 部署**：Workers + D1 + KV + Pages + Wrangler / CI。需要 Cloudflare 账号、域名、API Token；方案与免费额度见 [docs/cloudflare.md](docs/cloudflare.md)。
- [ ] **query / heartbeat 客户端**：mock server 已实现；客户端目前只发 login / profile。需要决定是否加好友在线状态 UI，或只做后台 heartbeat。
- [ ] **遮罩窗口实机复测**：代码已加父窗口事件过滤；需要在真实桌面验证“打开设置 → 最小化 → 还原”时遮罩层是否恢复。
- [ ] **设置窗口实机检查**：新增官网 / 水印 / 保存按钮的布局、DPI、最小尺寸。
- [ ] **HTTPS 端到端验证**：需要真实 HTTPS 认证服务端 + OpenSSL 1.1.x DLL；当前只验证了 QtNetwork TLS 探针。
- [ ] **品牌统一**（低优先级）：`LivePlatform` 类名、`:/LiveLoginDialog/...` 资源前缀、部分图片命名。
- [ ] **DirectUI 迁移决策**：调研完成，见 [docs/directui-evaluation.md](docs/directui-evaluation.md)；当前不建议，除非有硬性体积 / 许可证需求。

## P2 工程质量（不依赖后端）

- [x] **单元测试**：`tests/test_core.cpp`（Qt Test）：JSON 序列化、QRect / WidgetConfigInfo 往返、配置默认值 / 容错 / 合并、users.json 缺失回退、DPAPI token 往返、MD5 已知向量。`ctest --preset windows-x64`，CI 已接入；`POSTLIVE_BUILD_TESTS=OFF` 可关闭。
- [ ] **冒烟测试扩展**：已有 `POSTLIVE_SMOKE_TEST=1`（500 ms 自动退出，offscreen 下可验证无弹窗崩溃）；补充无摄像头 / 无麦克风 / 无 FFmpeg / 认证不可达的 UI 行为。
- [x] **CI 安装包**：CI 用预装 Inno Setup 6 编译 `packaging/PostLive.iss`，与 zip 一起上传为 Actions 产物；本地打包见 [packaging/README.md](packaging/README.md)。
- [x] **CI OpenSSL**：CI 在打包前自动获取 OpenSSL 1.1.1w 并把 `libssl-1_1-x64.dll` / `libcrypto-1_1-x64.dll` 复制进产物（zip 与安装包均包含）。
- [ ] **Qt 6 迁移**：`QCameraInfo`、`QCameraViewfinder`、`QAudioDeviceInfo` 需改用 `QMediaDevices` / `QVideoWidget` / `QAudioSource`。
- [ ] **代码格式检查**：运行 `.clang-format` 统一格式，并在 CI 中校验。
- [ ] **静态分析**：clang-tidy / MSVC `/analyze`。
- [ ] **配置目录**：`application.json` / `users.json` / 头像迁移到 `QStandardPaths::AppDataLocation`。
- [ ] **国际化**：Qt Linguist。
- [ ] **移除 MSVC 专有写法**：`#pragma execution_character_set`、`QString::asprintf` + `std::string`。
- [ ] **依赖管理**：vcpkg / CPM / FetchContent，或完善 `third_party` 升级流程。
- [ ] **CMake install / CPack**：安装规则已有，补 CPack 与可复现安装包。
- [ ] **FFmpeg 分发合规**：安装包内置时附带许可证与源码获取说明。
- [ ] **VC++ 运行时**：安装包附带 VC++ 2015-2022 Redistributable，或评估静态 CRT。

## 已知限制 / 注意

- `-platform offscreen` 下显示 `QMessageBox` 会段错误；错误路径需要真实 Windows 平台验证，无头测试只适合成功路径。
- 遮罩窗口修复、设置窗口布局尚未做视觉验证；本次新增的登录框 / 设置框内嵌错误提示（红边框 + 文字）与 `log/PostLive.log` 也需要实机过一眼。
- HTTPS 只用 QtNetwork TLS 探针验证过；还没有用真实 HTTPS 认证服务端跑完整登录。CI / 本地产物已带 OpenSSL 1.1.1 DLL。
- Qt 5.15 的 HTTPS 需要 OpenSSL 1.1.x DLL；CI 产物已包含，本地开发需自行放置或用 `POSTLIVE_OPENSSL_DIR`。
- 日志轮转（2 MB，保留 1 份 `.1`）只在启动与写入路径简单验证，未做长跑验证。

## 已完成（2026-10-29）

### 工程化

- [x] CMake + `CMakePresets.json` + `deploy-qt` + `install()`。
- [x] 源码重组为 `src/app`、`src/core`、`src/widgets`、`src/resources`、`src/platform`。
- [x] 删除旧 VS 工程、构建产物、安装包、用户 token、MySQL DLL。
- [x] GitHub Actions、Issue / PR 模板、Dependabot、项目文档。
- [x] 程序工作目录固定到可执行文件目录；默认配置 / 用户数据首次生成与容错。

### A 功能补齐

- [x] 主窗口编码器 / 码率 / 帧率选择，持久化到 `application.json`。
- [x] 预览开关；推流时自动停止并禁用。
- [x] 推流状态栏、FFmpeg stderr / `errorOccurred` 错误提示。
- [x] 优雅停播（`q` → 3 秒 terminate → 2 秒 kill）。
- [x] 设置窗口：认证服务器、官网、水印开关。
- [x] 自动登录可靠性：超时、token 失效清理、编辑 / 取消勾选中止。
- [x] 摄像头 / 麦克风热插拔刷新。
- [x] 托盘显示 / 隐藏、最小化暂停预览、退出前停止 FFmpeg。
- [x] 遮罩窗口父窗口事件过滤（待实机复测）。

### C 工程质量（C1：单元测试 + 日志）

- [x] 日志模块 `src/core/Logging`：`QLoggingCategory` 分类（app / core / network / stream / device），消息写入 `log/PostLive.log`（2 MB 轮转，保留 1 份备份）并镜像 stderr；`main` 启动时初始化。
- [x] 移除全部 `QMessageBox` 弹窗：登录失败 → 登录框内嵌提示 label（8 秒自动清除）；设置校验 → 字段红边框 + 内嵌文字；推流错误 → 主窗口状态栏；全局错误 / 崩溃 → 日志。未知异常、保存失败等路径全部走日志。
- [x] 单元测试 `tests/`（Qt Test，offscreen 可跑）：JSON（QString / QStringList / QRect / WidgetConfigInfo）往返、配置缺失 / 损坏 / 合并 / 补全、`saveConfig` 往返、`users.json` 缺失回退、DPAPI token 往返与幂等、MD5 已知向量；`ctest --preset windows-x64` 接入 CI。
- [x] offscreen 冒烟复验：`POSTLIVE_SMOKE_TEST=1` + `QT_QPA_PLATFORM=offscreen` 启动退出正常（同时证明无弹窗）。

### C 工程质量（C2：CI 安装包 + OpenSSL）

- [x] CI 用 GitHub 预装的 Inno Setup 6 编译 `packaging/PostLive.iss`（版本号从 CMakeLists 解析传入），安装包与 zip 一起上传为 Actions 产物；升级不覆盖用户 `application.json` / `users.json`。
- [x] CI 打包前自动获取 OpenSSL 1.1.1w（slproweb Light 安装包静默安装），把两枚 DLL 复制进构建目录；zip 与安装包因此都支持 HTTPS 认证。
- [x] `packaging/README.md` 更新安装包内容清单与本地 OpenSSL 补齐方式。

### B 认证与后端

- [x] API 地址可配置（`auth_server`），设置窗口可编辑。
- [x] `tools/mock-server`：login / profile / query / heartbeat。
- [x] 认证改用 QtNetwork 异步请求；支持 http / https、5 s 超时、系统代理。
- [x] token 使用 Windows DPAPI 加密存储；旧明文 token 自动迁移。
- [x] `POSTLIVE_OPENSSL_DIR` 部署 OpenSSL 1.1.x；`POSTLIVE_SMOKE_TEST` 冒烟入口。
- [x] Cloudflare 免费认证 / 推流方案调研（见 [docs/cloudflare.md](docs/cloudflare.md)）。
- [x] DirectUI 迁移可行性调研（见 [docs/directui-evaluation.md](docs/directui-evaluation.md)）。
