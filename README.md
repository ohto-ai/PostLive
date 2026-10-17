# PostLive — 学习直播平台客户端

PostLive 是一个学习性质的直播平台桌面客户端，使用 C++ / Qt 编写，包含登录、用户信息、摄像头采集、视频预览与 FFmpeg 推流，并提供配套的 RTMP 服务端部署文档。

> 这是一个课程 / 学习项目，代码年代较早，功能与接口可能不完整，请勿直接用于生产环境。

## 功能状态

### 登录界面

- [x] UI 设计
- [x] 头像自动获取及显示
- [x] 用户名和密码自动填充（填充假密码）
- [x] 用户信息同步
- [x] 登录 token
- [ ] 自动登录
- [ ] 设置界面（服务器设置、官网、遮罩窗口）
- [x] 遮罩窗口

### 主窗口

- [x] 摄像头获取与选择
- [x] 视频预览
- [x] FFmpeg 推流及停止
- [ ] 麦克风获取与选择
- [ ] 输出分辨率选择
- [ ] 码率设置
- [ ] 设置窗口
- [ ] 预览开关

### 其他

- [ ] 最小化到托盘
- [ ] 遮罩窗口最小化还原后无法显示遮罩层

## 环境要求

- Windows 10 / 11 x64
- Visual Studio 2019 / 2022
- Qt 5.x（MSVC 版本）
- FFmpeg（仓库 `3rd/` 下提供）
- MySQL 客户端库（`mysqlpp` / `libmysql`，用于登录单元）
- 推流服务端：Nginx + `nginx-rtmp-module`（部署步骤见 [`doc/install.md`](doc/install.md)）

## 构建

1. 用 Visual Studio 打开 [`LivePlatformClient/LivePlatformClient.sln`](LivePlatformClient/LivePlatformClient.sln)。
2. 在 Qt VS Tools 中配置本机的 Qt 5 版本。
3. 选择 `Release | x64`（或 `Debug | x64`）构建。
4. 各功能单元（登录、直播、连接池、FFmpeg demo、测试服务器等）为解决方案中的独立子项目，可按需编译。

## 运行

- 登录单元依赖测试服务器与 MySQL 数据库，配置说明见 [`Database.md`](Database.md) 和 [`API.md`](API.md)。
- 直播单元会枚举摄像头并调用 FFmpeg 进行推流；推流命令与参数见 [`doc/ffmpegcmd.md`](doc/ffmpegcmd.md) 和 [`doc/推流相关.md`](doc/推流相关.md)。
- RTMP 服务端（Nginx + nginx-rtmp-module）的编译与配置见 [`doc/install.md`](doc/install.md)。

## 项目结构

```text
LivePlatformClient/
├── ConnectionPoolUnit/       MySQL 连接池示例
├── LiveBroadcastUnit/        直播 / 推流单元
├── LiveLoginUnit/            登录单元
├── LiveServerTestDemo/       测试服务端
├── PilipiliLivePlatform/     主客户端
├── ffmpeg-demo/              FFmpeg 解码 / 预览示例
├── ffmpeg-process/           FFmpeg 进程调用示例
├── QtHttpDownloader/         HTTP 下载组件
├── ShadowWindowDemo/         遮罩窗口示例
└── PortablePacckage/         便携打包相关
API.md                       后端 API 说明
Database.md                  数据库说明
doc/                         安装、推流与 RTMP 文档
```

## 说明

- 用户名正则：`^[A-Za-z][A-Za-z0-9_-]{3,15}$`
- 密码正则：`^[A-Za-z0-9._~!@#$^&*]{6,24}$`
- 客户端重构版本见 [`PostLive.Re`](https://github.com/ohto-ai/PostLive.Re)。

## License

[GPL-3.0](LICENSE)
