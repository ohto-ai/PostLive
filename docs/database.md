# 用户数据格式（开发示例）

> 这里的 JSON 是旧测试服务端 / 本地用户文件的示例格式，不是生产数据库设计。

## `users.json`（客户端本地）

客户端在可执行文件旁边读写 `users.json`，用于记住账号、token、密码长度和头像缓存路径。默认文件来自 [`config/users.default.json`](../config/users.default.json)：

```json
{
    "user_names": [
        "thatboy",
        "jaychou",
        "kongjh"
    ],
    "users": {
        "thatboy": {
            "token": "385B3D8302A4B0E7570D6158CD2FBCA8",
            "gender": "male",
            "avatar": "https://example.com/avatar.png",
            "avatar_file": "user/avatar/thatboy.avatar",
            "pwmask": 7,
            "remember_password": true
        },
        "jaychou": {
            "token": "E10ADC3949BA59ABBE56E057F20F883E",
            "gender": "female",
            "avatar": "https://example.com/avatar.jpg",
            "avatar_file": "user/avatar/jaychou.avatar",
            "pwmask": 7,
            "remember_password": true
        }
    }
}
```

| 字段 | 说明 |
| --- | --- |
| `user_names` | 账号列表，用于登录框自动补全 |
| `users.<account>.token` | 服务端返回的登录 token（MD5 大写） |
| `users.<account>.pwmask` | 密码长度，用于“记住密码”时回填假密码 |
| `users.<account>.remember_password` | 是否在登录框回填假密码 |
| `users.<account>.avatar` | 远程头像 URL |
| `users.<account>.avatar_file` | 本地头像缓存路径 |
| `users.<account>.gender` | 服务端返回的性别 |

> 该文件包含 token，已被 `.gitignore` 忽略，不应提交。示例中的 token 仅用于说明格式。

## `userdata.json`（旧测试服务端）

旧 `LiveServerTestDemo` 读取一个“用户数据库” JSON，结构如下：

```json
{
    "user_list": ["thatboy", "jaychou", "kongjh"],
    "users": {
        "thatboy": {
            "token": "FCD12713057D0B49997EB05F45017C35",
            "gender": "male",
            "avatar": "https://example.com/avatar.png"
        },
        "jaychou": {
            "token": "E10ADC3949BA59ABBE56E057F20F883E",
            "gender": "female",
            "avatar": "https://example.com/avatar.jpg"
        }
    }
}
```

服务端把 `token` 当作密码的 MD5（大写）或已签发的 token 使用。这个文件已随旧服务端一起移除，仅保留格式说明。

## 安全说明

- 密码 / token 都是 MD5，且 token 明文存放在本地 JSON；
- 旧服务端直接读取 JSON，没有数据库、盐值或过期机制；
- 不要把这些示例当作生产实现，详见 [TODO.md](../TODO.md)。

## 相关文档

- 接口协议：[api.md](api.md)
- 客户端配置：[architecture.md](architecture.md)
