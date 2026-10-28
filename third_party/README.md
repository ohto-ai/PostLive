# third_party

本目录只存放**头文件形式**的第三方依赖。它们随源码一起编译，不通过包管理器安装。

| 依赖 | 版本 | 文件 | 许可证 |
| --- | --- | --- | --- |
| [cpp-httplib](https://github.com/yhirose/cpp-httplib) | 0.11.2 | `cpp-httplib/httplib.h` | MIT |
| [nlohmann/json](https://github.com/nlohmann/json) | 3.11.2 | `nlohmann/json.hpp`、`json_fwd.hpp` | MIT |

完整许可证文本见仓库根目录的 [THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md)。

## FFmpeg

`third_party/ffmpeg/` 是本地放置 FFmpeg 运行时的地方（`ffmpeg.exe` + `av*.dll` / `sw*.dll`），**不纳入版本控制**。原因：

- 完整开发包约 127 MB；
- 不同构建的许可证与编解码器支持不同；
- 客户端只以子进程方式调用 `ffmpeg.exe`，不需要链接 libav*。

CMake 会优先在该目录查找 `ffmpeg.exe`，也可以通过 `-DPOSTLIVE_FFMPEG_DIR=...` 指定其他位置。

## 更新依赖

1. 下载对应版本的官方发布文件；
2. 覆盖本目录中的头文件；
3. 更新上表中的版本号与 `THIRD_PARTY_NOTICES.md`；
4. 重新构建并验证 HTTP / JSON 相关功能。

不要直接修改第三方头文件；需要补丁时优先升级版本，并在提交信息中说明原因。
