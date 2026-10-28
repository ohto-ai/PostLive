# 推流实现

> 客户端通过 `QProcess` 调用外部 `ffmpeg.exe`，不链接 libav*。本文说明当前实现、设备名与排错方法。

## 流程

1. `LivePlatform` 用 `QCameraInfo::availableCameras()` 枚举摄像头，用 `QAudioDeviceInfo::availableDevices(QAudio::AudioInput)` 枚举麦克风。
2. 本地预览由 `QCamera` + `QCameraViewfinder` 完成。
3. 点击“开播”后：
   - 停止本地预览（避免设备被两个进程同时占用）；
   - 拼接 FFmpeg 命令行；
   - `QProcess::start(command)`；
   - 禁用摄像头 / 麦克风 / 服务器 / 分辨率控件。
4. 点击“停播”后向 FFmpeg 标准输入写入 `q`，1 秒后关闭 `QProcess` 通道；进程退出后由 `finished` 信号恢复 UI。
5. FFmpeg 退出后重新启动预览并恢复控件。

## 当前命令

`src/app/LivePlatform.cpp` 中拼接的命令：

```text
ffmpeg.exe
  -f dshow -i video="<摄像头名称>"
  -i assets/pilipili-logo.png
  -filter_complex "overlay=5:5"
  -f dshow -i audio="<麦克风名称>"
  -f flv
  -s <宽x高>
  <服务器地址>/livehime
```

说明：

- `-f dshow` 是 Windows DirectShow 输入，设备名必须与系统名称完全一致。
- logo 叠加在画面左上角 `(5,5)`；路径相对于可执行文件目录。
- `-s` 来自分辨率下拉框，默认 `1280x720`。
- 服务器地址来自主窗口输入框，默认 `rtmp://rtmp.ohtoai.top`。
- 代码固定追加 `/livehime`，最终推流地址形如 `rtmp://rtmp.ohtoai.top/livehime`。这个路径是硬编码的，建议后续改为 `<base>/live/<stream_key>`，见 [TODO.md](../TODO.md)。

## 设备名

列出 DirectShow 设备：

```powershell
ffmpeg -list_devices true -f dshow -i dummy
```

输出示例：

```text
[dshow @ ...] "USB2.0 HD UVC WebCam" (video)
[dshow @ ...] "Microphone (Realtek Audio)" (audio)
[dshow @ ...] "virtual-audio-capturer" (audio)
```

`virtual-audio-capturer` 来自 [screen-capture-recorder](https://github.com/rdp/screen-capture-recorder-to-video-windows-free)，用于采集系统声音；未安装时不会出现在列表中。

## 本地预览与推流

- 预览和推流不会同时进行：开播时摄像头被 FFmpeg 独占，客户端先停止 `QCamera`。
- 推流结束后客户端重新 `viewCamera->start()`。
- 如果摄像头被其他程序占用，开播会失败；关闭占用程序后重试。

## 停播

```cpp
QTimer::singleShot(1000, std::bind(&QProcess::close, &ffmpegProcess));
ffmpegProcess.write("q");
```

FFmpeg 收到 `q` 后正常结束。`QProcess::close` 只关闭通道，不会强制结束进程；如果 FFmpeg 卡住，需要手动结束进程。稳定性改进见 [TODO.md](../TODO.md)。

## 排错

### 1. 确认 FFmpeg 可用

```powershell
.\ffmpeg.exe -version
.\ffmpeg.exe -list_devices true -f dshow -i dummy
```

如果提示找不到 `ffmpeg.exe`，检查构建输出目录或 `POSTLIVE_FFMPEG_DIR`，见 [building.md](building.md)。

### 2. 先本地录制，再推流

```powershell
ffmpeg -f dshow -i video="<摄像头名称>" -t 5 test.mp4
```

本地录制成功说明设备与编码正常，问题在服务器或网络。

### 3. 确认 RTMP 服务端

```powershell
ffplay rtmp://<server>:1935/live/<stream>
```

服务端部署见 [rtmp-server.md](rtmp-server.md)。

### 4. 检查编码器

当前命令使用 `-f flv`，FFmpeg 会自动选择默认视频 / 音频编码器。如果构建不含 `libx264`，可能回退到其他编码器或失败。可在命令中显式指定：

```text
-c:v libx264 -preset veryfast -tune zerolatency -c:a aac
```

客户端尚未暴露这些参数，见 [TODO.md](../TODO.md)。

### 5. 检查防火墙 / 端口

RTMP 默认监听 `1935/tcp`。云服务器需要在安全组放行端口。

## 相关文档

- 常用命令：[ffmpeg-commands.md](ffmpeg-commands.md)
- RTMP 服务端：[rtmp-server.md](rtmp-server.md)
- 架构与进程模型：[architecture.md](architecture.md)

## 参考资料

- <https://blog.csdn.net/litong131219/article/details/103083005>
- <https://blog.csdn.net/weixin_37897683/article/details/81225228>
- <https://blog.csdn.net/suzhenpeng199503/article/details/87369043>
- <https://blog.csdn.net/megary/article/details/97660033>
- <https://www.jianshu.com/p/06c2025edcd3>
- <https://www.jianshu.com/p/c141fc7881e7>
- <https://www.cnblogs.com/lipcblog/p/6824145.html>
- <https://blog.csdn.net/mxdzchallpp/article/details/86551564>
