# Third-party notices

PostLive 本身以 [GPL-3.0](LICENSE) 发布。下面的组件随源码或运行时一起使用，请同时遵守各自的许可证。

## Qt 5.15

- 项目：<https://www.qt.io/>
- 许可证：LGPL-3.0 / GPL-2.0 / GPL-3.0 / 商业许可（取决于所安装的套件）
- 使用方式：动态链接 Qt 5 库。
- 分发时应附带 Qt 的许可证文本，并允许用户替换 Qt 动态库。

## FFmpeg

- 项目：<https://ffmpeg.org/>
- 许可证：默认 LGPL-2.1-or-later；启用 GPL 组件（如 x264）的构建为 GPL。
- 使用方式（两种）：
  1. **独立进程**：推流通过 `ffmpeg.exe` 完成；
  2. **进程内链接**：预览解码链接 `third_party/ffmpeg/sdk/` 中的 FFmpeg 7.x SDK（LGPL-3.0，见该目录 `LICENSE.txt`），运行时加载 `avcodec-61.dll` 等 FFmpeg 7.x 动态库。仅使用 libav 的解码 API，不修改 FFmpeg。
- FFmpeg 二进制不在本仓库中（`third_party/ffmpeg/bin/`、`third_party/ffmpeg/sdk/lib/*.dll.a` 除外——后者仅是开发用导入库）。若在安装包中分发 FFmpeg 动态库，请附带对应许可证（`third_party/ffmpeg/sdk/LICENSE.txt`）与源码获取方式。

## cpp-httplib 0.11.2

- 项目：<https://github.com/yhirose/cpp-httplib>
- 许可证：MIT
- 文件：`third_party/cpp-httplib/httplib.h`

```text
MIT License

Copyright (c) 2022 Yuji Hirose

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

## nlohmann/json 3.11.2

- 项目：<https://github.com/nlohmann/json>
- 许可证：MIT
- 文件：`third_party/nlohmann/json.hpp`、`third_party/nlohmann/json_fwd.hpp`

```text
MIT License

Copyright (c) 2013-2022 Niels Lohmann

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

## OpenSSL 1.1.x（可选，HTTPS 认证）

- 项目：<https://www.openssl.org/>
- 许可证：OpenSSL License / SSLeay License（1.1.x）
- 使用方式：Qt 5.15 的 QtNetwork 在 Windows 上通过 OpenSSL 1.1.x 提供 TLS。
- 仓库不包含 OpenSSL DLL。如果安装包分发 `libssl-1_1-x64.dll` / `libcrypto-1_1-x64.dll`，请附带 OpenSSL 的许可证文本。

## Windows SDK / DbgHelp

Windows SDK 随 Visual Studio 安装，受 Microsoft 许可约束；DbgHelp 仅用于生成崩溃转储。

## Inno Setup

仅作为构建工具使用，不随源码分发。
