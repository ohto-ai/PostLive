# Security policy

PostLive 是学习项目，**不适合直接用于生产环境**。已知问题包括：

- 密码与 token 使用 MD5；
- token 明文保存在 `users.json`；
- 认证 API 默认使用明文 HTTP，服务端实现不在本仓库；
- 推流地址与认证地址硬编码。

## 报告问题

如果你发现安全问题，请不要公开提交 Issue。可以通过 GitHub 的 “Report a vulnerability” / Security Advisory 私下联系维护者：

<https://github.com/ohto-ai/PostLive/security/advisories/new>

请说明影响范围、复现步骤和建议修复方式。不要在报告中包含真实的账号、token 或推流地址。

## 支持范围

项目只维护默认分支 `master`。旧版本（`v1.0.0.1` / `v1.0.0.2`）不再接收安全修复。
