# Troubleshooting Guide / 故障排查指南

> **Language / 语言**: [English](#english) | [中文](#中文)

---

## English

### Quick Diagnosis

| Symptom | Likely Cause | First Check |
|---------|--------------|-------------|
| Can't connect | Firewall / wrong IP / port | `telnet <server-ip> 24800` |
| Connects but no control | Screen layout / links missing | Verify `links` section |
| Mouse jumps / stuck | Corner size / delay | Adjust `switchCornerSize`, `switchDelay` |
| Clipboard not sync | `clipboardSharing=false` | Enable in `[server]` or options |
| High CPU / lag | Logging debug / no TLS | Set `level=info`, enable TLS |
| Wayland: no cursor | Missing libei / Portal | Install `libei`, `libportal` |
| macOS: no input | Accessibility permission | Grant in System Settings → Privacy |
| Windows: UAC prompt fails | Daemon not elevated | Run as Admin, check `daemon` section |

---

### Connection Issues

#### Cannot Connect to Server

```bash
# 1. Verify server is listening
netstat -an | grep 24800
# or
ss -tlnp | grep 24800

# 2. Test TCP connectivity
telnet <server-ip> 24800
# or
nc -zv <server-ip> 24800

# 3. Check firewall
# Windows:
netsh advfirewall firewall add rule name="TuPig Synergy" dir=in action=allow protocol=TCP localport=24800
# Linux (ufw):
sudo ufw allow 24800/tcp
# Linux (firewalld):
sudo firewall-cmd --permanent --add-port=24800/tcp
sudo firewall-cmd --reload
# macOS:
sudo /usr/libexec/ApplicationFirewall/socketfilterfw --add /path/to/synergy-core
sudo /usr/libexec/ApplicationFirewall/socketfilterfw --unblock /path/to/synergy-core
```

#### Connection Drops Frequently

| Cause | Solution |
|-------|----------|
| Network instability | Increase `heartbeat` (default 5000ms) |
| Server sleep | Disable sleep on server (`preventSleep=true`) |
| Client timeout | Check keep-alive: `CALV` every 3s |
| TLS handshake fail | Verify cert/key, try without TLS |

**Debug**: Enable protocol logging
```ini
[log]
level=debug
toFile=true
file=/path/to/synergy-debug.log
```

#### "Incompatible Version" Error

```
EICV: Protocol version mismatch
```

**Fix**: Both sides must support common version. Server logs show negotiated version. Update both ends to same major version.

---

### Input Issues

#### Mouse Not Moving / Stuck

1. **Check corner settings**:
   ```ini
   [options]
   switchCornerSize = 6
   switchDelay = 250
   ```

2. **Verify links are bidirectional**:
   ```ini
   section: links
       desktop:
           right = laptop
       laptop:
           left = desktop
   end
   ```

3. **Screen resolution mismatch**: Ensure `DINF` reports correct dimensions.

#### Keyboard Not Working

1. **Modifier keys stuck**: Press `Ctrl`/`Alt`/`Shift` on both machines to reset.
2. **Layout mismatch**: Enable `languageSync=true` in `[client]`.
3. **Half-duplex keys**: If CapsLock/NumLock behave oddly:
   ```ini
   section: screens
       laptop:
           halfDuplexCapsLock = true
           halfDuplexNumLock = true
   end
   ```

#### Clipboard Not Syncing

```ini
# Server config
[server]
clipboardSharing = true
clipboardSharingSize = 2048  # KB

# Or in GUI [server] section
clipboardSharing=true
clipboardSharingSize=2048
```

**Requirements**: Both sides v1.6+, TLS enabled for large transfers.

---

### Platform-Specific Issues

#### 🐧 Linux

| Issue | Solution |
|-------|----------|
| **Wayland: no cursor/input** | Install `libei`, `libportal`; ensure `xdg-desktop-portal` running |
| **X11: cursor not confined** | Set `xtestIsXineramaUnaware = true` on client |
| **Clipboard empty** | Install `wl-clipboard` (Wayland) or `xclip`/`xsel` (X11) |
| **High DPI blurry** | Set `QT_AUTO_SCREEN_SCALE_FACTOR=1` or per-monitor DPI |
| **SELinux blocks** | `setsebool -P allow_execstack=1` or custom policy |
| **Systemd service fails** | Check `ExecStart=/usr/bin/synergy-core --no-daemon` |

**Debug Wayland**:
```bash
# Check Portal
busctl --user call org.freedesktop.portal.Desktop /org/freedesktop/portal/desktop org.freedesktop.portal.ScreenCast GetVersion

# Check libei
pkg-config --modversion libei
```

#### 🍎 macOS

| Issue | Solution |
|-------|----------|
| **No input at all** | Grant **Accessibility** + **Input Monitoring** in System Settings → Privacy & Security |
| **Notarization fails** | Codesign with `--options runtime`, notarize via `xcrun notarytool` |
| **Apple Silicon: Rosetta** | Build native `arm64`; avoid Rosetta |
| **Menu bar icon missing** | Grant **Screen Recording** permission (for tray) |
| **Keychain prompts** | Store cert in keychain: `security add-trusted-cert -d -r trustRoot -k /Library/Keychains/System.keychain cert.pem` |

**Debug Permissions**:
```bash
# Check TCC database
sqlite3 ~/Library/Application\ Support/com.apple.TCC/TCC.db "SELECT * FROM access WHERE service='kTCCServiceAccessibility';"

# Reset permissions (last resort)
tccutil reset Accessibility
tccutil reset InputMonitoring
```

#### 🪟 Windows

| Issue | Solution |
|-------|----------|
| **UAC prompts / login screen not captured** | The daemon must be registered as a Windows service — see [Register the daemon service](#register-the-daemon-service-windows) |
| **Service mode fails** | Same; the binaries cannot self-install the service, so it must come from the MSI or a manual `sc create` |
| **Antivirus blocks** | Add exclusion for `synergy-core.exe`, `synergy-daemon.exe` |
| **High DPI scaling** | Set DPI awareness in manifest; per-monitor v2 |
| **Port already in use** | `netstat -ano | findstr :24800`, kill PID |
| **FIPS mode breaks TLS** | Disable FIPS or use OpenSSL FIPS provider |

**Debug Service**:
```powershell
# Check service status
Get-Service -Name "TuPig Synergy"

# View logs
Get-WinEvent -LogName Application -ProviderName "TuPig Synergy" -MaxEvents 50
```

<a id="register-the-daemon-service-windows"></a>
**Register the daemon service (Windows)**

UAC-prompt and login-screen support requires `synergy-daemon.exe` to run as a Windows
service: from session 0 it duplicates the token of `winlogon.exe` / `logonui.exe` to
start the core on the secure desktop. A user-session process cannot do this, so the
capability is unavailable until the service is registered.

The executables **cannot register themselves** — there is no `--install-service`
option and no `CreateService` call anywhere in the sources. Registration therefore
comes from one of:

1. **The MSI (recommended).** It declares `ServiceInstall` for `synergy-daemon.exe`
   with `Start="auto"`, plus `ServiceControl` to start the service on install and
   stop and delete it on uninstall. Installing the MSI is all that is required.
2. **Manual registration**, as Administrator:

```bat
sc create "TuPig Synergy" binPath= "<install dir>\synergy-daemon.exe" start= auto ^
   DisplayName= "TuPig Synergy"
sc description "TuPig Synergy" "Runs the Core process on secure desktops (UAC prompts, login screen, etc)."
sc start "TuPig Synergy"
```

To remove it:

```bat
sc stop "TuPig Synergy"
sc delete "TuPig Synergy"
```

> The service name is not hardcoded in the daemon — the Service Control Manager passes
> it in at startup — so any name pointing at the right binary works. The MSI happens to
> use `TuPig Synergy` (the project's proper name), which is what the commands above use.
>
> The **portable** 7Z package deliberately omits the daemon, so portable builds cannot
> provide UAC/login-screen support at all. Use the MSI for that.

---

### TLS / Security Issues

#### Certificate Errors

| Error | Fix |
|-------|-----|
| `certificate verify failed` | Check `certificate` path, permissions (readable by user) |
| `key too small` | Use `keySize=2048` or `4096` |
| `self-signed` | Disable `checkPeerFingerprints=false` (LAN only) |
| `expired` | Regenerate cert: `openssl req -x509 -newkey rsa:2048 -keyout key.pem -out cert.pem -days 3650 -nodes` |

#### Generate Self-Signed Cert

```bash
# Server cert
openssl req -x509 -newkey rsa:2048 -keyout server-key.pem -out server-cert.pem -days 3650 -nodes -subj "/CN=TuPig Synergy Server"

# Client cert (optional)
openssl req -x509 -newkey rsa:2048 -keyout client-key.pem -out client-cert.pem -days 3650 -nodes -subj "/CN=TuPig Synergy Client"

# Config
[security]
certificate = /path/to/server-cert.pem
keySize = 2048
tlsEnabled = true
checkPeerFingerprints = false  # for self-signed
```

---

### Performance Tuning

| Setting | Recommendation |
|---------|----------------|
| `heartbeat` | 5000ms (LAN), 10000ms (WAN) |
| `switchDelay` | 250ms (default), 100ms (fast) |
| `clipboardSharingSize` | 1024KB (text), 5120KB (images) |
| `log level` | `info` (prod), `debug` (debug only) |
| TLS session reuse | Enabled by default in OpenSSL 3.0 |

---

### Debugging Tools

| Tool | Purpose |
|------|---------|
| `synergy-core --debug` | Verbose protocol logging |
| Wireshark | Capture port 24800, decode as Synergy |
| `strace -p <pid>` | Linux syscall trace |
| `dtruss -p <pid>` | macOS syscall trace |
| Process Monitor | Windows file/reg/net activity |
| `lsof -i :24800` | Check port usage |

---

### Log Analysis

**Key log patterns:**

```
# Successful connection
[INFO] Server: Client "laptop" connected (v1.8)
[INFO] Server: Screen "laptop" entered at (1920,0)

# Keep-alive
[DEBUG] Server: CALV sent to "laptop"
[DEBUG] Client: CALV received, responding

# Errors
[ERROR] Client: TLS handshake failed: certificate verify failed
[WARN] Server: Keep-alive timeout for "laptop", disconnecting
```

---

### Getting Help

| Channel | Best For |
|---------|----------|
| **GitHub Issues** | Bug reports, crashes, reproducible issues |
| **GitHub Discussions** | Configuration help, "how do I..." |
| **Wiki** | Common setups, FAQ |
| **Debug Logs** | Always attach `synergy-debug.log` with issue |

**Minimal bug report template:**
```markdown
**OS/Version**: Windows 11 23H2 / TuPig Synergy 1.21.2
**Role**: Server (Desktop) ↔ Client (Laptop)
**Config**: [paste relevant config sections]
**Logs**: [attach synergy-debug.log]
**Steps**: 1. Start server 2. Start client 3. Move mouse to edge
**Expected**: Cursor moves to client
**Actual**: Cursor stops at edge, log shows [error]
```

---

## 中文

### 快速诊断

| 现象 | 可能原因 | 首要检查 |
|------|----------|----------|
| 无法连接 | 防火墙 / IP 错误 / 端口 | `telnet <server-ip> 24800` |
| 连上但无法控制 | 屏幕布局/链接缺失 | 检查 `links` 节 |
| 鼠标跳跃/卡住 | 角落大小/延迟 | 调整 `switchCornerSize`, `switchDelay` |
| 剪贴板不同步 | `clipboardSharing=false` | 在 `[server]` 或 options 启用 |
| 高 CPU/卡顿 | 调试日志 / 无 TLS | 设 `level=info`、启用 TLS |
| Wayland 无光标 | 缺 libei/Portal | 安装 `libei`, `libportal` |
| macOS 无输入 | 无辅助功能权限 | 系统设置 → 隐私与安全性 → 授权 |
| Windows UAC 失败 | 守护进程未提权 | 以管理员运行，检查 `daemon` 节 |

---

### 连接问题

#### 无法连接到服务端

```bash
# 1. 验证服务端监听
netstat -an | grep 24800
# 或
ss -tlnp | grep 24800

# 2. 测试 TCP 连通性
telnet <server-ip> 24800
# 或
nc -zv <server-ip> 24800

# 3. 检查防火墙
# Windows:
netsh advfirewall firewall add rule name="TuPig Synergy" dir=in action=allow protocol=TCP localport=24800
# Linux (ufw):
sudo ufw allow 24800/tcp
# Linux (firewalld):
sudo firewall-cmd --permanent --add-port=24800/tcp
sudo firewall-cmd --reload
# macOS:
sudo /usr/libexec/ApplicationFirewall/socketfilterfw --add /path/to/synergy-core
sudo /usr/libexec/ApplicationFirewall/socketfilterfw --unblock /path/to/synergy-core
```

#### 频繁断连

| 原因 | 解决方案 |
|------|----------|
| 网络不稳定 | 增加 `heartbeat` (默认 5000ms) |
| 服务端休眠 | 设置 `preventSleep=true` |
| 客户端超时 | 检查保活：`CALV` 每 3 秒 |
| TLS 握手失败 | 验证证书/密钥，尝试关闭 TLS |

**调试**：启用协议日志
```ini
[log]
level=debug
toFile=true
file=/path/to/synergy-debug.log
```

#### "版本不兼容" 错误

```
EICV: Protocol version mismatch
```

**修复**：双端必须支持共同版本。服务端日志显示协商版本。双端升级到同一主版本。

---

### 输入问题

#### 鼠标不动 / 卡住

1. **检查角落设置**：
   ```ini
   [options]
   switchCornerSize = 6
   switchDelay = 250
   ```

2. **验证链接双向**：
   ```ini
   section: links
       desktop:
           right = laptop
       laptop:
           left = desktop
   end
   ```

3. **分辨率不匹配**：确保 `DINF` 上报正确尺寸。

#### 键盘无响应

1. **修饰键卡住**：两台机器分别按 `Ctrl`/`Alt`/`Shift` 重置。
2. **布局不匹配**：客户端启用 `languageSync=true`。
3. **半双工键**：CapsLock/NumLock 行为异常时：
   ```ini
   section: screens
       laptop:
           halfDuplexCapsLock = true
           halfDuplexNumLock = true
   end
   ```

#### 剪贴板不同步

```ini
# 服务端配置
[server]
clipboardSharing = true
clipboardSharingSize = 2048  # KB

# 或 GUI [server] 节
clipboardSharing=true
clipboardSharingSize=2048
```

**要求**：双端 v1.6+，大文件传输需 TLS。

---

### 平台专属问题

#### 🐧 Linux

| 问题 | 解决方案 |
|------|----------|
| **Wayland 无光标/输入** | 安装 `libei`, `libportal`；确保 `xdg-desktop-portal` 运行 |
| **X11 光标不限制** | 客户端设 `xtestIsXineramaUnaware = true` |
| **剪贴板为空** | Wayland 装 `wl-clipboard`，X11 装 `xclip`/`xsel` |
| **高 DPI 模糊** | 设 `QT_AUTO_SCREEN_SCALE_FACTOR=1` 或逐显示器 DPI |
| **SELinux 拦截** | `setsebool -P allow_execstack=1` 或自定义策略 |
| **Systemd 服务失败** | 检查 `ExecStart=/usr/bin/synergy-core --no-daemon` |

**Wayland 调试**：
```bash
# 检查 Portal
busctl --user call org.freedesktop.portal.Desktop /org/freedesktop/portal/desktop org.freedesktop.portal.ScreenCast GetVersion

# 检查 libei
pkg-config --modversion libei
```

#### 🍎 macOS

| 问题 | 解决方案 |
|------|----------|
| **完全无输入** | 系统设置 → 隐私与安全性 → **辅助功能** + **输入监控** 授权 |
| **公证失败** | 签名加 `--options runtime`，`xcrun notarytool` 公证 |
| **Apple Silicon Rosetta** | 原生构建 `arm64`；避免 Rosetta |
| **菜单栏图标缺失** | 授予 **屏幕录制** 权限 (托盘用) |
| **钥匙串弹窗** | 证书存入钥匙串：`security add-trusted-cert -d -r trustRoot -k /Library/Keychains/System.keychain cert.pem` |

**权限调试**：
```bash
# 查看 TCC 数据库
sqlite3 ~/Library/Application\ Support/com.apple.TCC/TCC.db "SELECT * FROM access WHERE service='kTCCServiceAccessibility';"

# 重置权限 (最后手段)
tccutil reset Accessibility
tccutil reset InputMonitoring
```

#### 🪟 Windows

| 问题 | 解决方案 |
|------|----------|
| **UAC 提示 / 登录界面未捕获** | daemon 必须注册为 Windows 服务 —— 见 [注册守护服务](#注册守护服务-windows) |
| **服务模式失败** | 同上；可执行文件无法自行安装服务，必须由 MSI 或手工 `sc create` 完成 |
| **杀毒软件拦截** | 加白 `synergy-core.exe`, `synergy-daemon.exe` |
| **高 DPI 缩放** | 清单设 DPI 感知；逐显示器 v2 |
| **端口被占用** | `netstat -ano | findstr :24800`，结束 PID |
| **FIPS 模式破坏 TLS** | 关闭 FIPS 或用 OpenSSL FIPS provider |

**服务调试**：
```powershell
# 查看服务状态
Get-Service -Name "TuPig Synergy"

# 查看日志
Get-WinEvent -LogName Application -ProviderName "TuPig Synergy" -MaxEvents 50
```

<a id="注册守护服务-windows"></a>
**注册守护服务（Windows）**

UAC 提示与登录界面支持要求 `synergy-daemon.exe` 以 Windows 服务身份运行：它在会话 0 中复制
`winlogon.exe` / `logonui.exe` 的令牌，才能把 core 启动到安全桌面。用户会话中的进程无法做到
这一点，因此在服务注册完成之前，该能力不可用。

可执行文件**无法自行注册服务** —— 代码中不存在 `--install-service` 选项，也没有任何
`CreateService` 调用。因此注册只有两种来源：

1. **MSI（推荐）**：它为 `synergy-daemon.exe` 声明了 `ServiceInstall`（`Start="auto"`），
   并配 `ServiceControl` 在安装时启动服务、卸载时停止并删除服务。装完 MSI 即可。
2. **手工注册**（需管理员权限）：

```bat
sc create "TuPig Synergy" binPath= "<安装目录>\synergy-daemon.exe" start= auto ^
   DisplayName= "TuPig Synergy"
sc description "TuPig Synergy" "Runs the Core process on secure desktops (UAC prompts, login screen, etc)."
sc start "TuPig Synergy"
```

移除服务：

```bat
sc stop "TuPig Synergy"
sc delete "TuPig Synergy"
```

> 服务名在 daemon 中并未硬编码 —— 由服务控制管理器在启动时传入 —— 因此任何指向正确可执行
> 文件的名称都可用。MSI 使用的是 `TuPig Synergy`（项目正式名），上文命令即沿用该名称。
>
> **便携版** 7Z 包有意不含 daemon，因此便携构建完全无法提供 UAC/登录界面支持。该能力请用 MSI。

---

### TLS / 安全问题

#### 证书错误

| 错误 | 修复 |
|------|------|
| `certificate verify failed` | 检查 `certificate` 路径、权限 (用户可读) |
| `key too small` | 用 `keySize=2048` 或 `4096` |
| `self-signed` | 设 `checkPeerFingerprints=false` (仅局域网) |
| `expired` | 重新生成：`openssl req -x509 -newkey rsa:2048 -keyout key.pem -out cert.pem -days 3650 -nodes` |

#### 生成自签名证书

```bash
# 服务端证书
openssl req -x509 -newkey rsa:2048 -keyout server-key.pem -out server-cert.pem -days 3650 -nodes -subj "/CN=TuPig Synergy Server"

# 客户端证书 (可选)
openssl req -x509 -newkey rsa:2048 -keyout client-key.pem -out client-cert.pem -days 3650 -nodes -subj "/CN=TuPig Synergy Client"

# 配置
[security]
certificate = /path/to/server-cert.pem
keySize = 2048
tlsEnabled = true
checkPeerFingerprints = false  # 自签名时
```

---

### 性能调优

| 设置 | 建议 |
|------|------|
| `heartbeat` | 5000ms (局域网), 10000ms (广域网) |
| `switchDelay` | 250ms (默认), 100ms (极速) |
| `clipboardSharingSize` | 1024KB (文本), 5120KB (图片) |
| `log level` | `info` (生产), `debug` (仅调试) |
| TLS 会话复用 | OpenSSL 3.0 默认开启 |

---

### 调试工具

| 工具 | 用途 |
|------|------|
| `synergy-core --debug` | 详细协议日志 |
| Wireshark | 抓包端口 24800，解析为 Synergy |
| `strace -p <pid>` | Linux 系统调用跟踪 |
| `dtruss -p <pid>` | macOS 系统调用跟踪 |
| Process Monitor | Windows 文件/注册表/网络 |
| `lsof -i :24800` | 检查端口占用 |

---

### 日志分析

**关键日志模式：**

```
# 连接成功
[INFO] Server: Client "laptop" connected (v1.8)
[INFO] Server: Screen "laptop" entered at (1920,0)

# 保活
[DEBUG] Server: CALV sent to "laptop"
[DEBUG] Client: CALV received, responding

# 错误
[ERROR] Client: TLS handshake failed: certificate verify failed
[WARN] Server: Keep-alive timeout for "laptop", disconnecting
```

---

### 获取帮助

| 渠道 | 适用场景 |
|------|----------|
| **GitHub Issues** | Bug 报告、崩溃、可复现问题 |
| **GitHub Discussions** | 配置求助、"怎么做..." |
| **Wiki** | 常见布局、FAQ |
| **调试日志** | 提 Issue 必附 `synergy-debug.log` |

**最小 Bug 报告模板：**
```markdown
**OS/版本**: Windows 11 23H2 / TuPig Synergy 1.21.2
**角色**: 服务端(台式机) ↔ 客户端(笔记本)
**配置**: [粘贴相关配置节]
**日志**: [附件 synergy-debug.log]
**步骤**: 1. 启动服务端 2. 启动客户端 3. 移动鼠标到边缘
**预期**: 光标进入客户端
**实际**: 光标停在边缘，日志显示 [error]
```