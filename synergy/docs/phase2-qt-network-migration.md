# Phase 2: QtNetwork 迁移 — 详细实施方案

> **文档版本**: v1.0  
> **编制日期**: 2026-09-18  
> **对应问题**: Q-1 (SocketMultiplexer 死锁), Q-2 (4MB 栈缓冲), T-3 (Raw 指针)  
> **工期**: Week 3-6 (4 周)  
> **状态**: 待审核

---

## 1. 现状分析

### 1.1 当前网络栈架构

```
┌─────────────────────────────────────────────────────┐
│                   Application Layer                  │
│         (Server / Client / GUI)                      │
├─────────────────────────────────────────────────────┤
│                  ISocketFactory                      │
│              TCPSocketFactory                        │
├─────────────────────────────────────────────────────┤
│  IDataSocket          │  IListenSocket               │
│  ┌──────────┐         │  ┌──────────────┐           │
│  │TCPSocket │         │  │TCPListenSocket│           │
│  └────┬─────┘         │  └──────────────┘           │
│       │               │                             │
│  ┌────┴─────┐         │  ┌──────────────┐           │
│  │SecureSocket│        │  │SecureListenSocket│       │
│  └──────────┘         │  └──────────────┘           │
├─────────────────────────────────────────────────────┤
│              SocketMultiplexer                       │
│         (poll-based, 1s timeout, dual-lock)          │
├─────────────────────────────────────────────────────┤
│              ArchNetwork (platform)                  │
│         epoll / IOCP / kqueue / poll                 │
└─────────────────────────────────────────────────────┘
```

### 1.2 关键问题

| 问题 | 位置 | 严重度 | 影响 |
|------|------|--------|------|
| **Q-1 双层锁死锁** | `SocketMultiplexer.cpp:263-297` | High | `m_mutex` + cursor 迭代锁，job 执行期间持锁 |
| **Q-2 4MB 栈缓冲** | `TCPSocket.cpp:24`, `SecureSocket.cpp:37` | Medium | 栈溢出风险，大消息时内存峰值 |
| **Q-1 1s poll 超时** | `SocketMultiplexer.cpp:170` | Medium | 心跳延迟、响应慢 |
| **T-3 Raw 指针** | 全网络层 | Medium | 内存泄漏、double-free 风险 |

### 1.3 已有条件

- Qt6::Network 已在 `CMakeLists.txt:13` 中引入 (`find_package(Qt6 Network)`)
- `ISocketFactory` / `IDataSocket` / `IListenSocket` 接口清晰
- 现有测试覆盖有限，但接口稳定

---

## 2. 目标架构

```
┌─────────────────────────────────────────────────────┐
│                   Application Layer                  │
│         (Server / Client / GUI)                      │
├─────────────────────────────────────────────────────┤
│                  ISocketFactory                      │
│           QtNetworkSocketFactory (新)                │
├─────────────────────────────────────────────────────┤
│  IDataSocket          │  IListenSocket               │
│  ┌──────────────┐     │  ┌──────────────────┐       │
│  │QtTcpTransport│     │  │QtTcpListenTransport│     │
│  └──────┬───────┘     │  └──────────────────┘       │
│         │             │                             │
│  ┌──────┴───────┐     │  ┌──────────────────┐       │
│  │QtTlsTransport│     │  │QtTlsListenTransport│     │
│  └──────────────┘     │  └──────────────────┘       │
├─────────────────────────────────────────────────────┤
│              Qt Event Loop (QCoreApplication)        │
│         无独立网络线程，无 poll 超时                   │
├─────────────────────────────────────────────────────┤
│              Qt Network Module                       │
│         QTcpSocket / QSslSocket / QTcpServer         │
└─────────────────────────────────────────────────────┘
```

### 2.1 核心变化

| 维度 | Before | After |
|------|--------|-------|
| 事件循环 | 自研 `SocketMultiplexer` (poll + 1s 超时) | Qt Event Loop (epoll/IOCP/kqueue 原生集成) |
| 线程模型 | 独立网络线程 + 双层锁 | 主线程 Qt 事件驱动，无锁 |
| TLS | OpenSSL `SSL_CTX` 手动管理 | `QSslSocket` 自动管理 |
| 缓冲区 | 4MB 栈上 `StreamBuffer` | Qt 内置缓冲，背压通过 `bytesWritten` 信号 |
| 内存管理 | Raw pointer + 手动 delete | `std::unique_ptr` / `std::shared_ptr` |

---

## 3. 分步实施计划

### Step 1: SocketMultiplexer 并发重构 (Week 3, 2 天)

**目标**: 消除双层锁死锁风险，不改变外部接口

**变更文件**:
- `src/lib/net/SocketMultiplexer.cpp` — 重构内部实现
- `src/lib/net/SocketMultiplexer.h` — 移除 cursor 机制

**实施方案**:
```
当前: m_mutex (全局) + cursor 锁 (迭代保护)
目标: 单 mutex + condition_variable，无 cursor
```

1. 移除 `newCursor()` / `nextCursor()` / `deleteCursor()` 机制
2. `serviceThread` 改为: lock → copy job list → unlock → 执行 jobs → lock → 更新
3. `addSocket` / `removeSocket` 直接操作 `m_socketJobs`，持锁时间 < 1μs
4. 用 `std::condition_variable` 替代 `ARCH->sleep(0.1)` 空等

**验收**:
- [ ] TSAN 24h 0 报警
- [ ] 无死锁（压力测试 10k 次 add/remove 循环）
- [ ] 心跳延迟 < 100ms（当前 ~1s）

**预案**:
- **回滚**: 保留旧 `SocketMultiplexer` 代码在 `#ifdef LEGACY_MULTIPLEXER` 中
- **降级**: 若 TSAN 报错，回退到单线程模型（移除 serviceThread）

---

### Step 2: QtNetwork 抽象层 + 双实现骨架 (Week 3, 3 天)

**目标**: 定义统一接口，实现 Qt 后端骨架，保持 Legacy 后端可用

**新增文件**:
```
src/lib/net/
├── INetworkTransport.h          # 统一传输接口
├── QtNetworkTransport.h/.cpp    # Qt 实现骨架
├── LegacyNetworkTransport.h/.cpp # 现有实现包装
└── NetworkTransportFactory.h/.cpp # 工厂，运行时切换
```

**接口设计**:
```cpp
class INetworkTransport {
public:
  virtual ~INetworkTransport() = default;
  
  // 连接管理
  virtual void connect(const NetworkAddress &addr) = 0;
  virtual void close() = 0;
  virtual bool isConnected() const = 0;
  
  // 数据传输
  virtual uint32_t read(void *buffer, uint32_t n) = 0;
  virtual void write(const void *buffer, uint32_t n) = 0;
  virtual void flush() = 0;
  
  // 状态查询
  virtual bool isReady() const = 0;
  virtual bool isFatal() const = 0;
  virtual uint32_t getSize() const = 0;
  
  // TLS (可选)
  virtual void setSecurityLevel(SecurityLevel level) = 0;
  virtual SecurityLevel securityLevel() const = 0;
};
```

**验收**:
- [ ] 编译通过，双实现可实例化
- [ ] 现有测试全部通过（使用 Legacy 实现）
- [ ] 工厂可运行时切换

**预案**:
- **回滚**: `NetworkTransportFactory` 默认返回 Legacy 实现
- **降级**: 若 Qt 实现不稳定，保持 Legacy 作为默认

---

### Step 3: QtTcpTransport 替代 TCPSocket (Week 4-5, 5 天)

**目标**: 实现基于 `QTcpSocket` 的 TCP 传输，替代 `TCPSocket`

**新增文件**:
- `src/lib/net/QtTcpTransport.h/.cpp`
- `src/lib/net/QtTcpListenTransport.h/.cpp`

**关键实现**:
```cpp
class QtTcpTransport : public IDataSocket, public INetworkTransport {
public:
  QtTcpTransport(IEventQueue *events);
  
  // IDataSocket
  void connect(const NetworkAddress &) override;
  void close() override;
  uint32_t read(void *buffer, uint32_t n) override;
  void write(const void *buffer, uint32_t n) override;
  
private:
  QTcpSocket *m_socket = nullptr;  // Qt 管理生命周期
  IEventQueue *m_events;
};
```

**关键差异**:
| 维度 | TCPSocket | QtTcpTransport |
|------|-----------|----------------|
| 缓冲区 | 4MB 栈上 `StreamBuffer` | Qt 内置 `QByteArray`，动态增长 |
| 非阻塞 | 手动 `poll()` | Qt 信号驱动 (`readyRead`, `bytesWritten`) |
| 背压 | 无 | `bytesWritten` 信号 + 写入队列上限 |
| 错误处理 | `errno` | `QAbstractSocket::SocketError` |

**验收**:
- [ ] TCP 连接/收发/断开全流程通过
- [ ] 大消息 (>1MB) 不 OOM
- [ ] 慢消费者背压生效（内存 < 50MB）

**预案**:
- **回滚**: `ISocketFactory::create()` 可返回 Legacy `TCPSocket`
- **降级**: 若 Qt 事件循环与现有 `IEventQueue` 冲突，用 `QTimer` 桥接

---

### Step 4: QtTlsTransport 替代 SecureSocket (Week 4-5, 5 天)

**目标**: 实现基于 `QSslSocket` 的 TLS 传输，替代 `SecureSocket`

**新增文件**:
- `src/lib/net/QtTlsTransport.h/.cpp`
- `src/lib/net/QtTlsListenTransport.h/.cpp`

**关键实现**:
```cpp
class QtTlsTransport : public QtTcpTransport {
public:
  QtTlsTransport(IEventQueue *events, SecurityLevel level);
  
  // TLS 配置
  void setLocalCertificate(const QString &path);
  void setCaCertificates(const QStringList &paths);
  void setPeerVerifyMode(QSslSocket::PeerVerifyMode mode);
  
  // TLS 操作
  void startClientEncryption();
  void startServerEncryption();
  
private:
  QSslSocket *m_sslSocket;  // 继承自 QTcpSocket
  SecurityLevel m_securityLevel;
};
```

**TLS 特性对比**:
| 特性 | SecureSocket (OpenSSL) | QtTlsTransport (QSslSocket) |
|------|------------------------|-----------------------------|
| TLS 版本 | 手动设置 `SSL_CTX` | 自动协商最高版本 |
| 证书验证 | 自定义回调 | `QSslCertificate` + `verifyHost` |
| 指纹验证 | 手动实现 | 可扩展 `QSslCertificate::digest()` |
| OCSP Stapling | 需手动实现 | Qt 6.5+ 内置支持 |
| 会话复用 | 手动 `SSL_CTX` | 自动 |

**验收**:
- [ ] TLS 1.3 握手成功 (Wireshark 验证)
- [ ] 证书链验证通过
- - [ ] 指纹 TOFU 验证通过
- [ ] 自签名证书拒绝（除非配置允许）

**预案**:
- **回滚**: 工厂可返回 Legacy `SecureSocket`
- **降级**: 若 Qt TLS 与 OpenSSL 版本冲突，保留 OpenSSL 作为 fallback

---

### Step 5: T-3 网络层智能指针化 (Week 5, 2 天)

**目标**: 全面消除 Raw owning pointer

**变更范围**:
```
TCPSocketFactory::create() → 返回 unique_ptr<IDataSocket>
SecureSocketFactory::create() → 返回 unique_ptr<IDataSocket>
所有调用点 → 适配 unique_ptr
```

**关键模式**:
```cpp
// Before
IDataSocket *socket = factory->create(family, securityLevel);
// ... use socket ...
delete socket;

// After
auto socket = factory->create(family, securityLevel);
// ... use socket ...
// 自动释放
```

**验收**:
- [ ] 0 个 Raw owning pointer（`grep -r "new.*Socket" src/` 仅出现在工厂内部）
- [ ] 无 double-free（ASAN 通过）
- [ ] 无 use-after-free（ASAN 通过）

**预案**:
- **回滚**: 逐步迁移，每文件一个 commit，可单独 revert
- **降级**: 若某处无法迁移（如 C 回调），用 `std::shared_ptr` + 自定义 deleter

---

### Step 6: 移除旧网络线程 + 清理 (Week 6, 3 天)

**目标**: 删除 Legacy 网络代码，统一到 Qt

**删除文件**:
```
src/lib/net/SocketMultiplexer.cpp/h        # ~318 行
src/lib/net/ISocketMultiplexerJob.h        # ~30 行
src/lib/net/TSocketMultiplexerMethodJob.h  # ~40 行
src/lib/net/TCPSocketFactory.cpp/h         # ~120 行
src/lib/net/TCPSocket.cpp/h                # ~350 行
src/lib/net/SecureSocket.cpp/h             # ~400 行
src/lib/net/TCPListenSocket.cpp/h          # ~200 行
src/lib/net/SecureListenSocket.cpp/h       # ~250 行
```

**预计删减**: ~1,700 行

**迁移检查清单**:
- [ ] 所有 `#include "net/TCPSocket.h"` → `#include "net/QtTcpTransport.h"`
- [ ] 所有 `#include "net/SecureSocket.h"` → `#include "net/QtTlsTransport.h"`
- [ ] 所有 `SocketMultiplexer *` 参数移除
- [ ] 所有 `ISocketMultiplexerJob` 引用移除
- [ ] CMakeLists.txt 更新

**验收**:
- [ ] 编译通过，0 警告
- [ ] 所有现有测试通过
- [ ] 手动测试：Server/Client 连接、键鼠共享、剪贴板同步

**预案**:
- **回滚**: 保留 `LEGACY_NETWORK` CMake 选项，可一键切换
- **降级**: 若 Qt 事件循环与 `IEventQueue` 冲突，保留桥接层

---

## 4. 风险矩阵

| 风险 | 概率 | 影响 | 预案 | 触发条件 |
|------|------|------|------|----------|
| Qt 事件循环与现有 `IEventQueue` 冲突 | High | High | 用 `QTimer` 桥接，保留 `IEventQueue` 接口 | 编译错误 / 运行时死锁 |
| Qt TLS 与 OpenSSL 版本不兼容 | Medium | High | 保留 OpenSSL 作为 fallback，`#ifdef` 切换 | TLS 握手失败 |
| 背压导致心跳超时 | Medium | High | 心跳走独立高优先级通道 | 心跳延迟 > 5s |
| 指纹验证逻辑迁移遗漏 | Low | High | 完整移植 `verifyCertFingerprint` + 单测 | 指纹不匹配时连接成功 |
| 性能回退（Qt 比 raw socket 慢） | Medium | Medium | 基准测试对比，必要时用 `QTcpSocket` 直接操作 | 吞吐量下降 > 20% |

---

## 5. 回滚策略

### 5.1 代码级回滚
- 每个 Step 独立 commit，可单独 revert
- 保留 `LEGACY_NETWORK` CMake 选项：
  ```cmake
  option(LEGACY_NETWORK "Use legacy network stack" OFF)
  if(LEGACY_NETWORK)
    add_definitions(-DUSE_LEGACY_NETWORK)
  endif()
  ```

### 5.2 运行时回滚
- `NetworkTransportFactory` 支持运行时切换：
  ```cpp
  if (qEnvironmentVariableIsSet("USE_LEGACY_NETWORK")) {
      return std::make_unique<LegacyNetworkTransport>(...);
  } else {
      return std::make_unique<QtNetworkTransport>(...);
  }
  ```

### 5.3 发布级回滚
- Phase 2 完成后保留 2 周灰度期
- 灰度期间 `USE_LEGACY_NETWORK=1` 可一键回退
- 灰度期满后移除 Legacy 代码

---

## 6. 验收标准总表

| 维度 | 指标 | 验证方式 |
|------|------|----------|
| **编译** | 0 警告，0 错误 | `-Werror` CI |
| **内存** | 0 泄漏 | ASAN 24h |
| **并发** | 0 数据竞争 | TSAN 24h |
| **TLS** | 1.3 握手成功 | Wireshark |
| **背压** | 慢消费者内存 < 50MB | 压力测试 |
| **性能** | 吞吐量不回退 > 20% | 基准对比 |
| **删除** | 代码删减 ≥ 1,500 行 | `git diff --stat` |
| **兼容** | Server/Client 全功能正常 | 手动测试 |

---

## 7. 时间线

```
Week 3: Step 1 (并发重构) + Step 2 (抽象层骨架)
Week 4: Step 3 (QtTcpTransport) + Step 4 (QtTlsTransport) 开始
Week 5: Step 4 完成 + Step 5 (智能指针化)
Week 6: Step 6 (移除旧代码) + 集成测试 + 文档更新
```

---

> **下一步**: 审核本方案，确认后启动 Step 1 (SocketMultiplexer 并发重构)。
