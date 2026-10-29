# 认证 API

> 客户端期望的 HTTP 接口说明。服务端实现有两个：
>
> - **Cloudflare Worker**（[worker/](../worker/README.md)）：真实服务端，PBKDF2 加盐哈希 + KV token，部署后可直接使用；
> - **mock 服务端**（[tools/mock-server/](../tools/mock-server/README.md)）：本地开发用，存储格式与 Worker 相同。

## 连接信息

| 项 | 值 |
| --- | --- |
| 地址 | `http://localhost:8877`（默认值；可在设置窗口或 `application.json` 的 `auth_server` 中修改） |
| 方法 | `POST /api` |
| 请求 / 响应 | `application/json` |
| 认证 | 无 HTTP 头；凭证放在 JSON body 中 |

客户端认证使用 QtNetwork（`QNetworkAccessManager`），支持 `http://` 与 `https://`。Qt 5.15 在 Windows 上通过 OpenSSL 1.1.x 提供 TLS：连接 HTTPS 时，需要把 `libssl-1_1-x64.dll` / `libcrypto-1_1-x64.dll` 放到程序目录，部署方式见 [building.md](building.md)。

## 本地联调

仓库包含一个最小 mock 服务端，实现下面的 `login` / `profile` / `query` / `heartbeat`：

```powershell
.\build\windows-x64\Release\PostLiveMockServer.exe tools\mock-server\users.example.json 8877
```

示例账号 `test` / 密码 `123456`。详见 [tools/mock-server/README.md](../tools/mock-server/README.md)。

## 通用字段

请求：

```json
{
    "account": "客户端账号",
    "token": "登录凭证（登录后由服务端返回）",
    "type": "login | profile | query | heartbeat"
}
```

响应：

```json
{
    "account": "客户端账号",
    "token": "登录凭证",
    "success": true,
    "type": "login",
    "info": "备注信息，可能显示在客户端"
}
```

客户端只检查 `success`，必要时读取 `info`。失败时建议仍返回上述结构，`success` 为 `false`。

## Login

根据 `login` 字段选择密码或 token 验证。

### 密码登录

```json
{
    "account": "user",
    "password": "BD4FEB959D6CCD62DCFD6F9CAC11AEAD",
    "type": "login",
    "login": "password"
}
```

### Token 登录（记住密码 / 自动登录）

```json
{
    "account": "user",
    "token": "4314C0A84B7E97588E7C35E055AC5070",
    "type": "login",
    "login": "token"
}
```

### 成功响应

```json
{
    "account": "user",
    "token": "4314C0A84B7E97588E7C35E055AC5070",
    "success": true,
    "type": "login",
    "info": "login success."
}
```

### 失败响应

```json
{
    "account": "user",
    "success": false,
    "type": "login",
    "info": "password incorrect."
}
```

密码与 token 都是 **MD5 大写十六进制**（历史实现，经 HTTPS 传输）。两个服务端实现都**不以 MD5 形式存储密码**：存储的是 `PBKDF2-SHA256(收到的字符串, 每用户随机盐 + 服务端 pepper)`，见 [worker/README.md](../worker/README.md)。

## Profile

登录成功后，客户端立即请求头像与性别。

请求：

```json
{
    "account": "user",
    "token": "4314C0A84B7E97588E7C35E055AC5070",
    "type": "profile",
    "profile": ["avatar", "gender"]
}
```

响应（客户端当前读取的格式：`profile` 是对象）：

```json
{
    "account": "user",
    "token": "4314C0A84B7E97588E7C35E055AC5070",
    "success": true,
    "type": "profile",
    "info": "",
    "profile": {
        "avatar": "https://example.com/avatar.jpg",
        "gender": "male"
    }
}
```

> 旧文档中的 `profile` 数组形式（`[{...}, {...}]`）与当前客户端代码不一致；实现服务端时请使用上面的对象形式。

## Query（未实现）

> 查询他人信息。客户端尚未发送此请求，保留为协议参考。

```json
{
    "account": "user",
    "token": "4314C0A84B7E97588E7C35E055AC5070",
    "type": "query",
    "query": [
        {
            "account": "zhouh",
            "profile": ["avatar", "gender", "online"]
        },
        {
            "account": "kongjh",
            "profile": ["avatar", "gender"]
        }
    ]
}
```

响应：

```json
{
    "account": "user",
    "success": true,
    "type": "query",
    "info": "",
    "query": [
        {
            "account": "zhouh",
            "profile": {
                "avatar": "https://example.com/avatar.jpg",
                "gender": "female",
                "online": false
            }
        }
    ]
}
```

## Heartbeat（未实现）

> 客户端定时发送以维持在线。当前代码没有实现定时器。Worker / mock 服务端已实现：heartbeat 在 KV 中写一个 15 分钟 TTL 的在线标记，`query` 请求 `online` 字段时读取。

```json
{
    "account": "user",
    "token": "4314C0A84B7E97588E7C35E055AC5070",
    "type": "heartbeat"
}
```

```json
{
    "account": "user",
    "success": true,
    "type": "heartbeat",
    "info": ""
}
```

## Register（仅管理员）

> 客户端不发起注册（注册走网页）。Worker 实现了管理员专用的注册 / 重置密码：请求头带 `x-admin-token: <REGISTER_SECRET>`，body 同 login 的账号格式，`password` 必须是客户端会发送的大写 MD5 十六进制。重复调用同一账号即为重置密码。见 [worker/README.md](../worker/README.md)。

## 示例会话

```text
POST /api  {"type":"login","account":"user","login":"password","password":"<md5>"}
200 OK     {"success":true,"token":"...","info":"login success."}

POST /api  {"type":"login","account":"user","token":"...","login":"token"}
200 OK     {"success":true,"token":"...","info":"login success."}

POST /api  {"type":"profile","account":"user","token":"...","profile":["avatar","gender"]}
200 OK     {"success":true,"profile":{"avatar":"https://example.com/avatar.jpg","gender":"male"}}
```

## 相关文档

- 用户数据文件格式：[database.md](database.md)
- 旧测试服务端的实现思路：[legacy.md](legacy.md)
- 登录流程：[architecture.md](architecture.md)
