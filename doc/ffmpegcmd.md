### 拉流

```shell
ffplay -fflags nobuffer -nodisp rtmp://thatboy.icu:1935/live/home
```

### 转流

```shell
ffmpeg -i http://ivi.bupt.edu.cn/hls/cctv1hd.m3u8 -f flv rtmp://thatboy.icu:1935/live/home
```

### 桌面

```shell
ffmpeg -f gdigrab -framerate 30  -i desktop -vcodec libx264 -f flv -s 960x540 rtmp://thatboy.icu:1935/live/home
```

### 摄像头

```shell
ffmpeg dshow -i video="USB2.0 HD UVC WebCam" -vcodec libx264 -preset:v ultrafast -tune:v zerolatency -f flv rtmp://localhost:1935/live/home

ffmpeg dshow -i video="USB2.0 HD UVC WebCam" -vcodec libx264 -f flv rtmp://localhost:1935/live/home
```

### 桌面直播 有水印

```shell
ffmpeg -f gdigrab -framerate 30  -i desktop -i E:/live.png -filter_complex "overlay=5:5" -vcodec libx264 -acodec libmp3lame -ar 44100 -ac 1 -f flv -s 960x540 rtmp://thatboy.icu:1935/live/home

ffmpeg -f gdigrab -framerate 30 -i desktop -i E:/live.png -filter_complex "overlay=5:5" -vcodec libx264 -f flv -s 1920x1080 rtmp://thatboy.icu:1935/live/home
```

## 桌面 摄像头

```shell
ffmpeg -f gdigrab -framerate 30 -i desktop -i E:\Documents\Downloads\Video\test.mp4 -video_size 400x300 -filter_complex "overlay=5:5" -vcodec libx264 -f flv -s 1920x1080 rtmp://thatboy.icu:1935/hls/test

ffmpeg -thread_queue_size 96 -f gdigrab desktop -i :0.0 video="USB2.0 HD UVC WebCam" -vcodec libx264 -video_size 400x300 -filter_complex '[0:v][1:v]overlay=x=main_w-overlay_w-10:y=main_h-overlay_h-10[out]' -f flv -s 1920x1080 rtmp://thatboy.icu:1935/live/home
```

```shell
ffmpeg -re -stream_loop -1 -i "E:\Documents\Downloads\Video\年度致郁短片：《你，真的不行》_哔哩哔哩 (゜-゜)つロ 干杯~-bilibili_2.mp4" -vcodec libx264 -vprofile baseline -acodec aac -ar 44100 -strict -2 -ac 1 -f flv -q 10 rtmp://thatboy.icu:1935/hls/test
```

http://47.115.142.94:8080/hls/test.m3u8

http://localhost/hls/test.m3u8

```shell
ffmpeg -f gdigrab -framerate 30  -i desktop -vcodec libx264 -f flv -s 960x540 rtmp://thatboy.icu:1935/hls/test
```

-i D:/thatboylogo.png -filter_complex "overlay=5:5"

ffmpeg.exe -i D:/output1.mp4 -i D:/thatboylogo.png -filter_complex "overlay=5:5" -vcodec copy -acodec copy D:output2.mp4  

ffmpeg.exe -i D:/output1.mp4 -i D:/thatboylogo.png -filter_complex "overlay = 10: main_h-overlay_h-10" D:/output2.mp4  

## 录制声音视频

```shell
ffmpeg.EXE -f dshow -i video="screen-capture-recorder":audio="virtual-audio-capturer" D:/av-out.mp4
```

## 录制声音视频(GDI视频)

```shell
ffmpeg.EXE -f gdigrab -i desktop -f dshow -i audio="virtual-audio-capturer" D:/av-out.mp4
```

## 视听直播(水印)[m3u8莫名失效]

```shell
ffmpeg.EXE -f gdigrab -i desktop -i D:/thatboylogo.png -filter_complex "overlay=5:5" -f dshow -i audio="virtual-audio-capturer" -f flv -s 960x540 rtmp://thatboy.icu:1935/hls/home
```

