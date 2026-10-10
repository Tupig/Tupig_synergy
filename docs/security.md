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
change is rejected. This is not a public-CA certificate. A TLS connection
always requires a peer certificate. RSA keys shorter than 2048 bits are
rejected. Turning `security/checkPeers` off does not accept a client that has
no certificate.

Turning TLS off switches that connection to plaintext. Plaintext has no
confidentiality and no peer authentication. There is no separate server
password. Settings keys and the real command-line flags are in
[`configuration.md`](configuration.md).

The protocol has a message-length limit. The keyboard and mouse path rate-limits
input. Modifier bits outside the defined mask are cleared. Key-combination
blocking reads `security/blockedKeyCombos`. The default entry is
`0xEFFF:0x0006` (Ctrl+Alt+Delete), so a remote peer cannot raise the secure
attention sequence until that entry is removed. The default does not log a
warning. A warning is logged only when that chord is stored in the setting
again. A malformed entry is skipped. The local keyboard
is not affected.

The daemon and core IPC pipes are reachable by the interactive user because the
service runs as SYSTEM. A command is accepted only after `hello` carries the token
from the file written beside the system settings. Each file is readable by SYSTEM,
Administrators, and the active console user. The service refreshes both files
once a second, because only SYSTEM can see a console session that appears
after the file was created. Core refreshes its own file as well. The version string is not a credential.
Screen enter and exit commands run only an existing file under the settings
directory. Service mode copies the current settings into that directory and
sends the relative file name.

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

Windows packages are Authenticode-signed by SignPath Foundation only when
`SIGNPATH_API_TOKEN`, `SIGNPATH_ORGANIZATION_ID`, and `SIGNPATH_PROJECT_SLUG`
are set. The private key stays on SignPath's HSM. They are not set today, so
current Windows packages are unsigned. A local Release build cannot use that
key, so it stays unsigned. When CI signing is enabled, `signtool verify` must
succeed or that job stops. macOS
signing runs only when the Apple signing secrets are set. The Apple
Developer ID certificate is the base64 P12 in `APPLE_P12_CERTIFICATE`, imported
into a keychain that exists only for that job. See [build.md](build.md).
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

TLS **默认开启**，对端校验也 **默认开启**（`security/tlsEnabled` 与 `security/checkPeers`）。对端校验是指纹 TOFU：第一次接受的证书会被记住，之后证书变了就拒绝。这不是公共 CA 签发的证书。TLS 连接始终要求对端证书。短于 2048 位的 RSA 密钥会被拒绝。关掉 `security/checkPeers` 也不会接受没有证书的客户端。

关掉 TLS 后，这条连接变为明文。明文没有机密性，也没有对端身份。没有单独的服务器密码。设置键和真实的命令行参数见 [`configuration.md`](configuration.md)。

协议有消息长度上限。键鼠路径对输入做频率限制。修饰键掩码里未定义的位会被清掉。组合键拦截读取 `security/blockedKeyCombos`。默认条目是 `0xEFFF:0x0006`（Ctrl+Alt+Delete），对端在删掉这一条之前不能触发安全注意序列。默认值不记警告。只有这个组合再次写入设置时才记警告。本机键盘不受影响。格式错误的条目会被跳过。

守护进程和 core 的 IPC 管道对交互用户可达，因为服务以 SYSTEM 运行。只有 `hello` 带上系统设置目录旁对应令牌文件里的内容，命令才会被接受。每个文件仅 SYSTEM、Administrators 和当前控制台用户可读。服务每秒刷新这两个文件，因为只有 SYSTEM 能看到文件创建之后才出现的控制台会话。core 也会刷新自己的文件。版本号不是凭据。屏幕进入和离开命令只运行设置目录里已经存在的文件。服务模式会把当前设置复制到该目录，再只发送相对文件名。

### 安装包

Release 构建把安装包挂到 GitHub Release `v<X.Y.Z>`。发布作业用 [build provenance](https://github.com/actions/attest-build-provenance)（`actions/attest-build-provenance` v4.2.2）给这些文件的字节签名一份构建来源证明。从跑过这一步的 Release 下载之后：

```bash
gh attestation verify <file> --repo Tupig/Tupig_synergy
```

这份声明说明文件来自本仓库的工作流和提交。它不是 Authenticode 签名，也不是 Apple 公证。

只有配好 `SIGNPATH_API_TOKEN`、`SIGNPATH_ORGANIZATION_ID` 和 `SIGNPATH_PROJECT_SLUG` 时，Windows 包才会由 SignPath Foundation 做 Authenticode 签名。私钥留在 SignPath 的 HSM 上。现在没有这些值，所以当前 Windows 包未签名。本地 Release 构建拿不到这把私钥，因此不签名。CI 启用签名后，`signtool verify` 不通过则该作业停止。macOS 签名只在配好 Apple 签名 secret 时运行。Developer ID 证书是 `APPLE_P12_CERTIFICATE` 里的 base64 P12，只导入当次作业的临时钥匙串。见 [build.md](build.md)。已经发布的 v1.21.2 是在加入来源证明之前构建的，那些资产没有证明。下一次 Release 构建会带上。

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
