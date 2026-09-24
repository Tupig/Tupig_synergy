# Security Policy / 安全策略

> **Language / 语言**: [English](#english) | [中文](#中文)

---

## English

### Supported Versions

| Version | Supported |
|---|---|
| 1.21.x | ✅ Yes |

### Reporting a Vulnerability

Please use **GitHub private vulnerability reporting** for this repository
(GitHub → Security → Report a vulnerability). If that is unavailable, open a
minimal issue asking for a private channel and do not include exploit details
in the public issue. All security vulnerabilities will be promptly addressed.

**Please do NOT report security vulnerabilities through public GitHub issues.**

### Security Measures

#### TLS Encryption

- All network communication is encrypted using TLS 1.3 via OpenSSL 3.0+
- Certificate verification includes chain validation, expiration check, and key strength verification
- Fingerprint-based TOFU (Trust On First Use) provides an additional layer of security

#### Input Validation

- Keyboard and mouse events are validated before being processed
- Rate limiting prevents flooding attacks
- Modifier masks are checked against the defined bit set, and undefined bits are cleared
- Key combinations can be intercepted, but only those listed in
  `security/blockedKeyCombos` — the list is **empty by default**

> Key and button *ranges* are deliberately not re-checked: the wire format already
> bounds them, and several legal values look "out of range" (key id 0, X11 scroll
> buttons 254/255), so a hand-written range check would drop real input.
>
> Combination interception is opt-in because the interesting combinations are the
> ones the OS needs to see. Blocking `Ctrl+Alt+Del` on Windows stops UAC prompts
> and the login screen from responding, so it is never blocked unless you ask for
> it; doing so logs a warning at startup. Entries are written as `key[:mask]`
> (`0xEFFF:0x0005` is Ctrl+Alt+Delete); a malformed entry is skipped with a
> warning rather than failing open into blocking something unintended.

#### Protocol Security

- Message length limits prevent memory exhaustion attacks
- Protocol parsing uses proper error handling instead of assertions
- Type-safe format specifiers prevent injection

### Best Practices

1. Always use TLS encryption
2. Keep the software updated
3. Use strong passwords for server access
4. Monitor network traffic for anomalies

### Changelog

| Date | Change |
|---|---|
| 2026-09-18 | Initial security policy |
| 2026-09-23 | Documented configurable key-combination interception, and corrected the claim that it was always on |

---

## 中文

### 支持的版本

| 版本 | 支持 |
|---|---|
| 1.21.x | ✅ 是 |

### 报告漏洞

请使用本仓库的 **GitHub 私密漏洞报告**（GitHub → Security → Report a vulnerability）。
若该入口不可用，请先开一个**不含漏洞细节**的公开 issue 请求私密沟通渠道。
所有安全漏洞将及时处理。

**请不要通过公开的 GitHub Issues 报告安全漏洞。**

### 安全措施

#### TLS 加密

- 所有网络通信通过 OpenSSL 3.0+ 使用 TLS 1.3 加密
- 证书验证包括证书链验证、过期检查和密钥强度验证
- 基于指纹的 TOFU（首次使用信任）提供额外的安全层

#### 输入验证

- 键盘和鼠标事件在处理前会进行验证
- 频率限制防止洪泛攻击
- 修饰键掩码会对照已定义的位集合校验，未定义的位会被清除
- 键组合可被拦截，但**仅限** `security/blockedKeyCombos` 中列出的项 —— 该列表**默认为空**

> 键码与按键的**范围****不**做重复校验：线路格式本身已限定其范围，且若干合法值看起来像"越界"（键码 0、X11 滚轮键 254/255），手写范围检查反而会丢弃真实输入。
>
> 组合键拦截采用**显式启用**的设计，因为值得拦截的组合恰恰是操作系统需要看到的那些。在 Windows 上拦截 `Ctrl+Alt+Del` 会导致 UAC 提示与登录界面无响应，因此除非你主动要求，否则永不拦截；一旦配置，启动时会记录警告。条目格式为 `key[:mask]`（`0xEFFF:0x0005` 即 Ctrl+Alt+Delete）；格式错误的条目会被跳过并告警，而不会"失败开放"成拦截了意料之外的组合。

#### 协议安全

- 消息长度限制防止内存耗尽攻击
- 协议解析使用正确的错误处理而非断言
- 类型安全的格式说明符防止注入

### 最佳实践

1. 始终使用 TLS 加密
2. 保持软件更新
3. 为服务器访问使用强密码
4. 监控网络流量异常

### 更新日志

| 日期 | 改动 |
|---|---|
| 2026-09-18 | 初始安全策略 |
| 2026-09-23 | 记录可配置的键组合拦截，并更正"始终开启"的失实描述 |
