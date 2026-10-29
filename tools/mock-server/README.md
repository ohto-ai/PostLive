# PostLive mock auth server

开发 / 联调用的最小认证服务端，实现 [docs/api.md](../../docs/api.md) 中的 `login`、`profile`、`query`、`heartbeat`。

> 仅用于本地开发。密码和 token 使用 MD5，没有 HTTPS、数据库、限流或过期机制。

## 构建

```powershell
$env:QT_ROOT = "C:/Qt/5.15.2/msvc2019_64"
cmake --preset windows-x64
cmake --build --preset release
```

产物：`build/windows-x64/Release/PostLiveMockServer.exe`。

## 运行

```powershell
# 默认读取 users.example.json，监听 127.0.0.1:8877
.\build\windows-x64\Release\PostLiveMockServer.exe

# 指定用户文件和端口
.\build\windows-x64\Release\PostLiveMockServer.exe .\tools\mock-server\users.example.json 8877
```

示例用户：

- 账号：`test`
- 密码：`123456`（token = MD5(`123456`) = `E10ADC3949BA59ABBE56E057F20F883E`）

客户端保持默认认证地址 `http://localhost:8877`，或通过设置窗口改成 `http://127.0.0.1:<port>`。

## 用户文件格式

```json
{
    "user_list": ["test"],
    "users": {
        "test": {
            "token": "E10ADC3949BA59ABBE56E057F20F883E",
            "gender": "male",
            "avatar": "https://example.com/avatar.png"
        }
    }
}
```

`token` 可以是密码的 MD5，也可以是任意字符串；客户端用 token 登录时会直接比较。
建议复制一份 `users.json` 后修改，不要提交真实账号。

## 限制

- 明文 HTTP；
- token 无过期 / 无刷新；
- 没有注册接口；
- 没有持久化写入；
- 仅绑定 `127.0.0.1`，不要暴露到公网。
