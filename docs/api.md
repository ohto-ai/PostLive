# 认证 API

> 客户端期望的 HTTP 接口说明。仓库**不包含**可用的服务端实现；没有服务端时登录会失败。

## 连接信息

| 项 | 值 |
| --- | --- |
| 地址 | `http://localhost:8877`（默认值；可在设置窗口或 `application.json` 的 `auth_server` 中修改） |
| 方法 | `POST /api` |
| 请求 / 响应 | `application/json` |
| 认证 | 无 HTTP 头；凭证放在 JSON body 中 |

当前客户端使用 `cpp-httplib`，只支持 `http://`。如果要连接 Cloudflare 等 HTTPS 服务，需要启用 `CPPHTTPLIB_OPENSSL_SUPPORT`，或把认证请求改用 QtNetwork。

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

密码与 token 都是 **MD5 大写十六进制**（历史实现）。这不是安全做法，见 [TODO.md](../TODO.md)。

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

> 客户端定时发送以维持在线。当前代码没有实现定时器。

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
