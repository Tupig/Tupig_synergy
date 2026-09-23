# Protocol Reference / 协议参考

> **Language / 语言**: [English](#english) | [中文](#中文)

---

## English

### Overview

The TuPig Synergy protocol enables keyboard and mouse sharing between multiple computers over TCP/IP. It uses a client-server architecture with role separation:

| Role | Description |
|------|-------------|
| **Network Role** | Connection architecture |
| • **Server** | Listens on TCP port 24800 |
| • **Client** | Initiates connection to Server |
| **Control Role** | Input flow direction |
| • **Primary** | Shares keyboard/mouse (controls others) |
| • **Secondary** | Receives input events |

> **Note**: Typically the Primary also acts as Server, but roles can be separated for firewall traversal.

### Protocol Versions

| Version | Year | Key Features |
|---------|------|--------------|
| 1.0 | 2001 | Basic K/M sharing |
| 1.1 | 2002 | Physical key codes (`KeyButton`) |
| 1.2 | 2006 | Relative mouse movement |
| 1.3 | 2006 | Keep-alive, horizontal scroll |
| 1.4 | 2012 | TLS encryption |
| 1.5 | 2013 | File transfer (drag-drop) |
| 1.6 | 2014 | Clipboard streaming |
| 1.7 | 2021 | Secure input notifications |
| 1.8 | 2025 | Language synchronization |

**Current**: v1.8 (product version from `extra/cmake/Version.cmake`)

---

### Connection State Machine

```
DISCONNECTED ──TCP──► CONNECTING ──OK──► HANDSHAKE ──OK──► CONNECTED
     ▲                                      │                    │
     │                TCP Fail              │       CINN         │
     └──────────────────────────────────────┘                    │
                                    │                            ▼
                               COUT ◄───────────────────── ACTIVE
                                    │                            │
                                    └──────── CCLOSE ────────────┘
```

| State | Description |
|-------|-------------|
| **Disconnected** | Initial/final state, no connection |
| **Connecting** | TCP handshake in progress |
| **Handshake** | Version negotiation + auth (30s timeout) |
| **Connected** | Authenticated, awaiting `CINN` |
| **Active** | Receiving/processing input events |

---

### Message Categories

| Prefix | Category | Direction | Purpose |
|--------|----------|-----------|---------|
| (none) | **Handshake** | S→C / C→S | Version negotiation |
| `C` | **Commands** | Both | Screen control, keep-alive |
| `D` | **Data** | Both | Input events, clipboard |
| `Q` | **Queries** | S→C | Information requests |
| `E` | **Errors** | S→C | Error notifications |

---

### Key Messages

#### Handshake

| Message | Constant | Direction | Description |
|---------|----------|-----------|-------------|
| `Hello` | `kMsgHello` | S→C | Server identifies protocol version |
| `HelloBack` | `kMsgHelloBack` | C→S | Client responds with version + screen name |

#### Commands (`C*`)

| Message | Constant | Direction | Purpose |
|---------|----------|-----------|---------|
| `CALV` | `kMsgCKeepAlive` | Both | Keep-alive (every 3s) |
| `CBYE` / `CCLOSE` | `kMsgCClose` | S→C | Close connection |
| `CINN` | `kMsgCEnter` | S→C | Grant screen control |
| `COUT` | `kMsgCLeave` | S→C | Revoke screen control |
| `CROP` | `kMsgCResetOptions` | S→C | Reset options to defaults |
| `CSEC` | `kMsgCScreenSaver` | S→C | Screensaver control |

#### Data (`D*`)

| Message | Constant | Direction | Purpose |
|---------|----------|-----------|---------|
| `DKDN` | `kMsgDKeyDown` | S→C | Key press |
| `DKUP` | `kMsgDKeyUp` | S→C | Key release |
| `DKRP` | `kMsgDKeyRepeat` | S→C | Key repeat |
| `DMMV` | `kMsgDMouseMove` | S→C | Absolute mouse move |
| `DMRM` | `kMsgDMouseRelMove` | S→C | Relative mouse move |
| `DMDN` | `kMsgDMouseDown` | S→C | Mouse button down |
| `DMUP` | `kMsgDMouseUp` | S→C | Mouse button up |
| `DMWM` | `kMsgDMouseWheel` | S→C | Mouse wheel |
| `DCLP` | `kMsgCClipboard` | Both | Clipboard ownership |
| `DCLP` | `kMsgDClipboard` | Both | Clipboard data |
| `DDRG` | `kMsgDDragInfo` | S→C | Drag file info (v1.5+) |
| `DFTR` | `kMsgDFileTransfer` | Both | File transfer data (v1.5+) |
| `DSOP` | `kMsgDSetOptions` | S→C | Set client options |

#### Queries & Errors

| Message | Constant | Direction | Purpose |
|---------|----------|-----------|---------|
| `QINF` | `kMsgQInfo` | S→C | Request screen info |
| `DINF` | `kMsgDInfo` | C→S | Screen dimensions, name |
| `EICV` | `kMsgEIncompatible` | S→C | Version mismatch |
| `EBSY` | `kMsgEBusy` | S→C | Server busy |
| `EBAD` | `kMsgEBad` | S→C | Protocol violation |

---

### Typical Message Flow

```
Client                                    Server
  │                                         │
  ├─ TCP SYN ────────────────────────────► │
  ├─ TCP SYN+ACK ◄──────────────────────── ┤
  ├─ TCP ACK ───────────────────────────►  │
  │                                         │
  ├─ HelloBack (v1.8, "my-screen") ──────► │ Handshake
  ├─ ◄──────── Hello (v1.8) ────────────── ┤
  │                                         │
  ├─ ◄──────── QINF ────────────────────── ┤ Query info
  ├─ DINF (1920x1080, "my-screen") ──────► │
  │                                         │
  ├─ ◄──────── DSOP (options) ──────────── ┤ Set options
  │                                         │
  ├─ CALV ◄──────────────────────────────── ┤ Keep-alive
  ├─ CALV ────────────────────────────────► │ (every 3s)
  │                                         │
  ├─ ◄──────── CINN (0,0, seq, mods) ────── ┤ Enter screen
  │                                         │
  ├─ ◄──────── DMMV (x, y) ──────────────── ┤ Mouse move
  ├─ ◄──────── DMDN (btn=1) ─────────────── ┤ Mouse down
  ├─ ◄──────── DMUP (btn=1) ─────────────── ┤ Mouse up
  ├─ ◄──────── DKDN (key=0x41, mods=0) ──── ┤ Key 'A' down
  ├─ ◄──────── DKUP (key=0x41, mods=0) ──── ┤ Key 'A' up
  │                                         │
  ├─ ◄──────── COUT ─────────────────────── ┤ Leave screen
  │                                         │
  ├─ ◄──────── CCLOSE ───────────────────── ┤ Close
  │                                         │
```

---

### Protocol Constraints

| Constraint | Value | Constant |
|------------|-------|----------|
| Max Message Size | 4 MB | `PROTOCOL_MAX_MESSAGE_LENGTH` |
| Max List Elements | 1,048,576 | `PROTOCOL_MAX_LIST_LENGTH` |
| Max Hello Size | 1 KB | `kMaxHelloLength` |
| Keep-alive Interval | 3.0 s | `kKeepAliveRate` |
| Client Timeout | 9.0 s (3×keepalive) | — |
| Handshake Timeout | 30 s | — |
| Default Port | 24800 | — |

---

### TLS Encryption (v1.4+)

```
1. TCP Connection Established
2. TLS Handshake (Client validates Server cert)
3. Protocol Handshake (Hello/HelloBack) over TLS
```

**Certificate Requirements:**
- RSA or DSA key, ≥2048 bits
- Client **must** validate server certificate
- Self-signed certs supported for LAN use

---

### Key Implementation Files

| File | Purpose |
|------|---------|
| `src/lib/deskflow/protocol/ProtocolTypes.h` | Complete message definitions, incl. `ClientInfo` |
| `src/lib/deskflow/protocol/ProtocolUtil.h` | Serialization/parsing utilities |
| `src/lib/net/SecureSocket.h` | TLS wrapper |

---

### Implementation Checklist

#### Basic Client

- [ ] TCP connect to port 24800
- [ ] Hello/HelloBack handshake
- [ ] Version negotiation
- [ ] Keep-alive response
- [ ] `CINN`/`COUT` screen enter/leave
- [ ] Keyboard/mouse event synthesis
- [ ] Modifier key synchronization

#### Advanced Features

- [ ] Clipboard (`DCLP`/`C_CLIPBOARD`)
- [ ] File transfer (`DDRG`/`DFTR`)
- [ ] TLS encryption
- [ ] Secure input notifications (`SECN`)
- [ ] Language sync (`LSYN` v1.8+)

---

## 中文

### 概述

TuPig Synergy 协议实现多台计算机间通过 TCP/IP 共享键盘和鼠标。采用客户端-服务器架构，并分离网络角色与控制角色：

| 角色 | 说明 |
|------|------|
| **网络角色** | 连接架构 |
| • **Server** | 监听 TCP 端口 24800 |
| • **Client** | 主动连接 Server |
| **控制角色** | 输入流向 |
| • **Primary** | 共享键鼠（控制其他机器） |
| • **Secondary** | 接收输入事件 |

> **注意**：通常 Primary 兼任 Server，但角色可分离以穿越防火墙。

### 协议版本

| 版本 | 年份 | 关键特性 |
|------|------|----------|
| 1.0 | 2001 | 基础键鼠共享 |
| 1.1 | 2002 | 物理键码 (`KeyButton`) |
| 1.2 | 2006 | 相对鼠标移动 |
| 1.3 | 2006 | 保活、水平滚动 |
| 1.4 | 2012 | TLS 加密 |
| 1.5 | 2013 | 文件传输 (拖拽) |
| 1.6 | 2014 | 剪贴板流式传输 |
| 1.7 | 2021 | 安全输入通知 |
| 1.8 | 2025 | 语言同步 |

**当前版本**：v1.8（产品版本见 `extra/cmake/Version.cmake`）

---

### 连接状态机

```
DISCONNECTED ──TCP──► CONNECTING ──OK──► HANDSHAKE ──OK──► CONNECTED
     ▲                                      │                    │
     │                TCP 失败               │       CINN         │
     └──────────────────────────────────────┘                    │
                                    │                            ▼
                               COUT ◄───────────────────── ACTIVE
                                    │                            │
                                    └──────── CCLOSE ────────────┘
```

| 状态 | 说明 |
|------|------|
| **Disconnected** | 初始/最终状态，无连接 |
| **Connecting** | TCP 握手进行中 |
| **Handshake** | 版本协商+认证 (30秒超时) |
| **Connected** | 已认证，等待 `CINN` |
| **Active** | 接收/处理输入事件 |

---

### 消息分类

| 前缀 | 类别 | 方向 | 用途 |
|------|------|------|------|
| (无) | **握手** | S→C / C→S | 版本协商 |
| `C` | **命令** | 双向 | 屏幕控制、保活 |
| `D` | **数据** | 双向 | 输入事件、剪贴板 |
| `Q` | **查询** | S→C | 信息请求 |
| `E` | **错误** | S→C | 错误通知 |

---

### 关键消息

#### 握手

| 消息 | 常量 | 方向 | 说明 |
|------|------|------|------|
| `Hello` | `kMsgHello` | S→C | Server 宣告协议版本 |
| `HelloBack` | `kMsgHelloBack` | C→S | Client 响应版本+屏幕名 |

#### 命令 (`C*`)

| 消息 | 常量 | 方向 | 用途 |
|------|------|------|------|
| `CALV` | `kMsgCKeepAlive` | 双向 | 保活 (每 3 秒) |
| `CBYE`/`CCLOSE` | `kMsgCClose` | S→C | 关闭连接 |
| `CINN` | `kMsgCEnter` | S→C | 授予屏幕控制权 |
| `COUT` | `kMsgCLeave` | S→C | 撤销屏幕控制权 |
| `CROP` | `kMsgCResetOptions` | S→C | 重置选项为默认 |
| `CSEC` | `kMsgCScreenSaver` | S→C | 屏保控制 |

#### 数据 (`D*`)

| 消息 | 常量 | 方向 | 用途 |
|------|------|------|------|
| `DKDN` | `kMsgDKeyDown` | S→C | 按键按下 |
| `DKUP` | `kMsgDKeyUp` | S→C | 按键释放 |
| `DKRP` | `kMsgDKeyRepeat` | S→C | 按键重复 |
| `DMMV` | `kMsgDMouseMove` | S→C | 绝对鼠标移动 |
| `DMRM` | `kMsgDMouseRelMove` | S→C | 相对鼠标移动 |
| `DMDN` | `kMsgDMouseDown` | S→C | 鼠标按键按下 |
| `DMUP` | `kMsgDMouseUp` | S→C | 鼠标按键释放 |
| `DMWM` | `kMsgDMouseWheel` | S→C | 滚轮 |
| `DCLP` | `kMsgCClipboard` | 双向 | 剪贴板所有权 |
| `DCLP` | `kMsgDClipboard` | 双向 | 剪贴板数据 |
| `DDRG` | `kMsgDDragInfo` | S→C | 拖拽文件信息 (v1.5+) |
| `DFTR` | `kMsgDFileTransfer` | 双向 | 文件传输数据 (v1.5+) |
| `DSOP` | `kMsgDSetOptions` | S→C | 设置客户端选项 |

#### 查询与错误

| 消息 | 常量 | 方向 | 用途 |
|------|------|------|------|
| `QINF` | `kMsgQInfo` | S→C | 请求屏幕信息 |
| `DINF` | `kMsgDInfo` | C→S | 屏幕尺寸、名称 |
| `EICV` | `kMsgEIncompatible` | S→C | 版本不兼容 |
| `EBSY` | `kMsgEBusy` | S→C | Server 忙 |
| `EBAD` | `kMsgEBad` | S→C | 协议违规 |

---

### 典型消息流

```
Client                                    Server
  │                                         │
  ├─ TCP SYN ────────────────────────────► │
  ├─ TCP SYN+ACK ◄──────────────────────── ┤
  ├─ TCP ACK ───────────────────────────►  │
  │                                         │
  ├─ HelloBack (v1.8, "my-screen") ──────► │ 握手
  ├─ ◄──────── Hello (v1.8) ────────────── ┤
  │                                         │
  ├─ ◄──────── QINF ────────────────────── ┤ 查询信息
  ├─ DINF (1920x1080, "my-screen") ──────► │
  │                                         │
  ├─ ◄──────── DSOP (options) ──────────── ┤ 设置选项
  │                                         │
  ├─ CALV ◄──────────────────────────────── ┤ 保活
  ├─ CALV ────────────────────────────────► │ (每 3 秒)
  │                                         │
  ├─ ◄──────── CINN (0,0, seq, mods) ────── ┤ 进入屏幕
  │                                         │
  ├─ ◄──────── DMMV (x, y) ──────────────── ┤ 鼠标移动
  ├─ ◄──────── DMDN (btn=1) ─────────────── ┤ 鼠标按下
  ├─ ◄──────── DMUP (btn=1) ─────────────── ┤ 鼠标释放
  ├─ ◄──────── DKDN (key=0x41, mods=0) ──── ┤ 'A' 键按下
  ├─ ◄──────── DKUP (key=0x41, mods=0) ──── ┤ 'A' 键释放
  │                                         │
  ├─ ◄──────── COUT ─────────────────────── ┤ 离开屏幕
  │                                         │
  ├─ ◄──────── CCLOSE ───────────────────── ┤ 关闭连接
  │                                         │
```

---

### 协议约束

| 约束 | 数值 | 常量 |
|------|------|------|
| 最大消息大小 | 4 MB | `PROTOCOL_MAX_MESSAGE_LENGTH` |
| 最大列表元素 | 1,048,576 | `PROTOCOL_MAX_LIST_LENGTH` |
| 最大 Hello 大小 | 1 KB | `kMaxHelloLength` |
| 保活间隔 | 3.0 秒 | `kKeepAliveRate` |
| 客户端超时 | 9.0 秒 (3×保活) | — |
| 握手超时 | 30 秒 | — |
| 默认端口 | 24800 | — |

---

### TLS 加密 (v1.4+)

```
1. TCP 连接建立
2. TLS 握手 (Client 验证 Server 证书)
3. 协议握手通过 TLS 进行
```

**证书要求：**
- RSA 或 DSA 密钥，≥2048 位
- Client **必须** 验证 Server 证书
- 局域网支持自签名证书

---

### 核心实现文件

| 文件 | 用途 |
|------|------|
| `src/lib/deskflow/protocol/ProtocolTypes.h` | 完整消息定义（含 `ClientInfo`） |
| `src/lib/deskflow/protocol/ProtocolUtil.h` | 序列化/解析工具 |
| `src/lib/net/SecureSocket.h` | TLS 封装 |

---

### 实现检查清单

#### 基础客户端

- [ ] TCP 连接端口 24800
- [ ] Hello/HelloBack 握手
- [ ] 版本协商
- [ ] 保活响应
- [ ] `CINN`/`COUT` 屏幕进出
- [ ] 键盘/鼠标事件合成
- [ ] 修饰键同步

#### 高级特性

- [ ] 剪贴板 (`DCLP`/`C_CLIPBOARD`)
- [ ] 文件传输 (`DDRG`/`DFTR`)
- [ ] TLS 加密
- [ ] 安全输入通知 (`SECN`)
- [ ] 语言同步 (`LSYN` v1.8+)