# Security Policy / 安全策略

## Supported Versions / 支持的版本

| Version / 特本 | Supported / 支持 |
|---|---|
| 1.21.x | ✅ Yes / 是 |

## Reporting a Vulnerability / 报告漏洞

If you discover a security vulnerability within TuPig Synergy, please send an email to the maintainers. All security vulnerabilities will be promptly addressed.

如果您在 TuPig Synergy 中发现安全漏洞，请发送邮件给维护者。所有安全漏洞将及时处理。

**Please do NOT report security vulnerabilities through public GitHub issues.**  
**请不要通过公开的 GitHub Issues 报告安全漏洞。**

## Security Measures / 安全措施

### TLS Encryption / TLS 加密

- All network communication is encrypted using TLS 1.3 via OpenSSL 3.0+
- Certificate verification includes chain validation, expiration check, and key strength verification
- Fingerprint-based TOFU (Trust On First Use) provides additional layer of security

所有网络通信通过 OpenSSL 3.0+ 使用 TLS 1.3 加密。证书验证包括证书链验证、过期检查和密钥强度验证。基于指纹的 TOFU（首次使用信任）提供额外的安全层。

### Input Validation / 输入验证

- Keyboard and mouse events are validated before being processed
- Rate limiting prevents flooding attacks
- Dangerous key combinations can be intercepted

键盘和鼠标事件在处理前会进行验证。频率限制防止洪泛攻击。危险组合键可以被拦截。

### Protocol Security / 协议安全

- Message length limits prevent memory exhaustion attacks
- Protocol parsing uses proper error handling instead of assertions
- Type-safe format specifiers prevent injection

消息长度限制防止内存耗尽攻击。协议解析使用正确的错误处理而非断言。类型安全的格式说明符防止注入。

## Best Practices / 最佳 practices

1. Always use TLS encryption / 始终使用 TLS 加密
2. Keep the software updated / 保持软件更新
3. Use strong passwords for server access / 为服务器访问使用强密码
4. Monitor network traffic for anomalies / 监控网络流量异常

## Changelog / 更新日志

| Date / 日期 | Change / 改动 |
|---|---|
| 2026-09-18 | Initial security policy / 初始安全策略 |
