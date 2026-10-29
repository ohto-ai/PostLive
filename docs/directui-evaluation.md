# 去除 Qt、改用 DirectUI 的可行性调研

> 调研日期：2026-10-29。结论会随 DuiLib / nim_duilib / Windows SDK 版本变化。

## 先明确“DirectUI”指什么

“DirectUI”在中文 Windows 开发语境里通常有三种含义，结论完全不同：

| 含义 | 说明 | 能否用于本项目 |
| --- | --- | --- |
| Microsoft 内部 DirectUI | `dui70.dll` / `duser.dll` / `directui.dll`，Explorer、任务管理器、部分系统组件使用；未公开文档、无兼容性承诺 | **不可行**：非公开 API，版本脆弱，法律/授权状态不明确 |
| 开源 DirectUI 库 | DuiLib、NIM_Duilib、SOUI 等，XML 布局 + 自绘控件 | **技术可行**，但属于整体重写 |
| 即时模式 UI | 例如 Dear ImGui | 不适配当前换肤式桌面 UI |

本文按第二种（DuiLib / nim_duilib）评估。**不要直接调用 Windows 自带的 `dui70.dll` / `duser.dll`**：它们是 Microsoft 内部实现，没有公开头文件与稳定性保证，第三方商业使用存在法律风险。

## 结论

| 问题 | 结论 |
| --- | --- |
| 能否去掉 Qt 改用 DirectUI | **技术上可以，但不是“替换依赖”，而是重写 UI、多媒体、网络适配层** |
| 当前是否建议做 | **不建议作为当前主线**。项目是 Windows-only、代码量不大，但 Qt 的多媒体/网络/控件能力被大量使用 |
| 如果一定要做 | 选 **DuiLib（MIT）** 或 **rhett-lee/nim_duilib（MIT）**；不要用 Microsoft 内部 DirectUI，也不要用 SOUI 4（许可证与 GPL-3.0 冲突） |
| 更便宜的替代 | 保留 Qt，迁移 Qt 6；或为了体积/许可证做 Qt 静态构建（注意 LGPL 义务） |
| 若目标是学习 DirectUI | 另开一个 PoC 分支或独立 demo，不要直接重写可用版本 |

## 当前 Qt 耦合盘点

| 项目 | 数量 / 说明 |
| --- | --- |
| C++ 代码 | 约 **1,968 行**（`src/app`、`src/core`、`src/widgets`、`main.cpp`） |
| Qt Designer UI | **3 个 `.ui`，约 2,172 行** |
| `Q_OBJECT` 类 | 12 个 |
| `connect()` | 约 43 处 |
| Qt 模块 | Core、Gui、Widgets、Network、Multimedia、MultimediaWidgets |
| 资源 | 25 个 qrc 图片 / 图标 |
| 非 Qt 依赖 | `nlohmann/json`、`cpp-httplib`、FFmpeg（子进程）——这些可以保留 |

Qt 使用最重的区域：

| 区域 | 主要类 | 迁移影响 |
| --- | --- | --- |
| 窗口 / 控件 / 布局 | `QWidget`、`QDialog`、`QMainWindow`、`QVBoxLayout`、`QGridLayout`、`QLabel`、`QLineEdit`、`QComboBox`、`QCheckBox`、`QPushButton` | `.ui` 不能自动转换，需要手写 DuiLib XML + 自定义控件 |
| 无边框窗口 | `Dragable*`、`ShadowWidget`、`QMouseEvent`、`QPainter` | DuiLib 支持分层窗口/阴影，但要重写拖动、命中测试、遮罩 |
| 摄像头预览 | `QCamera`、`QCameraViewfinder`、`QCameraInfo` | 需要 Media Foundation（或 FFmpeg rawvideo）重写 |
| 麦克风枚举 | `QAudioDeviceInfo` | 需要 MMDevice API / FFmpeg dshow 列表 |
| 网络下载 | `QNetworkAccessManager` | 换成 WinHTTP/WinINet 或 cpp-httplib + 线程 |
| 进程 | `QProcess` | 换成 `CreateProcess` + 管道 |
| 托盘 / 菜单 | `QSystemTrayIcon`、`QMenu`、`QAction` | 换成 `Shell_NotifyIcon` + 自绘菜单 |
| 定时器 | `QTimer` | `SetTimer` / 工作线程 |
| 字符串 / 文件 | `QString`、`QFile`、`QDir`、`QFileInfo` | `std::wstring` / `std::filesystem` / Win32 |
| 图片 / 绘图 | `QPixmap`、`QPainter`、`QBitmap` | WIC + GDI+/D2D/Skia |
| 资源 | `.qrc` | 文件系统、Windows resources 或 DuiLib 资源包 |
| JSON / HTTP | `nlohmann`、`cpp-httplib` | 可保留；但 HTTPS 需要 OpenSSL 或 WinHTTP |

## DirectUI 方案对比

| 方案 | 许可证 | 构建 | 渲染 | 维护 | 结论 |
| --- | --- | --- | --- | --- | --- |
| Microsoft DirectUI (`dui70.dll`/`duser.dll`) | 未公开/不明确 | 无公开 SDK | 系统内部 | Microsoft 内部 | **不可行** |
| DuiLib (`github.com/duilib/duilib`) | MIT | CMake + vcpkg，x64 | GDI/GDI+，可选 Skia | 活跃，vcpkg 持续更新 | **最贴近经典 DirectUI，推荐做 PoC** |
| NIM_Duilib_Framework（网易原版） | MIT | VS 解决方案，无 CMake | GDI | 基本停更 | 不推荐直接用 |
| rhett-lee/nim_duilib（NIM 分支） | MIT | CMake，C++20 | Skia + SDL3 | 活跃 | **更现代，但依赖更重** |
| SOUI 4 | SOUI 3+ 非自由许可 | CMake | GDI/D2D/Skia | 活跃 | **不建议**：公司商用需付费，和 GPL-3.0 不兼容 |

DuiLib 的 MIT 许可证与项目的 GPL-3.0 兼容；NIM 分支同样是 MIT。SOUI 3+/4 的“个人免费、公司付费”条款不是 OSI 自由许可证，不能安全地用于 GPL-3.0 项目。

## 关键技术缺口：摄像头预览

这是整个迁移里最难的部分，不是 UI。

Qt 的 `QCamera` / `QCameraViewfinder` 已经封装了设备枚举、格式协商、预览渲染。改用 DirectUI 后需要自己实现：

1. **设备枚举**：`MFEnumDeviceSources`；
2. **预览**：`IMFSourceReader` 读取 NV12/YUY2 帧，转成 RGB 后画到 DuiLib 控件，或用 `MFCaptureD3D` 的思路渲染到 D3D11 纹理；
3. **格式协商**：摄像头支持的分辨率 / 帧率；
4. **设备热插拔**：`IMMNotificationClient` 或定时枚举；
5. **麦克风枚举**：`IMMDeviceEnumerator` / MMDevice API，并映射到 FFmpeg 的 dshow 设备名。

Microsoft 官方示例：

- `MFCaptureD3D`：`IMFMediaSource` + `IMFSourceReader` + `MFEnumDeviceSources`，用 Direct3D 预览；
- `MFCaptureToFile`：摄像头录制；
- `CaptureEngineVideoCapture`：`IMFCaptureEngine` 预览/录制/拍照。

没有“DirectUI + Media Foundation”的现成教程，需要自己把 MF 帧接到 DuiLib 控件。另一个可行路线是**继续用 FFmpeg 做预览**：启动第二个 FFmpeg 进程，把 dshow 摄像头输出成 rawvideo 管道，再画到 DuiLib；这样不用写 MF，但增加 CPU、延迟和进程管理复杂度，且设备名仍需枚举。

## 工作量估算

按“保留 FFmpeg 推流、保留 nlohmann/httplib、只替换 UI + 多媒体 + 平台适配”估算：

| 阶段 | 内容 | 人日 |
| --- | --- | --- |
| PoC | DuiLib 静态链接 + 一个窗口 + 登录界面 + MF 摄像头预览 | 3–5 |
| UI 移植 | 3 个 `.ui`、12 个类、43 个 connect、无边框/阴影/圆角头像/密码框/补全 | 10–15 |
| 多媒体 | MF 预览、设备枚举、热插拔、麦克风映射 | 8–15 |
| 平台适配 | WinHTTP/cpp-httplib、托盘、进程、定时器、资源、字符串 | 4–6 |
| 测试 / 打磨 | Win10/11、DPI、多显示器、真实摄像头/麦克风、回归 | 5–10 |
| **合计** | | **30–51 人日（约 6–10 周全职）** |

AI 执行大约 **40–70 小时**有效工作，其中摄像头/麦克风的实机验证不可省略。这个估算不包含 Cloudflare/认证改造，也不包含重新设计 UI。

## 收益与代价

**收益**

- 安装包显著变小：DuiLib 核心 < 2 MB，Qt 动态部署通常 40 MB+；
- 去掉 Qt DLL、`windeployqt`、Qt 插件目录；
- 避免 Qt LGPL 合规与 Qt 6 迁移；
- 纯 Win32/GDI 路径，启动与内存占用更低；
- 如果本来就要重做 UI，DirectUI 的 XML 换肤更贴近 Windows 客户端风格。

**代价**

- 这是一次**整体重写**，不是重构；回归风险高；
- DuiLib 文档少、生态小，遇到问题可参考的案例远少于 Qt；
- 摄像头预览/设备枚举要自己实现，Media Foundation 学习成本高；
- 失去 Qt 的跨平台能力（当前项目本来也只用 Windows）；
- DuiLib 是 GDI/GDI+ 自绘，高频动画/大列表性能有限；需要 GPU 渲染时得上 Skia 分支；
- 需要重新做 DPI、无障碍、输入法、托盘、深色主题等细节。

## 替代方案

| 目标 | 更便宜的做法 |
| --- | --- |
| 减小安装包 / 去掉 Qt DLL | Qt 静态构建；注意 LGPL 要求提供可重链接的目标文件/安装信息 |
| 避免 Qt 5 停止维护 | 迁移 Qt 6（Qt 6 的 Multimedia API 需要重写摄像头部分） |
| 使用现代 Windows 原生 UI | WinUI 3 / Windows App SDK（C++/WinRT，仍是重写） |
| 最小依赖、可接受全手写 UI | WTL / 纯 Win32 |
| 学习 DirectUI | 独立 demo / PoC 分支，不影响主线 |

## 建议

1. **当前不做**：Qt 版本已经能构建、能推流，DirectUI 迁移的收益主要是体积/许可证，成本是 6–10 周重写。
2. **如果目标是学习 DirectUI**：新建 `experiments/directui-poc` 分支或独立仓库，用 DuiLib MIT + `MFCaptureD3D` 做一个“登录 + 摄像头预览”最小闭环，时间盒 3–5 天；验证通过再讨论是否迁移。
3. **如果目标是去掉 Qt 依赖**：优先评估 Qt 静态构建或 Qt 6；这两条路都比 DirectUI 迁移风险低。
4. **如果确定要迁移**：先冻结功能，保留 `src/core` 的配置/JSON/FFmpeg 命令逻辑，按“UI → 多媒体 → 平台适配”顺序推进，并保留 Qt 版本作为对照。
5. **Cloudflare/认证改造与此无关**，可以先做；它不会因为 UI 框架变化而白做。

## 参考资料

- [DuiLib（MIT）](https://github.com/duilib/duilib)
- [DuiLib LICENSE](https://github.com/duilib/duilib/blob/master/LICENSE)
- [rhett-lee/nim_duilib（MIT）](https://github.com/rhett-lee/nim_duilib)
- [NIM_Duilib_Framework（网易原版）](https://github.com/netease-im/NIM_Duilib_Framework)
- [SOUI 4](https://github.com/soui4/soui)
- [SOUI 3+ License](https://github.com/soui4/soui/blob/master/LICENSE-en.md)
- [vcpkg duilib](https://vcpkg.io/en/package/duilib)
- [Microsoft DirectUI（ReactOS 文档）](https://reactos.org/wiki/DirectUI)
- [What is the fact of DirectUI?（Stack Overflow）](https://stackoverflow.com/questions/3581518/what-is-the-fact-of-directui)
- [Media Foundation SDK samples](https://learn.microsoft.com/en-us/windows/win32/medfound/media-foundation-sdk-samples)
- [MFCaptureD3D sample](https://learn.microsoft.com/en-us/windows/win32/medfound/mfcaptured3d-sample)
- [Qt LGPL obligations](https://www.qt.io/licensing/open-source-lgpl-obligations)
