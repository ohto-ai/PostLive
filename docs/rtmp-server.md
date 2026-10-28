# RTMP 服务端部署（Nginx + nginx-rtmp-module）

> 以下是在 Linux 服务器上部署 RTMP / HLS 服务的参考步骤，与客户端构建无关。
> 示例使用较旧的 Nginx 1.15.3 与 nginx-rtmp-module；生产环境请使用受支持的版本并配置 TLS。

## 1. 安装构建依赖

Debian / Ubuntu：

```bash
sudo apt update
sudo apt install -y build-essential libpcre3 libpcre3-dev zlib1g-dev libssl-dev git
```

## 2. 下载源码

```bash
cd /usr/local/src
wget http://nginx.org/download/nginx-1.15.3.tar.gz
tar -xzf nginx-1.15.3.tar.gz
git clone https://github.com/arut/nginx-rtmp-module.git
```

## 3. 编译安装

```bash
cd nginx-1.15.3
./configure \
    --prefix=/usr/local/nginx \
    --with-http_ssl_module \
    --with-http_stub_status_module \
    --add-module=../nginx-rtmp-module
make -j"$(nproc)"
sudo make install
```

## 4. 配置

编辑 `/usr/local/nginx/conf/nginx.conf`，保留默认的 `events` / `http` 结构，并加入 RTMP 与 HLS：

```nginx
worker_processes  auto;

events {
    worker_connections  1024;
}

rtmp {
    server {
        listen 1935;
        chunk_size 4096;

        # 纯 RTMP 直播
        application live {
            live on;
            record off;
        }

        # HLS 直播
        application hls {
            live on;
            hls on;
            hls_path /tmp/hls;
            hls_fragment 3s;
            hls_playlist_length 60s;
        }
    }
}

http {
    include       mime.types;
    default_type  application/octet-stream;
    sendfile      on;

    server {
        listen 8080;

        location /hls {
            types {
                application/vnd.apple.mpegurl m3u8;
                video/mp2t ts;
            }
            root /tmp;
            add_header Cache-Control no-cache;
            add_header Access-Control-Allow-Origin *;
        }

        location /stat {
            rtmp_stat all;
            rtmp_stat_stylesheet stat.xsl;
        }
    }
}
```

创建 HLS 目录并启动：

```bash
sudo mkdir -p /tmp/hls
sudo /usr/local/nginx/sbin/nginx -t
sudo /usr/local/nginx/sbin/nginx
```

## 5. 测试

推一路测试流：

```bash
ffmpeg -re -f lavfi -i testsrc=size=1280x720:rate=30 \
       -f lavfi -i sine=frequency=1000 \
       -c:v libx264 -preset ultrafast -tune zerolatency \
       -c:a aac -f flv rtmp://<server>:1935/live/test
```

播放 RTMP：

```bash
ffplay rtmp://<server>:1935/live/test
```

播放 HLS：

```text
http://<server>:8080/hls/test.m3u8
```

## 6. 客户端配置

主窗口的服务器地址默认为 `rtmp://rtmp.ohtoai.top`，客户端会在末尾追加 `/livehime`，因此实际推流地址是：

```text
rtmp://rtmp.ohtoai.top/livehime
```

如果要推送到本文的 `live` 应用，需要把该地址改为类似：

```text
rtmp://<server>:1935/live/<stream_key>
```

并调整 `src/app/LivePlatform.cpp` 中固定追加的 `/livehime`（TODO）。

## 7. 常见问题

| 问题 | 处理 |
| --- | --- |
| `nginx: command not found` | 使用 `/usr/local/nginx/sbin/nginx` 或加入 `PATH` |
| `./configure` 找不到 PCRE / OpenSSL | 安装对应的 `-dev` 包，或使用 `--with-pcre=...` 指定源码 |
| 推流成功但无法播放 | 检查安全组 / 防火墙是否放行 `1935/tcp` 与 `8080/tcp` |
| HLS 延迟大 | HLS 天然有分片延迟；低延迟场景优先使用 RTMP |
| 端口被占用 | `ss -lntp | grep -E '1935|8080'` |

## 历史笔记

旧文档中的命令还包含：

```text
ffplay -fflags nobuffer -nodisp rtmp://thatboy.icu:1935/live/home
ffmpeg -i http://ivi.bupt.edu.cn/hls/cctv1hd.m3u8 -f flv rtmp://thatboy.icu:1935/live/home
ffmpeg -f gdigrab -framerate 30 -i desktop -vcodec libx264 -f flv -s 960x540 rtmp://thatboy.icu:1935/live/home
```

其中的域名为历史示例，已不可用；命令形式可参考 [ffmpeg-commands.md](ffmpeg-commands.md)。
