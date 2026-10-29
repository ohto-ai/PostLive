# Cloudflare 免费方案评估

> 评估日期：2026-10-29。Cloudflare 的免费额度、条款和产品状态会变化，部署前请以官方文档为准。

## 结论

| 问题 | 结论 |
| --- | --- |
| 能否用 Cloudflare 做免费认证 | **可以**。Workers + D1 + KV 足以覆盖小规模学习项目；注意 10 ms CPU/请求、10 万请求/天、PBKDF2 迭代上限。 |
| 能否用 Cloudflare 做免费 RTMP 流服务器 | **没有现成的免费方案**。Workers 不能接收 RTMP；Cloudflare Stream 按分钟计费；Tunnel 公网视频受 Free/Pro/Business 套餐条款限制。 |
| 最接近免费的推流 | **客户端 FFmpeg 输出 HLS → 上传 R2 → Pages/hls.js 播放**。没有 RTMP 低延迟，但免费额度内可行。 |
| 最低成本的托管推流 | **Cloudflare Stream**：ingress/编码免费，存储 $5/1000 分钟/月，分发 $1/1000 分钟；没有真正的免费层。 |
| WebRTC 路线 | **Cloudflare Realtime SFU** 每月 1 TB egress 免费，但 Qt 客户端需要 WebRTC 栈，且 SFU 没有原生 RTMP/WHIP ingest。 |

**推荐路线**：认证先做 Workers + D1；推流保留现有的自建 Nginx-RTMP（见 [rtmp-server.md](rtmp-server.md)），或做一个 R2 + HLS 的实验；如果要求稳定低延迟再考虑 Stream 付费。

## 免费额度（2026-10）

| 产品 | 免费额度 | 说明 |
| --- | --- | --- |
| Workers | 100,000 请求/天；10 ms CPU/请求；128 MB 内存；50 个外部子请求 | 超出返回 1027；CPU 时间是计算时间，不含等待 I/O |
| D1 | 500 万行读/天；10 万行写/天；5 GB 存储 | 2026-09-01 起强制执行免费日限额 |
| KV | 10 万读/天；1,000 写/天；1 GB 存储 | 适合 session / 缓存，不适合频繁写 |
| R2 | 10 GB-month；100 万 Class A；1,000 万 Class B；egress 免费 | 仅 Standard 存储；自定义域名可走 CDN |
| Pages | 500 次构建/月；100 个自定义域名 | 适合注册页、播放器静态站 |
| Durable Objects | 10 万请求/天；5 GB；13,000 GB-s/天；仅 SQLite | 单请求 30 s CPU，可用来跑较重的密码哈希 |
| Realtime SFU | 1,000 GB egress/月免费 | 超出 $0.05/GB；beta；WebRTC |
| Stream | **无免费层** | 存储 $5/1000 分钟/月；分发 $1/1000 分钟；ingress/编码免费 |
| Turnstile | 免费，挑战次数不限 | 适合注册页防机器人 |
| Tunnel | 免费 | 公网 hostname 的视频分发受套餐条款限制 |

## 认证方案

### 架构

```text
Qt 客户端 ──HTTPS──> Cloudflare Worker /api ──> D1 (users/tokens)
                                      ├──> KV (session / rate limit)
                                      └──> Turnstile / Pages 注册页 (可选)
```

- Worker 实现现有 `POST /api` 协议：`login`、`profile`，后续再加 `register`、`query`、`heartbeat`。
- D1 存用户、密码哈希、token 版本；KV 存短期 session、登录限流、头像 URL 缓存。
- 注册页放 Pages，使用 Turnstile；桌面客户端只负责登录，避免在客户端做 CAPTCHA。
- Worker secrets 保存 JWT 密钥、pepper；用 `wrangler secret put` 注入，不进仓库。

### 密码哈希

这是免费方案最大的技术约束：

- Workers Free 只有 **10 ms CPU/请求**；
- Web Crypto 的 PBKDF2 **最多 100,000 次迭代**；
- OWASP 建议的 PBKDF2-HMAC-SHA256 600,000 次在 Workers 中约 40–60 ms，会触发 `Error 1102`。

可行做法：

1. **20,000–80,000 次 PBKDF2 + 服务端 pepper**：能塞进 10 ms，但低于 OWASP 建议，只适合学习项目；
2. **把哈希放进 Durable Object**：DO 单请求 30 s CPU，免费层可用；入口 Worker 只做路由，DO 负责 PBKDF2/Argon2；代价是多一层调用和 DO 请求配额；
3. **升级 Workers Paid（$5/月）**：获得更长 CPU，最省事；
4. **改用外部认证服务**（Supabase Auth、Firebase Auth 等免费层）：如果密码安全是硬要求，比自己在免费 Worker 里降迭代次数更稳。

当前客户端发送的是 **MD5**，必须改成“HTTPS + 服务端 PBKDF2/Argon2”。不能继续用 MD5 作为服务端存储格式。

### Token 与限流

- 登录成功签发 JWT（HMAC-SHA256，Web Crypto 很快），或随机 token 存 KV；
- KV 存 session 时设置 TTL；不要在每次 `heartbeat` 时写 D1；
- 登录失败计数放 KV，超过阈值短时锁定；
- 注册接口加 Turnstile；桌面端登录接口可按 IP / 账号做简单限流。

### 请求量估算

Workers 免费层 100,000 请求/天：

- 如果客户端每 30 秒发一次 heartbeat：2,880 请求/用户/天 → 约 **34 个活跃用户**；
- 如果改成每 5 分钟：288 请求/用户/天 → 约 **347 个活跃用户**；
- 只有登录 / 资料请求（每次登录 2 次）时，几千用户也够用。

所以 **heartbeat 频率决定免费层能撑多少用户**。建议 heartbeat 用 KV 或直接不做，避免 D1 写放大。

### 客户端改造

当前 `src/core/utils.cpp` 的 `createAuthClient()` 会读取 `application.json` 的 `auth_server`（默认 `http://localhost:8877`），用 `cpp-httplib` 发起同步请求。

问题是：

- 当前构建只支持明文 HTTP，Cloudflare 需要 HTTPS；
- 同步请求在 UI 线程执行，已加 3 s / 5 s 超时，但网络慢时仍会短暂卡登录窗口。

两种改法：

1. **改用 QtNetwork（推荐）**：项目已经链接 `Qt5::Network`，`QNetworkAccessManager` 在 Windows 上走 Schannel/TLS，不需要额外 OpenSSL；把登录流程改成 signal/slot 异步。
2. **给 cpp-httplib 开启 OpenSSL**：定义 `CPPHTTPLIB_OPENSSL_SUPPORT` 并链接 OpenSSL；构建和分发都更重。

同时需要：

- API 地址可配置（当前硬编码 `localhost:8877`）；
- token 不再明文写 `users.json`，改用 Windows Credential Manager / DPAPI 或加密存储；
- 注册 / 找回密码目前只是打开网页，Cloudflare 方案下可以指向 Pages 页面。

### 部署

```bash
npm create cloudflare@latest postlive-auth
cd postlive-auth
npx wrangler d1 create postlive
npx wrangler kv namespace create SESSIONS
npx wrangler secret put JWT_SECRET
npx wrangler deploy
```

- `workers.dev` 子域名自带 HTTPS，免费；
- 自定义域名可选（需要自己的域名）；
- CI 可用 `cloudflare/wrangler-action`，把 `CLOUDFLARE_API_TOKEN` 放在 GitHub Secrets；
- 本地联调用 `wrangler dev`，D1/KV 用本地模拟。

## 推流方案对比

| 方案 | 免费？ | 协议 / 延迟 | 客户端改动 | 主要限制 |
| --- | --- | --- | --- | --- |
| Cloudflare Stream | **否**，按分钟计费 | RTMP/SRT/WHIP 推流；HLS/WebRTC 播放，延迟低 | 小：替换推流 URL / 播放 URL | 存储 $5/1000 分钟/月，分发 $1/1000 分钟；无免费层 |
| R2 + HLS | **是**（免费额度内） | 客户端 FFmpeg 出 HLS；HTTP 分段，延迟约 6–30 s | 中：改 FFmpeg 输出 + 上传器 + 播放页 | 没有 RTMP ingest；需要管理 playlist/分段/清理；上传占上行带宽 |
| Realtime SFU | 1 TB egress/月免费 | WebRTC，低延迟 | 大：Qt 需要 WebRTC 栈 / WHIP 网关 | 没有原生 RTMP/WHIP ingest；beta；客户端架构重写 |
| Tunnel + 自建 RTMP | 软件免费 | RTMP via Tunnel | 小（服务端） | 公网 hostname 视频分发违反 Free/Pro/Business 条款；private route 需要 WARP；源站需 24/7 |
| 自建 VPS + Nginx-RTMP | VPS 可能免费/低价 | RTMP / HLS | 小 | 需要服务器运维；Cloudflare 只做 DNS 或 HLS 分发 |

### Cloudflare Stream

- 支持 RTMP/RTMPS/SRT/WHIP 推流，HLS/WebRTC 播放；
- ingress 和编码免费，费用只在存储和分发；
- 没有真正的免费层；Starter 套餐约 $5/月（含 1,000 分钟存储 + 5,000 分钟分发）；
- 粗略成本：1 小时直播 + 10 个观众 ≈ $0.30 存储 + $0.60 分发 ≈ **$0.90**；100 个观众 ≈ **$6.30**；
- 集成成本最低：客户端把 `rtmp://.../livehime` 换成 Stream 的 RTMP 地址即可，播放用 Stream 的 HLS/WebRTC URL。

### R2 + HLS

流程：

```text
ffmpeg -f dshow ... -f hls -hls_time 3 -hls_list_size 6 \
       -hls_segment_filename "out/seg_%05d.ts" out/index.m3u8
        │
        └── 上传器（Qt / rclone / Worker 预签名 URL）──> R2 bucket
                                                       │
Pages + hls.js <── 自定义域名 + CORS + Cache Everything ┘
```

要点：

- R2 免费层 10 GB、100 万写、1,000 万读、egress 免费；低并发学习项目足够；
- 生产必须用自定义域名（`r2.dev` 仅用于非生产、有限速）；
- 浏览器播放需要给 bucket 配 CORS；`.m3u8` / `.ts` 建议用 Cache Everything 规则缓存；
- playlist 要原子更新（先传分片，再传 `.m3u8`），并定期清理旧分片；
- 延迟取决于 `hls_time`，通常 6–30 s；没有 RTMP 的低延迟；
- 上传鉴权可以用 Worker 签发预签名 URL，或客户端带 JWT 调 Worker 上传。

### Realtime SFU

- 每月 1 TB egress 免费，超出 $0.05/GB；对 720p/2 Mbps 大约能覆盖 1,000+ 观众小时/月；
- 但它是 WebRTC SFU，不是 RTMP 服务器：通过 `/sessions`、`/tracks` API 建立 publish/subscribe；
- 没有原生 RTMP 或 WHIP ingest；WHIP 需要自己做网关（Cloudflare 有 demo），或者用 Stream 的 WHIP；
- Qt 5 没有内置 WebRTC；需要 libwebrtc / GStreamer / FFmpeg WHIP + 会话 API，工作量大；
- 适合“以后重写为 WebRTC 客户端”的路线，不适合当前 Qt/FFmpeg 架构的第一步。

### Tunnel + 自建 RTMP

- `cloudflared` 免费，但 **公网 hostname** 会把流量代理进 Cloudflare，受 Free/Pro/Business 的“不能分发视频”条款约束；
- **private network route** 豁免该条款，但每个客户端都要装 WARP/connector，不适合公开观众；
- 源站必须 24/7 在线；Cloudflare 只是隧道，不提供转码、录制、HLS。

## 对 PostLive 的建议

1. **先做认证**：Workers + D1 + KV，客户端改用 QtNetwork HTTPS；这是投入产出比最高的部分。
2. **推流先不动**：保留现有 Nginx-RTMP 部署，客户端继续 RTMP；避免为了“免费”重写整个推流链路。
3. **做 R2 + HLS 实验**：如果目标是“零服务器、低并发、能看就行”，这是 Cloudflare 免费额度内最现实的方案。
4. **需要稳定低延迟再付费**：Cloudflare Stream 的集成成本最低，按分钟计费，学习项目成本通常不到一杯咖啡/月。
5. **不要用 Tunnel 公网 hostname 做公开视频分发**：有条款风险，可能被限速/封禁。

## 我执行的工作量估算

| 任务 | 预计 |
| --- | --- |
| Workers + D1 + KV 认证 API（login/profile/register、JWT、限流） | 2–3 小时 |
| 客户端 QtNetwork HTTPS + 异步登录 | 1–2 小时 |
| R2 + HLS PoC（FFmpeg 输出、上传器、Pages 播放器） | 3–5 小时 |
| Cloudflare Stream 接入（付费账号） | 1–2 小时 |
| 部署文档 / Wrangler / CI | 1–2 小时 |

不含 Cloudflare 账号注册、域名购买、真实设备/网络验证。认证部分可以完全由我在本地用 `wrangler dev` 验证；推流部分需要真实 bucket/账号才能端到端验证。

## 风险清单

- **10 ms CPU**：PBKDF2 迭代次数受限；要么降档 + pepper，要么用 DO，要么升级 Paid。
- **10 万请求/天**：heartbeat 频率直接决定能支撑的在线用户数。
- **D1 10 万写/天**：不要每次心跳写库。
- **免费套餐视频条款**：Cloudflare 明确禁止 Free/Pro/Business 通过其网络分发视频；Tunnel 公网 hostname 也适用。
- **没有出站邮件**：Cloudflare Email Routing 主要是收件/转发，密码重置需要第三方邮件 API 或网页流程。
- **workers.dev 可用性**：部分地区/网络可能不稳定；自定义域名更稳。
- **产品状态变化**：Realtime 仍是 beta；Stream 的 WebRTC 计费从 2026-10-15 开始；免费额度可能调整。

## 参考资料

- [Workers limits](https://developers.cloudflare.com/workers/platform/limits/)
- [Workers pricing](https://developers.cloudflare.com/workers/platform/pricing/)
- [D1 pricing](https://developers.cloudflare.com/d1/platform/pricing/)
- [KV limits](https://developers.cloudflare.com/kv/platform/limits/)
- [R2 pricing](https://developers.cloudflare.com/r2/pricing/)
- [R2 public buckets](https://developers.cloudflare.com/r2/buckets/public-buckets/)
- [Stream pricing](https://developers.cloudflare.com/stream/pricing/)
- [Realtime SFU pricing](https://developers.cloudflare.com/realtime/sfu/pricing/)
- [Durable Objects pricing](https://developers.cloudflare.com/durable-objects/platform/pricing/)
- [Durable Objects limits](https://developers.cloudflare.com/durable-objects/platform/limits/)
- [Turnstile plans](https://developers.cloudflare.com/turnstile/plans/)
- [Delivering videos with Cloudflare](https://developers.cloudflare.com/fundamentals/reference/policies-compliances/delivering-videos-with-cloudflare/)
- [Pages limits](https://developers.cloudflare.com/pages/platform/limits/)
