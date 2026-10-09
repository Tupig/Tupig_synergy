# Configuration Reference / 配置参考

> **Language / 语言**: [English](#english) | [中文](#中文)

---

## English

### Configuration File Locations

The settings file is named after the application (`kAppName`). In a normal build
that name is `TuPig Synergy`, so the file is `TuPig Synergy.conf`, not
`Synergy.conf`. The directory uses the same name.

On Windows, `<install-dir>/settings/TuPig Synergy.conf` is used when that
portable file exists. Otherwise the first existing file below is used, and if
neither exists the user file is created:

1. `%APPDATA%\TuPig Synergy\TuPig Synergy.conf`
2. `<system-drive>\ProgramData\TuPig Synergy\TuPig Synergy.conf`

`QSettingsProxy` reads the registry key `HKCU\Software\TuPig Synergy\TuPig Synergy`
only when no file path is supplied at all.

On Linux and macOS, a set `XDG_CONFIG_HOME` wins even when the file does not
exist yet: `$XDG_CONFIG_HOME/TuPig Synergy/TuPig Synergy.conf`. When that
variable is unset, the first existing path below is used, or the user path is
created.

#### Linux
1. `~/.config/TuPig Synergy/TuPig Synergy.conf`
2. `/etc/TuPig Synergy/TuPig Synergy.conf`

#### macOS
1. `~/Library/TuPig Synergy/TuPig Synergy.conf`
2. `/Library/TuPig Synergy/TuPig Synergy.conf`

> Certificates and logs are placed next to the settings file that was chosen.

`gui/windowGeometry` is not stored in that file: it lives in a separate state
file, `<state-dir>/TuPig Synergy.state` (see `Settings::m_stateKeys`).

---

### File Format

INI-style with sections and key-value pairs:

```ini
[section]
key=value
```

Comments start with `#` or `;`. Only non-default values are written.

---

### GUI Configuration Sections

#### `[client]` — Client Mode Options

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `dynamicConnectionInterval` | bool | `false` | Exponential backoff on reconnect |
| `languageSync` | bool | `true` | Sync keyboard layout from server |
| `remoteHost` | string | — | Comma-separated host list |
| `yScrollScale` | float | `1.0` | Vertical scroll multiplier (0.1–10.0) |
| `xScrollScale` | float | `1.0` | Horizontal scroll multiplier (0.1–10.0) |
| `invertYScroll` | bool | `false` | Invert vertical scroll |
| `invertXScroll` | bool | `false` | Invert horizontal scroll |
| `xdpRestoreToken` | UUID | — | XDG Portal restore token |

#### `[core]` — General Options

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `coreMode` | int | `0` | `0`=None, `1`=Client, `2`=Server |
| `display` | int | auto | X11 display number (Linux) |
| `interface` | IP | auto | Bind address (`0.0.0.0` = all) |
| `lastVersion` | string | — | Last run version |
| `port` | int | `24800` | TCP port |
| `preventSleep` | bool | `false` | Inhibit system sleep |
| `processMode` | int | `0` on an installed Windows build, `1` otherwise | `0` = Service, `1` = Desktop |
| `computerName` | string | hostname | Unique computer identifier |
| `useHooks` | bool | `true` | Windows: use hooks vs raw input |
| `language` | ISO 639 | `en` | UI language code |
| `enableEnterCommand` | bool | `false` | Run command on screen enter |
| `enterCommand` | string | — | Command to run on enter |
| `enableExitCommand` | bool | `false` | Run command on screen leave |
| `exitCommand` | string | — | Command to run on leave |

#### `[daemon]` — Windows Daemon

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `configFile` | path | — | Core config the daemon starts the core with |
| `elevate` | bool | `true` when not portable | Run elevated (UAC). Portable mode defaults to `false` |
| `logFile` | path | — | Daemon log file |
| `logLevel` | string | — | Log verbosity |

#### `[gui]` — GUI Behavior

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `autoHide` | bool | `false` | Hide window on startup |
| `closeReminder` | bool | `true` | Show "runs in background" reminder |
| `closeToTray` | bool | `true` | Minimize to tray on close |
| `ignoreMissingKeyboardLayouts` | bool | `false` | Suppress missing-layout warnings |
| `logExpanded` | bool | `true` | Expand log panel by default |
| `showVersionInTitle` | bool | `false` | Show version in window title |
| `shownFirstConnectedMessage` | bool | `false` | First-connect hint shown |
| `shownServerFirstStartMessage` | bool | `false` | Server-started hint shown |
| `startCoreWithGui` | bool | `false` | Auto-start core with GUI |
| `symbolicTrayIcon` | bool | `true` | Monochrome tray icon |
| `windowGeometry` | QRect | — | Saved window position/size (state file) |

#### `[log]` — Logging

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `file` | path | — | Log file path |
| `level` | enum | `INFO` | `FATAL`, `ERROR`, `WARNING`, `INFO`, `DEBUG`, `VERBOSE` |
| `toFile` | bool | `false` | Write to file |
| `guiDebug` | bool | `false` | Include GUI internal debug |

#### `[security]` — TLS/Security

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `checkPeerFingerprints` | bool | `true` | Verify peer cert fingerprints |
| `certificate` | path | — | TLS certificate file |
| `keySize` | int | `2048` | `2048` or `4096` |
| `tlsEnabled` | bool | `true` | Enable TLS encryption |
| `blockedKeyCombos` | list | empty | Key combinations to intercept, as `key[:mask]` (e.g. `0xEFFF:0x0005`). Empty blocks nothing |

#### `[fileTransfer]` — Drag-and-drop file transfer

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `enabled` | bool | `false` | Accept files pushed from the server |
| `dropDirectory` | path | — | Where accepted files are written; empty refuses every transfer |
| `maxFileSizeMb` | int | `64` | Largest single file accepted |
| `maxFileCount` | int | `32` | Largest number of files accepted in one drag |

> **Off by default.** Enabling this lets the server write files onto this machine,
> so it is opt-in. When on, every name is reduced to a single safe file name —
> traversal (`../`), absolute paths, drive letters, control characters, Windows
> reserved device names and over-long names are refused rather than adjusted, and
> the content is discarded rather than written under a substitute name. Writes are
> staged and renamed, and an existing file is never overwritten (a `name (1).ext`
> variant is used instead). Declared sizes, the file count and the running total
> are all bounded.

#### `[server]` — Server Mode

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `clipboardSize` | int | `3` | Max clipboard transfer size (MiB) |
| `externalConfig` | bool | `false` | Use external server config file |
| `externalConfigFile` | path | — | Path to server config |
| `protocol` | enum | `synergy` | `synergy` or `barrier` |
| `xdpRestoreToken` | UUID | — | XDG Portal restore token |

#### `[internalConfig]` — Server Layout (GUI-generated)

Complex nested structure for screen layout. See **Server Configuration** below.

---

### Server Configuration (Text Format)

Used by `synergy-core -c config.conf`. Generated from GUI's `[internalConfig]`.

#### Syntax

```ini
section: name
    arg = value
end
```

Sections: `screens`, `aliases`, `links`, `options`

---

#### `screens` — Computer Definitions

```ini
section: screens
    my-desktop:
        halfDuplexCapsLock = false
        halfDuplexNumLock = false
    laptop:
        meta = alt
    workstation:
end
```

**Screen Options:**

| Option | Type | Description |
|--------|------|-------------|
| `halfDuplexCapsLock` | bool | CapsLock doesn't send press+release |
| `halfDuplexNumLock` | bool | NumLock doesn't send press+release |
| `halfDuplexScrollLock` | bool | ScrollLock doesn't send press+release |
| `xtestIsXineramaUnaware` | bool | XTest+Xinerama workaround |
| `preserveFocus` | bool | Don't drop focus on switch |
| `switchCorners` | corners | Disable switching in corners |
| `switchCornerSize` | int | Corner size in pixels |
| `shift` / `ctrl` / `alt` / `meta` / `super` | modifier | Modifier remapping |

---

#### `aliases` — Hostname Aliases

```ini
section: aliases
    my-desktop:
        desktop.local
        desktop.office.lan
    laptop:
        laptop.home
end
```

Clients can connect using canonical name or any alias.

---

#### `links` — Screen Topology

```ini
section: links
    my-desktop:
        right = laptop
        up(50,100) = workstation(0,50)
    laptop:
        left = my-desktop
    workstation:
        down = my-desktop
end
```

**Link Syntax:**
```
{direction}[(start,end)] = target[(start,end)]
```

- Directions: `left`, `right`, `up`, `down`
- Ranges: `(start,end)` percentages (0–100)
- `start` = top/left edge, `end` = bottom/right edge

**Example**: `right(0,50) = laptop(50,100)` — top half of my right edge → bottom half of laptop's left edge

---

#### `options` — Global Settings

```ini
section: options
    heartbeat = 5000
    switchDelay = 250
    switchDoubleTap = 250
    switchCornerSize = 6
    relativeMouseMoves = false
    clipboardSharing = true
    clipboardSharingSize = 1024
    win32KeepForeground = true
end
```

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `heartbeat` | ms | `5000` | Keep-alive interval |
| `switchDelay` | ms | `250` | Edge dwell before switch |
| `switchDoubleTap` | ms | `250` | Double-tap edge to switch |
| `switchCornerSize` | px | `6` | Corner hotzone size |
| `relativeMouseMoves` | bool | `false` | Relative moves when locked |
| `clipboardSharing` | bool | `true` | Enable clipboard sync |
| `clipboardSharingSize` | KB | `1024` | Max clipboard transfer size |
| `win32KeepForeground` | bool | `true` | Grab focus on switch (Windows) |

---

### Hotkey Actions (`keystroke`, `mousebutton`)

```ini
section: options
    keystroke(control+super+right) = switchInDirection(right)
    keystroke(control+super+left) = switchInDirection(left)
    keystroke(control+alt+delete) = lockCursorToScreen(toggle)
    mousebutton(middle) = switchInDirection(right); switchInDirection(left)
end
```

**Action Syntax:**
```
keystroke(modifiers+key) = action_on_press; action_on_release
```

**Available Actions:**

| Action | Syntax | Description |
|--------|--------|-------------|
| Switch direction | `switchInDirection(dir)` | `dir` = `left`/`right`/`up`/`down` |
| Next screen | `switchToNextScreen()` | Cycle screens |
| Specific screen | `switchToScreen(name)` | Jump to named screen |
| Lock cursor | `lockCursorToScreen(mode)` | `on`/`off`/`toggle` |
| Keystroke | `keystroke(key)` | Synthesize key combo |
| Key down/up | `keyDown(key)`, `keyUp(key)` | Press/release |
| Mouse button | `mousebutton(btn)` | Press/release mouse |
| Mouse down/up | `mouseDown(btn)`, `mouseUp(btn)` | Press/release |

**Key Names:** Standard names (`F1`–`F24`, `Left`, `Right`, `Space`, `Return`, `Escape`, `Tab`, `BackSpace`, `Delete`, `Home`, `End`, `PageUp`, `PageDown`, `Insert`, `KP_0`–`KP_9`, `KP_Add`, `KP_Subtract`, etc.) + Unicode `\uXXXX`.

---

### Example: Three-Monitor Horizontal Layout

```ini
# Physical: [Laptop] [Desktop] [iMac]
section: screens
    laptop:
    desktop:
    imac:
end

section: links
    desktop:
        left = laptop
        right = imac
    laptop:
        right = desktop
    imac:
        left = desktop
end

section: aliases
    laptop:
        laptop.local
    desktop:
        desktop.office
    imac:
        imac.local
end

section: options
    heartbeat = 5000
    switchDelay = 300
    clipboardSharing = true
    clipboardSharingSize = 2048
    keystroke(control+alt+left) = switchInDirection(left)
    keystroke(control+alt+right) = switchInDirection(right)
end
```

---

### Command line

`server` and `client` are required positional arguments. `--settings` (`-s`)
selects the INI file. `--new-instance` skips the single-instance check.
There is no `--debug` flag and no `--no-daemon` flag.

```bat
synergy-core --help
synergy-core --version
synergy-core server
synergy-core client --settings "%APPDATA%\TuPig Synergy\TuPig Synergy.conf"
```

```bash
synergy-core --help
synergy-core --version
synergy-core server
synergy-core client --settings "$HOME/.config/TuPig Synergy/TuPig Synergy.conf"
```

The Windows GUI file is `synergy_<X.Y.Z>.exe`, with `X.Y.Z` from
`cmake/Version.cmake` (currently 1.21.2). `--reset` restores settings to
their defaults.

```bat
synergy_1.21.2.exe --version
synergy_1.21.2.exe --reset
synergy-daemon.exe --version
synergy-daemon.exe --foreground
```

Linux runs `./build/bin/synergy`. macOS runs `build/bin/TuPig Synergy.app`.
Neither platform ships `synergy-daemon`.

### Environment

`USE_LEGACY_NETWORK=1` selects the old socket stack. Leave it unset to keep
the default Qt TLS stack. The program does not read `SYNERGY_PORT`,
`SYNERGY_CONFIG`, `SYNERGY_LOG_LEVEL`, or `SYNERGY_TLS_CERT`. Change the port,
settings path, log level, and certificate in the INI file, or pass
`--settings`.

---

## 中文

### 配置文件位置

配置文件名来自应用名（`kAppName`）。正常构建中该名字是 `TuPig Synergy`，所以文件是 `TuPig Synergy.conf`，不是 `Synergy.conf`。目录名相同。

Windows 上，若 `<安装目录>/settings/TuPig Synergy.conf` 存在，就用这份便携配置。否则使用下面第一个已经存在的文件；两个都不存在时创建用户文件：

1. `%APPDATA%\TuPig Synergy\TuPig Synergy.conf`
2. `<系统盘>\ProgramData\TuPig Synergy\TuPig Synergy.conf`

`QSettingsProxy` 只在完全没有文件路径时才读注册表 `HKCU\Software\TuPig Synergy\TuPig Synergy`。

Linux 与 macOS 上，若设置了 `XDG_CONFIG_HOME`，即使文件还不存在也使用 `$XDG_CONFIG_HOME/TuPig Synergy/TuPig Synergy.conf`。未设置该变量时，使用下面第一个已存在的路径，否则创建用户路径。

#### Linux
1. `~/.config/TuPig Synergy/TuPig Synergy.conf`
2. `/etc/TuPig Synergy/TuPig Synergy.conf`

#### macOS
1. `~/Library/TuPig Synergy/TuPig Synergy.conf`
2. `/Library/TuPig Synergy/TuPig Synergy.conf`

证书和日志放在所选配置文件旁边。

`gui/windowGeometry` 不存在该文件中：它位于独立的状态文件
`<状态目录>/TuPig Synergy.state`（见 `Settings::m_stateKeys`）。

---

### 文件格式

INI 风格，节与键值对：

```ini
[section]
key=value
```

注释以 `#` 或 `;` 开头。仅写入非默认值。

---

### GUI 配置节

#### `[client]` — 客户端模式

| 键 | 类型 | 默认值 | 说明 |
|-----|------|--------|------|
| `dynamicConnectionInterval` | bool | `false` | 重连指数退避 |
| `languageSync` | bool | `true` | 从服务端同步键盘布局 |
| `remoteHost` | string | — | 逗号分隔主机列表 |
| `yScrollScale` | float | `1.0` | 垂直滚动缩放 (0.1–10.0) |
| `xScrollScale` | float | `1.0` | 水平滚动缩放 (0.1–10.0) |
| `invertYScroll` | bool | `false` | 反转垂直滚动 |
| `invertXScroll` | bool | `false` | 反转水平滚动 |
| `xdpRestoreToken` | UUID | — | XDG Portal 恢复令牌 |

#### `[core]` — 通用选项

| 键 | 类型 | 默认值 | 说明 |
|-----|------|--------|------|
| `coreMode` | int | `0` | `0`=无, `1`=客户端, `2`=服务端 |
| `display` | int | 自动 | X11 显示编号 (Linux) |
| `interface` | IP | 自动 | 绑定地址 (`0.0.0.0`=所有) |
| `lastVersion` | string | — | 上次运行版本 |
| `port` | int | `24800` | TCP 端口 |
| `preventSleep` | bool | `false` | 阻止系统休眠 |
| `processMode` | int | 已安装的 Windows 为 `0`，其余为 `1` | `0`=服务，`1`=桌面 |
| `computerName` | string | 主机名 | 唯一计算机标识 |
| `useHooks` | bool | `true` | Windows: 钩子 vs 原始输入 |
| `language` | ISO 639 | `en` | UI 语言代码 |
| `enableEnterCommand` | bool | `false` | 进屏时运行命令 |
| `enterCommand` | string | — | 进屏执行命令 |
| `enableExitCommand` | bool | `false` | 离屏时运行命令 |
| `exitCommand` | string | — | 离屏执行命令 |

#### `[daemon]` — Windows 守护进程

| 键 | 类型 | 默认值 | 说明 |
|-----|------|--------|------|
| `configFile` | path | — | 守护进程启动核心时使用的核心配置文件 |
| `elevate` | bool | 非便携模式为 `true` | 以提升权限运行 (UAC)。便携模式默认为 `false` |
| `logFile` | path | — | 守护进程日志文件 |
| `logLevel` | string | — | 日志详细程度 |

#### `[gui]` — GUI 行为

| 键 | 类型 | 默认值 | 说明 |
|-----|------|--------|------|
| `autoHide` | bool | `false` | 启动时隐藏窗口 |
| `closeReminder` | bool | `true` | 显示"后台运行"提醒 |
| `closeToTray` | bool | `true` | 关闭窗口最小化到托盘 |
| `ignoreMissingKeyboardLayouts` | bool | `false` | 抑制缺失键盘布局的警告 |
| `logExpanded` | bool | `true` | 默认展开日志面板 |
| `showVersionInTitle` | bool | `false` | 标题栏显示版本 |
| `shownFirstConnectedMessage` | bool | `false` | 首次连接提示已显示 |
| `shownServerFirstStartMessage` | bool | `false` | 服务端启动提示已显示 |
| `startCoreWithGui` | bool | `false` | GUI 启动时自动启动核心 |
| `symbolicTrayIcon` | bool | `true` | 单色托盘图标 |
| `windowGeometry` | QRect | — | 保存窗口位置/大小（存于状态文件） |

#### `[log]` — 日志

| 键 | 类型 | 默认值 | 说明 |
|-----|------|--------|------|
| `file` | path | — | 日志文件路径 |
| `level` | enum | `INFO` | `FATAL`, `ERROR`, `WARNING`, `INFO`, `DEBUG`, `VERBOSE` |
| `toFile` | bool | `false` | 写入文件 |
| `guiDebug` | bool | `false` | 包含 GUI 内部调试 |

#### `[security]` — TLS/安全

| 键 | 类型 | 默认值 | 说明 |
|-----|------|--------|------|
| `checkPeerFingerprints` | bool | `true` | 验证对端证书指纹 |
| `certificate` | path | — | TLS 证书文件 |
| `keySize` | int | `2048` | `2048` 或 `4096` |
| `tlsEnabled` | bool | `true` | 启用 TLS 加密 |
| `blockedKeyCombos` | list | 空 | 要拦截的键组合，格式 `key[:mask]`（如 `0xEFFF:0x0005`）。为空则不拦截任何组合 |

#### `[fileTransfer]` — 拖拽文件传输

| 键 | 类型 | 默认值 | 说明 |
|-----|------|--------|------|
| `enabled` | bool | `false` | 是否接受服务端推送的文件 |
| `dropDirectory` | path | — | 接收文件的落盘目录；为空则拒绝所有传输 |
| `maxFileSizeMb` | int | `64` | 单个文件大小上限 |
| `maxFileCount` | int | `32` | 单次拖拽的文件数上限 |

> **默认关闭。** 启用后服务端可向本机写入文件，故采用显式启用。开启时，每个文件名
> 都会被归约为单一安全文件名 —— 路径穿越（`../`）、绝对路径、盘符、控制字符、
> Windows 保留设备名与过长名一律**拒绝**而非就地修正，其内容也被丢弃，不会以替换名
> 写入。写入先落暂存文件再改名，且**从不覆盖**已有文件（改用 `name (1).ext`）。
> 声明大小、文件数与累计总量均受限。

#### `[server]` — 服务端模式

| 键 | 类型 | 默认值 | 说明 |
|-----|------|--------|------|
| `clipboardSize` | int | `3` | 剪贴板传输大小上限（MiB） |
| `externalConfig` | bool | `false` | 使用外部服务端配置文件 |
| `externalConfigFile` | path | — | 服务端配置文件路径 |
| `protocol` | enum | `synergy` | `synergy` 或 `barrier` |
| `xdpRestoreToken` | UUID | — | XDG Portal 恢复令牌 |

#### `[internalConfig]` — 服务端布局 (GUI 生成)

复杂嵌套结构定义屏幕拓扑。详见下方 **服务端配置**。

---

### 服务端配置 (文本格式)

供 `synergy-core -c config.conf` 使用。由 GUI 的 `[internalConfig]` 导出。

#### 语法

```ini
section: name
    arg = value
end
```

节：`screens`、`aliases`、`links`、`options`

---

#### `screens` — 计算机定义

```ini
section: screens
    my-desktop:
        halfDuplexCapsLock = false
        halfDuplexNumLock = false
    laptop:
        meta = alt
    workstation:
end
```

**屏幕选项：**

| 选项 | 类型 | 说明 |
|------|------|------|
| `halfDuplexCapsLock` | bool | CapsLock 不发送按下+释放 |
| `halfDuplexNumLock` | bool | NumLock 不发送按下+释放 |
| `halfDuplexScrollLock` | bool | ScrollLock 不发送按下+释放 |
| `xtestIsXineramaUnaware` | bool | XTest+Xinerama 兼容 |
| `preserveFocus` | bool | 切屏不丢焦点 |
| `switchCorners` | corners | 角落禁用切屏 |
| `switchCornerSize` | int | 角落热区像素 |
| `shift` / `ctrl` / `alt` / `meta` / `super` | modifier | 修饰键重映射 |

---

#### `aliases` — 主机名别名

```ini
section: aliases
    my-desktop:
        desktop.local
        desktop.office.lan
    laptop:
        laptop.home
end
```

客户端可用规范名或任意别名连接。

---

#### `links` — 屏幕拓扑

```ini
section: links
    my-desktop:
        right = laptop
        up(50,100) = workstation(0,50)
    laptop:
        left = my-desktop
    workstation:
        down = my-desktop
end
```

**链接语法：**
```
{方向}[(起,止)] = 目标[(起,止)]
```

- 方向：`left`、`right`、`up`、`down`
- 范围：`(起,止)` 百分比 (0–100)
- `起` = 上/左边缘，`止` = 下/右边缘

**示例**：`right(0,50) = laptop(50,100)` — 我的右边缘上半段 → laptop 左边缘下半段

---

#### `options` — 全局选项

```ini
section: options
    heartbeat = 5000
    switchDelay = 250
    switchDoubleTap = 250
    switchCornerSize = 6
    relativeMouseMoves = false
    clipboardSharing = true
    clipboardSharingSize = 1024
    win32KeepForeground = true
end
```

| 选项 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `heartbeat` | ms | `5000` | 保活间隔 |
| `switchDelay` | ms | `250` | 边缘停留延迟 |
| `switchDoubleTap` | ms | `250` | 双击边缘切屏 |
| `switchCornerSize` | px | `6` | 角落热区大小 |
| `relativeMouseMoves` | bool | `false` | 锁定时相对移动 |
| `clipboardSharing` | bool | `true` | 启用剪贴板同步 |
| `clipboardSharingSize` | KB | `1024` | 最大剪贴板传输 |
| `win32KeepForeground` | bool | `true` | 切屏抢占焦点 (Windows) |

---

### 热键动作 (`keystroke`, `mousebutton`)

```ini
section: options
    keystroke(control+super+right) = switchInDirection(right)
    keystroke(control+super+left) = switchInDirection(left)
    keystroke(control+alt+delete) = lockCursorToScreen(toggle)
    mousebutton(middle) = switchInDirection(right); switchInDirection(left)
end
```

**动作语法：**
```
keystroke(修饰键+键) = 按下动作; 释放动作
```

**可用动作：**

| 动作 | 语法 | 说明 |
|------|------|------|
| 方向切屏 | `switchInDirection(dir)` | `dir` = `left`/`right`/`up`/`down` |
| 循环切屏 | `switchToNextScreen()` | 顺序切换 |
| 指定屏幕 | `switchToScreen(name)` | 跳转到命名屏幕 |
| 锁定光标 | `lockCursorToScreen(mode)` | `on`/`off`/`toggle` |
| 合成按键 | `keystroke(key)` | 模拟组合键 |
| 按下/释放 | `keyDown(key)`, `keyUp(key)` | 按下/释放 |
| 鼠标按键 | `mousebutton(btn)` | 模拟鼠标按键 |
| 鼠标按下/释放 | `mouseDown(btn)`, `mouseUp(btn)` | 按下/释放 |

**键名：** 标准名 (`F1`–`F24`, `Left`, `Right`, `Space`, `Return`, `Escape`, `Tab`, `BackSpace`, `Delete`, `Home`, `End`, `PageUp`, `PageDown`, `Insert`, `KP_0`–`KP_9`, `KP_Add`, `KP_Subtract` 等) + Unicode `\uXXXX`。

---

### 示例：三显示器横向布局

```ini
# 物理布局: [Laptop] [Desktop] [iMac]
section: screens
    laptop:
    desktop:
    imac:
end

section: links
    desktop:
        left = laptop
        right = imac
    laptop:
        right = desktop
    imac:
        left = desktop
end

section: aliases
    laptop:
        laptop.local
    desktop:
        desktop.office
    imac:
        imac.local
end

section: options
    heartbeat = 5000
    switchDelay = 300
    clipboardSharing = true
    clipboardSharingSize = 2048
    keystroke(control+alt+left) = switchInDirection(left)
    keystroke(control+alt+right) = switchInDirection(right)
end
```

---

### 命令行

`server` 与 `client` 是必填的位置参数。`--settings`（`-s`）指定 INI 文件。`--new-instance` 跳过单实例检查。没有 `--debug`，也没有 `--no-daemon`。

```bat
synergy-core --help
synergy-core --version
synergy-core server
synergy-core client --settings "%APPDATA%\TuPig Synergy\TuPig Synergy.conf"
```

```bash
synergy-core --help
synergy-core --version
synergy-core server
synergy-core client --settings "$HOME/.config/TuPig Synergy/TuPig Synergy.conf"
```

Windows GUI 文件是 `synergy_<X.Y.Z>.exe`，`X.Y.Z` 来自 `cmake/Version.cmake`（当前 1.21.2）。`--reset` 把设置恢复为默认值。

```bat
synergy_1.21.2.exe --version
synergy_1.21.2.exe --reset
synergy-daemon.exe --version
synergy-daemon.exe --foreground
```

Linux 运行 `./build/bin/synergy`。macOS 运行 `build/bin/TuPig Synergy.app`。这两个平台都没有 `synergy-daemon`。

### 环境变量

`USE_LEGACY_NETWORK=1` 改用旧的套接字栈。不设置则使用默认的 Qt TLS 栈。程序不读取 `SYNERGY_PORT`、`SYNERGY_CONFIG`、`SYNERGY_LOG_LEVEL` 或 `SYNERGY_TLS_CERT`。端口、配置路径、日志级别和证书写在 INI 里，或用 `--settings` 指定文件。