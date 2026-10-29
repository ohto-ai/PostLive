# PostLive 认证 Worker（Cloudflare Workers + D1 + KV）

真实的认证服务端实现，接口与 [docs/api.md](../docs/api.md) 完全一致（`login` / `profile` / `query` / `heartbeat`，另加管理员专用的 `register`）。方案与免费额度评估见 [docs/cloudflare.md](../docs/cloudflare.md)。

- **D1**（`DB`）：用户表，密码以 PBKDF2-SHA256（每用户随机盐 + 服务端 pepper）存储，表结构见 [schema.sql](schema.sql)；
- **KV**（`KV`）：登录 token（30 天 TTL）、登录失败计数（10 次 / 15 分钟锁定）、heartbeat 在线标记（15 分钟 TTL）；
- **Secrets**：`PEPPER`（混入每次 PBKDF2 的盐）、`REGISTER_SECRET`（注册接口的管理员令牌），用 `wrangler secret put` 注入，不进仓库。

客户端不需要改代码：它发送的仍是大写 MD5 十六进制（经 HTTPS 传输），Worker 把这个字符串当作“密码等价物”，绝不明文存储。

## 部署

前置：Node 18+，一个 Cloudflare 账号。

```bash
cd worker

# 1. 创建资源（输出的 id 填进 wrangler.toml）
npx wrangler login
npx wrangler d1 create postlive              # database_id
npx wrangler kv namespace create SESSIONS    # id

# 2. 建表 + 注入密钥
npx wrangler d1 execute postlive --remote --file=schema.sql
npx wrangler secret put PEPPER           # 随机长字符串，例如 openssl rand -hex 32
npx wrangler secret put REGISTER_SECRET  # 注册接口用的管理员令牌

# 3. 部署
npx wrangler deploy
```

部署后地址为 `https://postlive-auth.<你的子域>.workers.dev`，自带 HTTPS。在客户端设置窗口把认证服务器改成该地址即可登录。

`wrangler.toml` 中的 `REPLACE_WITH_*` 占位符未替换时，`wrangler deploy` 会失败——这是有意的，避免把测试资源意外部署上去。

## 注册 / 重置密码

客户端不做注册（注册页走 Web）。管理员用 `REGISTER_SECRET` 调用同一 `/api` 端点：

```bash
curl -X POST "https://postlive-auth.<子域>.workers.dev/api" \
  -H "x-admin-token: <REGISTER_SECRET>" \
  -H "content-type: application/json" \
  -d '{"type":"register","account":"test","password":"E10ADC3949BA59ABBE56E057F20F883E","gender":"male","avatar":"https://example.com/avatar.png"}'
```

注意 `password` 必须是**客户端会发送的字符串**，即真实密码的大写 MD5 十六进制（`echo -n 123456 | md5sum` → `E10ADC...`）。重复调用同一账号即为重置密码（可顺带更新 `gender` / `avatar`）。

## 本地开发与测试

```bash
node --test            # 单元测试：stub 掉 D1/KV，覆盖登录、token、锁定、注册等 11 个用例
npx wrangler dev       # 本地起服务（D1/KV 本地模拟），客户端指向 http://127.0.0.1:8787
```

本地建用户：`npx wrangler d1 execute postlive --local --file=schema.sql` 后，对 `http://127.0.0.1:8787/api` 调用上面的 register。

CI 在独立的 Linux job 里跑 `node --test`。

## 免费额度注意事项（详见 docs/cloudflare.md）

- PBKDF2 迭代次数固定 50 000（`PBKDF2_ITERATIONS` 常量）：免费版 10 ms CPU 上限内的折衷，配合 pepper 弥补；升级 Workers Paid 后可调高并重新注册用户；
- KV 写配额 1 000 次/天：heartbeat 每 15 分钟最多写一次，失败计数只在登录失败时写；在线用户数受此限制；
- 登录失败 10 次锁 15 分钟，按账号计。

## 安全说明与剩余风险

- 密码等价物（MD5 hex）只经 HTTPS 传输；服务端存储为 PBKDF2-SHA256 + 盐 + pepper，数据库泄露不等于密码泄露；
- 客户端 MD5 是历史行为：MD5 抗碰撞性已破，但在这里它只是“传输中的口令等价物”，安全依赖 TLS 与服务端慢哈希；
- 无数字签名 / 证书绑定；token 一旦泄露在 TTL 内有效（KV 手动清理 `token:<token>` 可吊销）；
- 密码重置 = 管理员重新 register，没有邮件流程。
