# Security Policy / 安全策略

> **Language / 语言**: [English](#english) | [中文](#中文)

---

## English

### Supported versions

| Version | Supported |
|---|---|
| 1.21.x and current `main` | Yes |
| Older release lines | No |

Report vulnerabilities through the process in the repository root
[`SECURITY.md`](../SECURITY.md). Private vulnerability reporting is enabled.
Do not put exploit details in a public issue.

### What this program is

TuPig Synergy shares a keyboard, mouse, and clipboard across computers you
control. A connected peer can inject input into the other machine. Treat the
machines in one group as mutually trusted. Do not point it at a network you
do not control.

### Network

TLS is **on by default**, and peer checking is **on by default**
(`security/tlsEnabled` and `security/checkPeers`). Peer checking is
fingerprint TOFU: the first accepted certificate is remembered, and a later
change is rejected. This is not a public-CA certificate. RSA keys shorter
than 2048 bits are rejected when peer checking is on.

Turning TLS off switches that connection to plaintext. Plaintext has no
confidentiality and no peer authentication. There is no separate server
password. Settings keys and the real command-line flags are in
[`configuration.md`](configuration.md).

The protocol has a message-length limit. The keyboard and mouse path rate-limits
input. Modifier bits outside the defined mask are cleared. Key-combination
blocking reads `security/blockedKeyCombos`, which is **empty by default**.
Blocking `Ctrl+Alt+Del` on Windows stops UAC and the login screen from seeing
that chord, so it is not blocked unless you list it, and listing it logs a
warning at startup. A malformed entry is skipped.

### Packages

Release builds attach their packages to the GitHub Release `v<X.Y.Z>`. The
publish job signs a [build provenance attestation](https://github.com/actions/attest-build-provenance)
over those exact bytes (`actions/attest-build-provenance` v4.2.2). After you
download a package from a release that ran this step:

```bash
gh attestation verify <file> --repo Tupig/Tupig_synergy
```

That statement says the file came from this repository's workflow and commit.
It is not an Authenticode signature and it is not Apple notarization.

Windows packages are Authenticode-signed only when all four `WINDOWS_SSL_*`
secrets are set. They are not set today, so current Windows packages are
unsigned. macOS signing runs only when the Apple signing secrets are set.
The release published as v1.21.2 was built before provenance was added, so
those assets have no attestation. The next release build will.

The portable Windows archive does not contain `synergy-daemon.exe`. The daemon
is a separate process installed by the MSI. Nothing in this tree registers
the Windows service by itself.

### Other boundaries

- Drag-and-drop file copy is off by default (`fileTransfer/enabled`).
- `synergy-daemon` can start the core on the Windows secure desktop. That is
  required for UAC and the login screen, and it is why the daemon stays its
  own process.
- Code scanning runs from `.github/workflows/codeql.yml` on push, pull
  request, and a weekly schedule.
- Dependabot security updates, secret scanning, and push protection are
  enabled.

### Changelog

| Date | Change |
|---|---|
| 2026-09-18 | Initial security policy |
| 2026-09-23 | Documented configurable key-combination interception |
| 2026-10-09 | Aligned supported versions with `SECURITY.md`. Stated the real TLS default, unsigned Windows packages, and build provenance. Removed the claim that every connection is TLS 1.3 and that the server has a password |

---

## 中文

### 支持的版本

| 版本 | 支持 |
|---|---|
| 1.21.x 与当前 `main` | 是 |
| 更早的发布线 | 否 |

漏洞请按仓库根目录 [`SECURITY.md`](../SECURITY.md) 上报。私密漏洞报告已开启。
不要把利用细节写进公开 issue。

### 这个程序做什么

TuPig Synergy 在你自己控制的几台电脑之间共享键盘、鼠标和剪贴板。连上的对端可以向另一台机器注入输入。同一组里的机器应彼此信任。不要把它指向你不控制的网络。

### 网络

TLS **默认开启**，对端校验也 **默认开启**（`security/tlsEnabled` 与 `security/checkPeers`）。对端校验是指纹 TOFU：第一次接受的证书会被记住，之后证书变了就拒绝。这不是公共 CA 签发的证书。开启对端校验时，短于 2048 位的 RSA 密钥会被拒绝。

关掉 TLS 后，这条连接变为明文。明文没有机密性，也没有对端身份。没有单独的服务器密码。设置键和真实的命令行参数见 [`configuration.md`](configuration.md)。

协议有消息长度上限。键鼠路径对输入做频率限制。修饰键掩码里未定义的位会被清掉。组合键拦截只读 `security/blockedKeyCombos`，该列表 **默认为空**。在 Windows 上拦截 `Ctrl+Alt+Del` 会让 UAC 和登录界面收不到这个组合，所以除非你写进列表，否则不拦截；写进去之后，启动时会记一条警告。格式错误的条目会被跳过。

### 安装包

Release 构建把安装包挂到 GitHub Release `v<X.Y.Z>`。发布作业用 [build provenance](https://github.com/actions/attest-build-provenance)（`actions/attest-build-provenance` v4.2.2）给这些文件的字节签名一份构建来源证明。从跑过这一步的 Release 下载之后：

```bash
gh attestation verify <file> --repo Tupig/Tupig_synergy
```

这份声明说明文件来自本仓库的工作流和提交。它不是 Authenticode 签名，也不是 Apple 公证。

只有四个 `WINDOWS_SSL_*` secret 都配好时，Windows 包才会做 Authenticode 签名。现在没有这些 secret，所以当前 Windows 包未签名。macOS 签名只在配好 Apple 签名 secret 时运行。已经发布的 v1.21.2 是在加入来源证明之前构建的，那些资产没有证明。下一次 Release 构建会带上。

Windows 便携包不含 `synergy-daemon.exe`。daemon 是独立进程，由 MSI 安装。本仓库没有任何代码自行注册 Windows 服务。

### 其他边界

- 拖放传文件默认关闭（`fileTransfer/enabled`）。
- `synergy-daemon` 可以把 core 启动到 Windows 安全桌面。UAC 和登录界面需要这个能力，所以 daemon 保持独立进程。
- `.github/workflows/codeql.yml` 在 push、pull request 和每周定时跑代码扫描。
- Dependabot 安全更新、secret scanning 和 push protection 已开启。

### 更新日志

| 日期 | 改动 |
|---|---|
| 2026-09-18 | 初始安全策略 |
| 2026-09-23 | 记录可配置的键组合拦截 |
| 2026-10-09 | 支持版本与 `SECURITY.md` 对齐。写明 TLS 默认值、未签名的 Windows 包和构建来源证明。去掉“所有连接都是 TLS 1.3”和“服务器有密码”的说法 |
