# TODO / 路线图

> 最后更新：2026-10-29
> 当前分支：`feat/auth-network`（包含 A2 / B1 / B2 / B3 / C1 / C2 / D1 合并）；`master` 仍为 `416d3cb`。
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
| A 功能补齐 | ✅ 除品牌统一、遮罩窗口实机复测；已并入 PostLive.Re 的采集源与 UI 主题 |
| B 认证客户端侧 | ✅ API 地址、mock server、QtNetwork HTTPS、DPAPI token |
| B 服务端 / 部署 | 🔄 Worker + D1 + KV 代码就绪（含单元测试），待 Cloudflare 账号部署 |
| C 工程质量 | 🔄 C1（单元测试 + 日志）、C2（CI 安装包 + OpenSSL）完成；其余见 P2 |
| 调研 | ✅ Cloudflare、DirectUI |

## 下次优先做

1. **直播页实测**：进程内编码已打通（WGC 采集 → 画布合成 → libav 编码 → HLS 实时落盘 → nginx/浏览器播放），待实机确认画质/流畅度与浏览器播放体验
2. **推流音频**：WASAPI 麦克风采音 → 进程内 AAC 编码 → HLS 双轨（本期未做，控制 dock 的音轨开关暂只作用于 RTMP 路径）
3. **RTMP 推流同样迁到进程内编码**（flv over avio）——stdin 管道方案因 ffmpeg 7 调度问题（实时喂入被积压到 EOF）已废弃
2. **本地观看链路实测**：设置勾选"本地 HLS 输出"→ 开播 → 浏览器打开 http://localhost:18080/player.html（nginx 已配置在 C:\Tools
ginx-1.30.6，模板见 [tools/nginx/](tools/nginx/)）；以及用 Cloudflare Worker 跑一次真实 HTTPS 登录（[worker/README.md](worker/README.md)）
3. **实机验证**：遮罩窗口、设置窗口（新增直播设置区）、新登录界面、CI 安装包安装流程

## P1 需要决策 / 外部依赖

- [x] **凭证安全（服务端）**：客户端协议不变（HTTPS + 大写 MD5 hex），服务端存储改为 PBKDF2-SHA256（每用户盐 + pepper）；Worker 与 mock server 同步实现，含 `register` 管理接口。剩余风险记录在 [worker/README.md](worker/README.md)。
- [ ] **Cloudflare 部署**：代码与配置已就绪（[worker/](worker/README.md)），需要 Cloudflare 账号：`wrangler login` → 创建 D1/KV → 填 `wrangler.toml` id → `secret put` → `wrangler deploy`。方案与免费额度见 [docs/cloudflare.md](docs/cloudflare.md)。
- [ ] **query / heartbeat 客户端**：服务端（Worker + mock）已实现全部端点；客户端目前只发 login / profile。需要决定是否加好友在线状态 UI，或只做后台 heartbeat（注意 KV 写配额）。
- [ ] **遮罩窗口实机复测**：代码已加父窗口事件过滤；需要在真实桌面验证“打开设置 → 最小化 → 还原”时遮罩层是否恢复。
- [ ] **query / heartbeat 客户端 UI**（服务端已就绪）与 **品牌统一**（`LivePlatform` 类名、pilipili 标题图等旧命名）仍待定。
- [ ] **设置窗口实机检查**：新增官网 / 水印 / 保存按钮的布局、DPI、最小尺寸。
- [ ] **HTTPS 端到端验证**：需要真实 HTTPS 认证服务端 + OpenSSL 1.1.x DLL；当前只验证了 QtNetwork TLS 探针。
- [ ] **品牌统一**（低优先级）：`LivePlatform` 类名、`:/LiveLoginDialog/...` 资源前缀、部分图片命名。
- [ ] **DirectUI 迁移决策**：调研完成，见 [docs/directui-evaluation.md](docs/directui-evaluation.md)；当前不建议，除非有硬性体积 / 许可证需求。

## P2 工程质量（不依赖后端）

- [x] **单元测试**：`tests/test_core.cpp`（Qt Test）：JSON 序列化、QRect / WidgetConfigInfo 往返、配置默认值 / 容错 / 合并、users.json 缺失回退、DPAPI token 往返、MD5 已知向量。`ctest --preset windows-x64`，CI 已接入；`POSTLIVE_BUILD_TESTS=OFF` 可关闭。
- [ ] **冒烟测试扩展**：已有 `POSTLIVE_SMOKE_TEST=1`（500 ms 自动退出，offscreen 下可验证无弹窗崩溃）；补充无摄像头 / 无麦克风 / 无 FFmpeg / 认证不可达的 UI 行为。
- [x] **CI 安装包**：CI 用预装 Inno Setup 6 编译 `packaging/PostLive.iss`，与 zip 一起上传为 Actions 产物；本地打包见 [packaging/README.md](packaging/README.md)。
- [x] **CI OpenSSL**：CI 在打包前自动获取 OpenSSL 1.1.1w 并把 `libssl-1_1-x64.dll` / `libcrypto-1_1-x64.dll` 复制进产物（zip 与安装包均包含）。
- [ ] **Qt 6 迁移**：原 blocker（QCameraInfo / QCameraViewfinder / QAudioDeviceInfo）已随进程内 FFmpeg 预览移除，剩余为常规 API 迁移评估。
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
- 遮罩窗口修复、设置窗口布局尚未做视觉验证；新登录布局（760x480 左图右表单）与新主题只在 offscreen 快照里检查过构图，文字渲染 / DPI / 阴影需要实机过一眼（`POSTLIVE_SMOKE_TEST=login` + `POSTLIVE_SNAPSHOT=路径` 可重新生成快照）。
- gdigrab 窗口捕获按**标题**匹配（ffmpeg 限制）：同标题窗口无法区分、最小化窗口不可捕获。
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

### H 进程内编码推流（替换 stdin 管道架构）

- [x] **发现并定位 stdin 管道致命缺陷**：ffmpeg 7 对管道输入的调度会把所有处理积压到 EOF（实时喂入零消费，TCP 输入同样；mpegts/hls 输出均零字节），minidump + 隔离复现确认。
- [x] `SceneEncoder`：进程内 libav 编码——合成画布 BGRA→sws→YUV420P→libopenh264→libavformat hls muxer 直写 `hls/` 目录；时间戳由应用生成（从 0 单调），分片实时落盘（ffprobe 验证 h264 1280x720@30）。
- [x] LivePlatform 本地 HLS 模式切换到 SceneEncoder；控制条显示播放页地址；FFmpeg stdin 管道代码移除（RTMP 进程内迁移列为待办）。
- [x] e2e：无头自动开播 → 分片实时生成 → nginx 分发 → 播放页可播。

### G 直播页对齐 OBS（采集引擎 + 画布交互 + 合成推流）

- [x] 窗口采集换 **PrintWindow + PW_RENDERFULLCONTENT**（`src/media/capture/WindowPrintCapture`）：修复 Chrome/Electron 等硬件加速窗口采集黑屏（gdigrab 遗留问题）。
- [x] 屏幕采集换 **DXGI 桌面复制**（`src/media/capture/DesktopDuplicationCapture`）：多屏逐显示器选择（+ 菜单按显示器列出）、捕获线程独立 DPI 上下文、模式切换自动重初始化、手动绘制鼠标指针；offscreen 探针验证真实桌面帧。
- [x] 预览画布重构：固定 16:9（=推流分辨率）合成画布，OBS 式交互——点击选中（列表联动）、拖拽移动、右下角手柄缩放，首次出帧自动按源宽高比修正；选择框只画在预览层、不进推流画面。
- [x] 推流架构改为 **OBS 式合成推流**：ScenePreview 以推流分辨率合成画布，`canvasReady` 通过 stdin 以 rawvideo/bgra 喂给 ffmpeg.exe（背压丢帧保护），`buildStreamPipeArgs` 重写参数构建；**预览与推流同源所见即所得，可边推流边改场景**，dshow 设备不再被双开占用。
- [x] 控制 dock 新增"音轨"区：麦克风音频开关（持久化 stream.audio_enabled）；设置对话框已有麦克风设备选择。
- [x] `POSTLIVE_SMOKE_CAPTURE=desktop|window:<标题>` 无头采集探针；WGC 的 Win10/Win11 双路径（Win11 TryCreateFromWindowId、Win10 IGraphicsCaptureItemInterop）。
- [x] 本地观看：设置勾选"本地 HLS 输出"后，合成画布直接写 HLS（2s 分片环形缓冲）到 exe 目录 hls/，官方 nginx 静态分发 + hls.js 播放页（[tools/nginx/](tools/nginx/)，本机已配置 http://localhost:18080/player.html）；已知限制：文件源音频暂不进推流、桌面光标为简化箭头绘制、HLS 起播延迟 4–10s。

### F 直播页推倒重做（提案 B：OBS 式源管理 + 多源合成）

- [x] `StreamSource` 场景源模型（kind/name/title，JSON 持久化到 `scene.sources`，kind 越界钳制）；场景 = 有序列表，首层为主画面、其余自下而上叠加。
- [x] `buildSceneArgs` 多源推流参数构建器：多视频输入 + `filter_complex` 画中画链（PPI 缩放叠加右下角）、水印作为顶层自动加入、麦克风/文件音频映射规则、错误场景拒绝；`tests/test_scene_args.cpp` 9 个契约用例。
- [x] `ScenePreview` 合成预览控件：每源一个 FFmpegVideo 解码线程、按 id 路由帧、画布内合成绘制（主层居中 + PiP 右下角堆叠）、拖入媒体文件即添加为源；替代并删除 FFmpegWidget。
- [x] `LivePlatform` 重写为 OBS 式三栏：左侧源列表 dock（＋菜单添加 摄像头/屏幕/窗口/文件、移除、上移/下移、添加用设备栏）、中央画布、右侧控制 dock（开始/停止、状态、进度、设置）、底部状态条；场景持久化 + 首启自动种入摄像头源。
- [x] 设置对话框新增"麦克风"（推流音频）下拉；offscreen 快照验证三栏布局与实时摄像头预览。

### E 主控制页重做（提案 A：直播姬式极简单屏）

- [x] `LivePlatform.ui` 重排：标题栏（PostLive 文字品牌，替换 pilipili 图）→ 采集源卡片行（摄像头/整个屏幕/窗口/本地文件，QButtonGroup 互斥）→ 上下文行（按源显示摄像头+麦克风 / 窗口列表 / 文件选择）→ 大预览画布 → 控制条（开始/停止互斥显示 + 状态 + 实时进度 + 设置按钮）。
- [x] 推流参数（服务器/分辨率/码率/帧率/编码器）全部迁入 `LiveLoginSettingDialog` 新增的"直播设置"区（rtmp(s) 校验 + 持久化 stream.* 配置）。
- [x] 实时状态：ffmpeg 参数加 `-progress pipe:1`，控制条解析 stdout 显示 直播时长 / 码率 / FPS；`buildStreamArgs` 测试同步更新。
- [x] offscreen 快照验证：源卡片选中态、摄像头实时预览渲染、控制条布局均正常。

### D 合并 PostLive.Re（进程内 FFmpeg 采集 + UI 主题）

- [x] `third_party/ffmpeg/sdk`：FFmpeg 7.x SDK（LGPL-3.0）入库，与自带 av*-61 运行时 DLL 主版本匹配，进程内链接 avcodec / avdevice / avformat / avutil / swscale。
- [x] `src/media` 三件套（自 PostLive.Re 移植）：`FFmpegInputDevice`（dshow 枚举 + gdigrab 桌面/窗口 + 本地文件）、`FFmpegVideo` 解码线程、`FFmpegWidget` 统一预览（加载动画、错误内嵌、拖文件切源）。
- [x] 完整窗口选择器：`src/platform/WindowList`（过滤最小化 / 工具窗口 / cloaked UWP / 空标题），主窗口“来源”行新增 摄像头 / 整个屏幕 / 窗口 / 本地文件 四种源。
- [x] 推流参数抽为纯函数 `buildStreamArgs()`（gdigrab + 麦克风、文件 `-re` 实时推 + 自带音轨），新增 `tests/test_stream_args.cpp` 契约测试；Qt Multimedia 依赖移除（设备枚举改用 avdevice）。
- [x] UI：全局主题 `src/resources/postlive.qss`（深炭色 + 蓝 accent，468 行，源自 PostLive.Re）；登录窗口重构为 760x480 左图右表单（`ImageLabel` cover 裁剪，保留头像 / 记住密码 / 自动登录 / 设置 / 内嵌错误提示全部功能）；主窗口 / 设置窗口换肤；登录成功后 splash 动画。
- [x] 冒烟扩展：`POSTLIVE_SMOKE_TEST=platform`（无头构造主窗口）与 `=login`（渲染登录框并存 POSTLIVE_SNAPSHOT 快照），均已在 offscreen 验证通过。

### B 认证与后端（B3：真实服务端 + 凭证安全）

- [x] `worker/`：Cloudflare Workers 认证服务端（ES Module，无构建步骤）——`login`（密码 / token）/ `profile` / `query` / `heartbeat` 与 `docs/api.md` 一致，另有管理员 `register`（`x-admin-token` 头）；D1 存用户，KV 存 token（30 天）/ 失败计数（10 次 / 15 分钟锁定）/ 在线标记（15 分钟 TTL）。
- [x] 密码存储：PBKDF2-SHA256（每用户随机盐 + `PEPPER` secret，50 000 次，迭代数随用户入库）；客户端协议不变（HTTPS + 大写 MD5 hex，服务端视为密码等价物，绝不明文存储）。
- [x] Worker 单元测试（`node --test`，11 个用例，stub D1/KV）：注册校验、密码 / token 登录、token 与账号绑定、profile 字段过滤、heartbeat→query 在线状态、失败锁定、注册 upsert；CI 独立 Linux job 运行。
- [x] mock server 同步：同样的 PBKDF2 存储格式（`password_salt` / `password_hash` / `password_iterations`），`--hash` 子命令生成字段，旧格式文件自动回退；curl 端到端验证五条链路。
- [x] 文档：`worker/README.md`（部署 / 注册 / 配额 / 风险）、`docs/api.md`、`docs/cloudflare.md`、`docs/database.md`、`tools/mock-server/README.md`。

### B 认证与后端（客户端侧）

- [x] API 地址可配置（`auth_server`），设置窗口可编辑。
- [x] `tools/mock-server`：login / profile / query / heartbeat。
- [x] 认证改用 QtNetwork 异步请求；支持 http / https、5 s 超时、系统代理。
- [x] token 使用 Windows DPAPI 加密存储；旧明文 token 自动迁移。
- [x] `POSTLIVE_OPENSSL_DIR` 部署 OpenSSL 1.1.x；`POSTLIVE_SMOKE_TEST` 冒烟入口。
- [x] Cloudflare 免费认证 / 推流方案调研（见 [docs/cloudflare.md](docs/cloudflare.md)）。
- [x] DirectUI 迁移可行性调研（见 [docs/directui-evaluation.md](docs/directui-evaluation.md)）。
