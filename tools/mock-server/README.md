# PostLive mock auth server

开发 / 联调用的最小认证服务端，实现 [docs/api.md](../../docs/api.md) 中的 `login`、`profile`、`query`、`heartbeat`。

> 仅用于本地开发。密码存储与真实 Worker（[worker/](../../worker/README.md)）相同算法的 PBKDF2 加盐哈希，但仍是明文 HTTP、无 HTTPS、无数据库、无限流或过期机制。

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
- 密码：`123456`（客户端发送的是其大写 MD5：`E10ADC3949BA59ABBE56E057F20F883E`，token 同值）

客户端保持默认认证地址 `http://localhost:8877`，或通过设置窗口改成 `http://127.0.0.1:<port>`。

## 用户文件格式

```json
{
    "user_list": ["test"],
    "users": {
        "test": {
            "token": "E10ADC3949BA59ABBE56E057F20F883E",
            "password_salt": "301413d52bb246a9781399ad83dc3fce",
            "password_hash": "44b40ed6b751afc756458b3f06bca93f8e960c76cde69a203855fddd2d845bb2",
            "password_iterations": 1000,
            "gender": "male",
            "avatar": "https://example.com/avatar.png"
        }
    }
}
```

- `token` 是 token 登录直接比较的字符串（mock 服务端没有 KV，token 固定写在文件里）。
- `password_salt` / `password_hash` / `password_iterations` 是加盐存储字段；存在时密码登录走 PBKDF2 校验，不存在时回退为旧版 `MD5(password) == token` 比较。

## 生成加盐字段

`--hash` 子命令输出可直接粘贴进用户文件的 JSON 片段（算法与 Worker 相同；迭代次数为本地开发降为 1000，Worker 为 50000）：

```powershell
.\build\windows-x64\Release\PostLiveMockServer.exe --hash E10ADC3949BA59ABBE56E057F20F883E
```

传入的参数是客户端会发送的字符串（真实密码的大写 MD5 十六进制）。

## 限制

- 明文 HTTP；
- token 无过期 / 无刷新、无按登录签发；
- 没有注册接口（新增用户 = 编辑用户文件，用 `--hash` 生成字段）；
- 没有持久化写入；
- 仅绑定 `127.0.0.1`，不要暴露到公网。
