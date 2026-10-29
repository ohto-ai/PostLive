# TODO / 路线图

> 最后更新：2026-10-29
> 状态来自 2020 年的代码与本次工程化整理。欢迎认领；提交前请先看 [CONTRIBUTING.md](CONTRIBUTING.md)。

## 实施计划与预估（AI 执行）

| 批次 | 内容 | 有效工作时间 | 会话数 |
| --- | --- | --- | --- |
| A. 功能补齐 | 设置窗口、码率/帧率、预览开关、遮罩修复、自动登录、推流错误、热插拔、优雅停播、托盘、品牌统一 | 2–4 小时 | 1–2 |
| B. 认证与安全 | API 地址可配置、mock 服务端、凭证安全、HTTPS、`query` / `heartbeat` | 3–5 小时 | 1–2 |
| C. 工程质量 | Qt 6 迁移、测试、CI 安装包、格式化/静态分析、配置目录、日志、i18n、依赖管理、CPack、FFmpeg/VC 运行时、配置容错 | 5–8 小时 | 2–3 |
| **合计** | | **10–15 小时** | **4–6** |

最小可用批次（设置、码率、预览开关、推流错误、停播、热插拔、API 地址、配置容错）约 3–5 小时。
以上包含编译与冒烟测试，不含真实摄像头 / 麦克风 / RTMP 服务端 / HTTPS 证书 / 代码签名等外部验证。

## 功能

- [ ] **设置窗口**：实现服务器地址、官网、遮罩开关等选项，并持久化到 `application.json`。
- [x] **码率 / 帧率 / 编码器设置**：主窗口下拉框，默认 `Auto`，写入 FFmpeg 参数并持久化。
- [x] **预览开关**：主窗口 `previewCheckBox`；推流时自动停止并禁用。
- [ ] **修复遮罩窗口**：最小化还原后遮罩层可能不再显示（`ShadowWidget`）。
- [ ] **自动登录可靠性**：网络失败、token 过期时给出明确提示，并支持取消自动登录。
- [x] **推流状态与错误提示**：状态栏 + `QProcess::errorOccurred` / stderr 对话框。
- [ ] **设备热插拔**：摄像头 / 麦克风插拔后刷新下拉框。
- [x] **停止推流**：写 `q` → 3 秒后 terminate → 2 秒后 kill。
- [ ] **窗口 / 托盘行为**：统一关闭、最小化、退出菜单与托盘状态。
- [ ] **品牌统一**：类名（`LivePlatform`）、资源前缀（`:/LiveLoginDialog/...`）与部分图片仍是旧命名，统一为 PostLive。

## 认证与后端

- [ ] **可配置 API 地址**：`utils.cpp` 中硬编码 `httplib::Client{ "localhost", 8877 }`，改为读取配置。
- [ ] **提供本地 mock 服务端**：旧 `LiveServerTestDemo` 依赖缺失的 `MD5.h`，已移除；可用 `cpp-httplib` + Qt 重写一个开发用服务端。
- [ ] **凭证安全**：密码只做 MD5、token 明文存 `users.json`；至少改为加盐哈希 + 加密存储，并在文档中说明。
- [ ] **HTTPS 支持**：服务端部署 TLS 时，客户端需要 OpenSSL 支持的 `cpp-httplib` 构建。
- [ ] **补齐 API 类型**：客户端目前只实现 `login` 与 `profile`，`query` / `heartbeat` 尚未实现。
- [x] **Cloudflare 免费认证评估**：Workers + D1 + KV 方案与免费额度见 [`docs/cloudflare.md`](docs/cloudflare.md)；是否采用待定。
- [x] **Cloudflare 推流方案评估**：R2 + HLS、Stream、Realtime、Tunnel + 自建 RTMP 的对比见 [`docs/cloudflare.md`](docs/cloudflare.md)；推荐先做 R2 + HLS 实验，或直接使用 Stream 付费。
- [ ] **Cloudflare 部署**：Wrangler 配置、D1 schema、Worker secrets、Pages 注册/播放页、CI 部署。

## 工程与质量

- [ ] **Qt 6 迁移**：`QCameraInfo`、`QCameraViewfinder`、`QAudioDeviceInfo` 在 Qt 6 中移除，需要改用 `QMediaDevices` / `QVideoWidget` / `QAudioSource`。
- [ ] **单元测试**：为 `utils` 的 JSON 序列化、`WidgetConfigInfo`、配置默认值补测试（建议 Catch2 或 Qt Test）。
- [ ] **测试冒烟流程**：无摄像头 / 无麦克风 / 无 FFmpeg / 认证服务不可达时的 UI 行为。
- [ ] **CI 打包**：当前 CI 只构建并上传 `windeployqt` 后的 zip；补 Inno Setup 安装包与发布工作流。
- [ ] **代码格式检查**：接入 `.clang-format` 检查（先统一格式化，再在 CI 中校验）。
- [ ] **静态分析**：接入 clang-tidy / MSVC `/analyze`。
- [ ] **配置目录**：把 `application.json` / `users.json` / 头像移到 `QStandardPaths::AppDataLocation`，而不是可执行文件旁边。
- [x] **配置容错**：缺失 / 损坏时回退到内置默认值；保存失败会提示。
- [ ] **日志**：增加统一日志（`QLoggingCategory` 或 spdlog），替代散落的 `QMessageBox`。
- [ ] **国际化**：界面中文字符串硬编码在 `.ui` / `.cpp`，接入 Qt Linguist。
- [ ] **移除 MSVC 专有写法**：`#pragma execution_character_set("utf-8")`、`QString::asprintf` + `std::string` 可改为 `QStringLiteral` / `arg()`。
- [ ] **依赖管理**：评估用 vcpkg / CPM / FetchContent 替代内置头文件，或至少记录升级流程（见 `third_party/README.md`）。
- [x] **DirectUI 迁移调研**：结论见 [docs/directui-evaluation.md](docs/directui-evaluation.md)；当前不建议迁移，除非有硬性体积/许可证需求。
- [ ] **CMake 安装 / CPack**：完善 `install()` 与 CPack，生成可复现的安装包。
- [ ] **FFmpeg 分发合规**：安装包内置 FFmpeg 时附带许可证与源码获取说明（LGPL / GPL 取决于构建）。
- [ ] **VC++ 运行时**：安装包附带 VC++ 2015-2022 Redistributable，或在 CI 中检测 / 安装；评估静态 CRT 与 Qt 的兼容性。

## 最近完成（2026-10-29）

- [x] 主窗口推流参数：编码器 / 码率 / 帧率，持久化到 `application.json`。
- [x] 预览开关，推流时自动停止预览。
- [x] 推流状态栏与 FFmpeg 错误提示。
- [x] 优雅停播（`q` → terminate → kill）。
- [x] `application.json` / `users.json` 缺失或损坏时回退到内置默认值；保存失败提示。

## 本次整理已完成

- [x] 使用 CMake 重建构建系统，提供 `CMakePresets.json`。
- [x] 源码按 `src/app`、`src/core`、`src/widgets`、`src/resources`、`src/platform` 重组。
- [x] 第三方头文件统一到 `third_party/`，FFmpeg 本地目录加入 `.gitignore`。
- [x] 移除旧 Visual Studio 解决方案与 9 个遗留 / demo 子工程（见 `docs/legacy.md`）。
- [x] 清理构建产物、安装包、MySQL DLL、用户 token、下载头像等不应入库的文件。
- [x] 增加 GitHub Actions 构建、Issue / PR 模板、Dependabot。
- [x] 增加 README、构建 / 架构 / 依赖 / API / 推流 / RTMP / 遗留工程文档。
- [x] 程序启动时把工作目录固定到可执行文件目录，运行时文件不再依赖启动位置。
- [x] 默认配置与用户数据改为首次构建时从 `config/*.default.json` 创建。
