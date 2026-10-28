# 常用 FFmpeg 命令

> 下面的命令在 Windows `cmd` / PowerShell 与 Git Bash 中均可使用。把 `<...>` 替换成实际设备名、路径或地址。
> 首次使用前建议先运行 `ffmpeg -version` 与设备列表命令。

## 设备与格式

### 列出 DirectShow 设备

```bash
ffmpeg -list_devices true -f dshow -i dummy
```

### 查看摄像头支持的分辨率 / 帧率

```bash
ffmpeg -f dshow -i video="<摄像头名称>" -list_options true
```

## 摄像头

### 预览（不保存）

```bash
ffplay -f dshow -i video="<摄像头名称>"
```

### 录制 5 秒

```bash
ffmpeg -f dshow -i video="<摄像头名称>" -t 5 camera-test.mp4
```

### 推流

```bash
ffmpeg -f dshow -i video="<摄像头名称>" \
       -c:v libx264 -preset veryfast -tune zerolatency \
       -f flv rtmp://<server>:1935/live/<stream>
```

## 桌面采集

### 录制桌面

```bash
ffmpeg -f gdigrab -framerate 30 -i desktop -t 10 desktop.mp4
```

### 推流桌面

```bash
ffmpeg -f gdigrab -framerate 30 -i desktop \
       -c:v libx264 -preset veryfast -tune zerolatency \
       -f flv -s 1280x720 rtmp://<server>:1935/live/<stream>
```

## 声音

### 录制系统声音（需要 virtual-audio-capturer）

```bash
ffmpeg -f dshow -i audio="virtual-audio-capturer" -t 10 audio.wav
```

### 摄像头 + 麦克风录制

```bash
ffmpeg -f dshow -i video="<摄像头名称>" \
       -f dshow -i audio="<麦克风名称>" \
       -t 10 camera-with-audio.mp4
```

### 摄像头 + 麦克风推流

```bash
ffmpeg -f dshow -i video="<摄像头名称>" \
       -f dshow -i audio="<麦克风名称>" \
       -c:v libx264 -preset veryfast -tune zerolatency -c:a aac \
       -f flv rtmp://<server>:1935/live/<stream>
```

## 水印 / 叠加

### 摄像头 + logo 推流（客户端使用的形式）

```bash
ffmpeg -f dshow -i video="<摄像头名称>" \
       -i assets/pilipili-logo.png \
       -filter_complex "overlay=5:5" \
       -f dshow -i audio="<麦克风名称>" \
       -f flv -s 1280x720 rtmp://<server>/livehime
```

### 桌面 + logo 推流

```bash
ffmpeg -f gdigrab -framerate 30 -i desktop \
       -i assets/pilipili-logo.png \
       -filter_complex "overlay=5:5" \
       -f dshow -i audio="virtual-audio-capturer" \
       -f flv -s 1280x720 rtmp://<server>:1935/live/<stream>
```

### 把 logo 放在右下角

```bash
-filter_complex "overlay=x=main_w-overlay_w-10:y=main_h-overlay_h-10"
```

### 给已有视频加水印

```bash
ffmpeg -i input.mp4 -i assets/pilipili-logo.png \
       -filter_complex "overlay=5:5" \
       -c:v libx264 -preset veryfast -c:a copy output.mp4
```

## 转流 / 拉流

### 拉流播放

```bash
ffplay -fflags nobuffer -nodisp rtmp://<server>:1935/live/<stream>
```

### 把 HLS 转推到 RTMP

```bash
ffmpeg -i http://<source>/playlist.m3u8 -c copy -f flv rtmp://<server>:1935/live/<stream>
```

### 循环推一个本地视频

```bash
ffmpeg -re -stream_loop -1 -i input.mp4 \
       -c:v libx264 -preset veryfast -c:a aac \
       -f flv rtmp://<server>:1935/live/<stream>
```

## HLS

```bash
ffmpeg -re -f lavfi -i testsrc=size=1280x720:rate=30 \
       -f lavfi -i sine=frequency=1000 \
       -c:v libx264 -preset ultrafast -tune zerolatency -c:a aac \
       -f hls -hls_time 3 -hls_list_size 5 \
       -hls_segment_filename "out/seg_%03d.ts" out/index.m3u8
```

## 排查

- `Unknown input format: 'dshow'`：FFmpeg 构建不含 DirectShow，换用 Windows 官方 / gyan.dev 构建。
- `Could not find video device`：设备名不一致或被占用；重新运行 `-list_devices`。
- 推流成功但播放失败：先确认服务端应用名与 stream key，再检查防火墙。
- 花屏 / 卡顿：降低分辨率或码率，使用 `-preset veryfast -tune zerolatency`。

更多推流说明见 [streaming.md](streaming.md)，服务端部署见 [rtmp-server.md](rtmp-server.md)。
