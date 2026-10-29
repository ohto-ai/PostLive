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
ffmpeg.exe -hide_banner -loglevel warning -nostats
  -f dshow -i video="<摄像头名称>"
  -i assets/pilipili-logo.png
  -f dshow -i audio="<麦克风名称>"
  -filter_complex "overlay=5:5"
  -f flv
  [-c:v <编码器>] [-b:v <码率>k] [-r <帧率>]
  -pix_fmt yuv420p
  -s <宽x高>
  <服务器地址>/livehime
```

说明：

- 所有输入都放在输出参数之前：视频（0）、logo（1）、音频（2），`overlay` 使用前两个视频输入。
- `-f dshow` 是 Windows DirectShow 输入，设备名必须与系统名称完全一致。
- logo 叠加在画面左上角 `(5,5)`；路径相对于可执行文件目录。
- `-s` 来自分辨率下拉框，默认 `1280x720`；代码会把 `×` 归一化为 `x`。
- 编码器 / 码率 / 帧率为 `Auto` 时不追加对应参数，保留 FFmpeg 的默认选择。
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

- 主窗口的“预览”复选框控制本地预览；取消勾选会停止 `QCamera`。
- 开播时自动停止预览并禁用复选框；推流结束后如果复选框仍勾选，客户端会重新 `viewCamera->start()`。
- 如果摄像头被其他程序占用，开播会失败；错误会显示在状态栏，并弹出 FFmpeg 的 stderr。

## 停播

```cpp
stopRequested = true;
ffmpegProcess.write("q");
ffmpegProcess.closeWriteChannel();
// 3 秒后仍未退出则 terminate；再 2 秒后 kill
```

FFmpeg 收到 `q` 后正常结束。如果进程卡住，客户端会先 `terminate()`，再 `kill()`，避免留下后台进程。

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

编码器下拉框默认 `Auto`，此时不传 `-c:v`，由 FFmpeg 选择默认编码器。也可以选择 `libx264`、`h264_nvenc`、`h264_amf`、`h264_qsv`、`libopenh264`。

如果选择当前 FFmpeg 构建不包含的编码器，开播会失败；错误会显示在状态栏和对话框中。可以用下面的命令查看可用编码器：

```bash
ffmpeg -hide_banner -encoders | findstr h264
```

当前测试用的 FFmpeg 构建启用了 `libopenh264`，但未启用 `libx264`。

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
