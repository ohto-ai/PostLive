```
cd /usr/local/ &&sudo wget http://nginx.org/download/nginx-1.15.3.tar.gz && sudo tar -zxvf nginx-1.15.3.tar.gz && rm nginx-1.15.3.tar.gz && mv nginx-1.15.3 nginx && cd nginx && sudo ./configure --conf-path=/usr/local/nginx/nginx.conf --with-http_ssl_module --prefix=/usr/local/nginx --with-http_stub_status_module --add-module=/opt/module/nginx-rtmp-module && sudo chmod -R 777 . &&sudo make &&sudo make install
```

./configure --add-module=/opt/module/nginx-rtmp-module --with-http_flv_module --with-http_ssl_module && make

sudo  ./configure --add-module=/opt/module/nginx-rtmp-module --with-http_flv_module --with-http_ssl_module && sudo chmod -R 777 . &&sudo  make &&sudo  make install                   



./configure --prefix=/usr/local/nginx --user=nginx --group=nginx --with-http_ssl_module --with-http_gzip_static_module --with-http_stub_status_module --with-http_realip_module --pid-path=/var/run/nginx.pid --with-pcre=/opt/software/pcre-8.35 --with-zlib=/opt/software/zlib-1.2.8 --with-openssl=/opt/software/openssl-1.0.1i --add-module=/opt/software/nginx-rtmp-module


./configure --prefix=/opt/nginx --with-http_ssl_module --with-http_gzip_static_module --with-http_stub_status_module --with-http_realip_module --pid-path=/var/run/nginx.pid \#pid路径
--with-pcre=/opt/software/pcre-8.35 \  #依赖pcre的源码路径
--with-openssl=/opt/software/openssl-1.0.1i  #依赖openssl的源码路径

--add-module=/opt/software/nginx-rtmp-module    #添加nginx-rtmp-module模块（要弄rtmp服务器必须加上这条）

make

make install


./configure --conf-path=/usr/local/nginx/nginx.conf --prefix=/usr/local/nginx --with-http_ssl_module --with-http_stub_status_module --add-module=/opt/module/nginx-rtmp-module

#rtmp_auto_push on;

#rtmp_auto_push_reconnect 1s;

rtmp{
    server{
        listen 1935;
        application  porn{
            live on;
        }
        application hls{
            live on;
            hls on;
            hls_path /temp/hls;
        }
        access_log logs/rtmp_access.log;
    }
}