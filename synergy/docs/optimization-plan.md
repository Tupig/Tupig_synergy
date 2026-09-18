# TuPig Synergy 代码库全面优化实施计划书

> **文档版本**：v1.0  
> **编制日期**：2026-09-18  
> **编制人**：Sisyphus (AI Agent)  
> **审核状态**：待审核  
> **保密等级**：内部公开  

---

## 1. 执行摘要

### 1.1 项目背景
TuPig Synergy 是基于 Synergy/Deskflow 的跨平台键鼠共享工具，代码库约 15 万行 C++，包含网络、输入、协议、平台抽象、GUI 等核心模块。经过系统性代码审计，发现 **23 个关键问题** 涉及安全、架构、性能、可维护性等维度。

### 1.2 优化目标
| 维度 | 现状 | 目标 |
|------|------|------|
| **安全性** | TLS 验证缺失、协议解析不安全、无撤销检查 | 零信任网络、全链路类型安全、实时撤销感知 |
| **架构** | `Server.cpp` 2071行、`Config.cpp` 2074行、自研网络栈 | 模块化 Server、类型安全协议、QtNetwork 统一网络栈 |
| **性能** | 1s poll 延迟、4MB 栈缓冲、逐字节解析 | 零拷贝、异步 I/O、零拷贝解析、心跳隔离 |
| **部署** | 用户需自装 Qt6/OpenSSL | 零依赖绿色版、三平台原生安装包、自动证书管理 |
| **可维护性** | 单测<30%、无静态分析、技术债累积 | 单测≥85%、CI门禁、技术债分期偿还 |

### 1.3 资源投入
| 角色 | 数量 | 关键技能 |
|------|------|----------|
| 核心网络/安全工程师 | 2 | C++20、epoll/IOCP/kqueue、TLS、并发编程 |
| 平台工程师 | 2 | X11/Win32/Cocoa、Raw Input、Keychain/Secure Enclave |
| 架构师/Tech Lead | 1 | 技术决策、代码评审、技术债优先级 |
| QA/自动化 | 1 | 压测框架、故障注入、CI/CD |

### 1.4 工期总览
**总工期：12 周 (3 个月)**

| Phase | 周次 | 核心主题 | 关键交付 |
|-------|------|----------|----------|
| **Phase 0** | Week 0 | 基础设施就绪 | CI/CD、静态分析、基准测试 |
| **Phase 1** | Week 1-2 | **安全基线 (P0)** | TLS验证、协议限制、类型安全、输入验证、X11降级、证书管理 |
| **Phase 2** | Week 3-6 | **网络栈迁移 (P0)** | QtNetwork 迁移、SocketMultiplexer 重构、缓冲区定容量 |
| **Phase 3** | Week 5-8 | **Server 拆分 (P0)** | 7大模块拆分、Server Facade 精简 |
| **Phase 4** | Week 9-10 | **性能优化 (P1)** | 心跳隔离、异步 I/O、零拷贝、剪贴板优化 |
| **Phase 5** | Week 11 | **部署打包** | 三平台原生安装包、绿色版、LGPL 合规 |
| **Phase 6** | Week 12 | **验收交付 + 技术债** | 全维度验收、技术债偿还计划 |

---

## 2. 问题全景图：23 个关键问题全景

### 2.1 按优先级分布
| 优先级 | 数量 | 占比 |
|--------|------|------|
| **P0 严重安全** | 5 | 22% |
| **P1 高危质量** | 7 | 30% |
| **P2 性能瓶颈** | 5 | 22% |
| **P3 技术债** | 6 | 26% |

### 2.2 问题完整清单

| 编号 | 优先级 | 问题描述 | 核心文件 | 映射策略 |
|------|--------|----------|----------|----------|
| **S-1** | **P0** | TLS 证书验证被禁用 (`verifyIgnoreCertCallback` 强制返回 1) | `SecureSocket.cpp:45-48` | S-1 |
| **S-2** | **P0** | 协议消息长度限制过大 (4MB/1M) | `ProtocolTypes.h:120` | S-2 |
| **S-3** | **P0** | 协议解析使用 `va_list` 无类型安全 | `ProtocolUtil.cpp` 全文 | S-3 |
| **S-4** | **P0** | 输入事件注入无验证 | `KeyState.cpp` 等三平台 | S-4 |
| **S-5** | **P0** | X11 错误处理器导致崩溃 | `XWindowsScreen.cpp:99` | S-5 |
| **Q-1** | **P1** | SocketMultiplexer 死锁风险 (双层锁) | `SocketMultiplexer.cpp:263-297` | Q-1 |
| **Q-2** | **P1** | 4MB 栈上静态缓冲区 | `TCPSocket.cpp:24` / `SecureSocket.cpp:37` | Q-2 |
| **Q-3** | **P1** | 协议版本硬编码 | `ProtocolTypes.h:37-48` | Phase 3 Step 1 |
| **Q-4** | **P1** | X11 全局状态 `s_screen` 单例 | `XWindowsScreen.cpp:83` | 合并 S-5 |
| **Q-5** | **P1** | `ProtocolUtil.cpp` 使用 `assert` 作错误处理 | 多处 | 合并 S-3 |
| **Q-6** | **P1** | 单测覆盖 <30% | 全核心模块 | 全 Phase 强制 |
| **Q-7** | **P1** | 静态分析/编译警告清理缺失 | 全项目 | Phase 0 |
| **P-1** | **P2** | SocketMultiplexer 1秒轮询超时 | `SocketMultiplexer.cpp:170` | P-2 (异步 I/O) |
| **P-2** | **P2** | 剪贴板全量内存拷贝 | `Clipboard.cpp` / `XWindowsClipboard.cpp` | Phase 4 |
| **P-3** | **P2** | 协议解析逐字节处理 | `ProtocolUtil.cpp` | S-3.3 (流式解析) |
| **P-4** | **P2** | Windows Hook 无条件加载 | `MSWindowsScreen.cpp:98` | Phase 3 Step 5 |
| **P-5** | **P2** | 剪贴板格式转换器重复造轮子 | 30+ 文件 | Phase 3 Step 6 |
| **T-1** | **P3** | C++17 风格遗留 → C++20 | 全局 | 长期 |
| **T-2** | **P3** | Qt 信号槽旧语法 | GUI 模块 | Phase 3 Step 8 |
| **T-3** | **P3** | Raw 指针手动内存管理 | 网络/协议层 | 全程智能指针化 |
| **T-4** | **P3** | 平台层代码重复 | win32/linux/macos | Phase 3 抽象接口 |
| **T-5** | **P3** | 缺乏单元测试覆盖 | 核心模块 <30% | 全 Phase 强制门禁 |
| **T-6** | **P3** | 构建系统碎片化 | vcpkg + 系统 Qt 混用 | Phase 0 |

---

## 3. 实施路线图：6 个 Phase，12 周

### Phase 0: 基础设施就绪 (Week 0, 2天) — **先行依赖**

| 任务 ID | 任务内容 | 对应问题 | 产出物 | 验收标准 |
|---------|----------|----------|--------|----------|
| **T-0.1** | CI 配置：`-Werror -Wall -Wextra -Wpedantic` | Q-7, T-6 | `.github/workflows/ci.yml` | 编译 0 警告 |
| **T-0.2** | 启用 `clang-tidy` / `cppcheck` (如可用) | Q-7 | CI 配置 | 静态分析 0 high |
| **T-0.3** | 配置 `CMakePresets.json` + `vcpkg` manifest 模式 | T-6 | `CMakePresets.json`、`vcpkg.json` | 统一构建配置 |
| **T-0.4** | 建立基准测试：TLS 握手/内存/启动时间 | 所有性能项 | `benchmark/` 目录 | 基线数据 |
| **T-0.5** | 创建分支 + 目录结构 | 所有 | `git checkout -b refactor/security-baseline` | 分支就绪 |

---

### Phase 1: 安全基线 (Week 1-2) — **P0 必须完成**

#### Week 1: 协议层安全加固

| 任务 ID | 任务内容 | 对应问题 | 核心文件变更 | 验收标准 |
|---------|----------|----------|--------------|----------|
| **S-1.1** | 移除 `verifyIgnoreCertCallback` 强制返回 1 | S-1 | `SecureSocket.cpp` 删除 45-48 行 | 编译通过 |
| **S-1.2** | 实现 `verifyCertificateCallback` 完整验证 | S-1 | `SecureSocket.cpp` 新增回调 | 证书链/主机名/有效期/密钥长度≥2048 全验证 |
| **S-1.3** | 集成 `CertManager` 自动加载/生成证书 | S-1 | `SecureSocket.cpp` + `CertManager` | 首运行自动生成证书、TLS 监听成功 |
| **S-1.4** | 支持 `checkPeerFingerprints` 配置项 | S-1 | `Settings.h` + `SecureSocket.cpp` | 指纹不匹配拒绝连接 |
| **S-1.5** | 单测：正常/过期/自签名/错误主机名/指纹不匹配 | S-1 | `SecureSocketTest.cpp` | 5 用例全通过 |
| **S-2.1** | 定义 `MessageSizeLimit` 分级枚举 | S-2 | `ProtocolTypes.h` | 编译通过 |
| **S-2.2** | `ProtocolUtil::readf` 解析前检查长度 | S-2 | `ProtocolUtil.cpp` | 超限消息拒绝、日志记录 |
| **S-2.3** | 大消息类型实现流式读取 (避免全量缓冲) | S-2, P-3 | `ProtocolUtil.cpp` | 32MB 剪贴板/文件流式处理 |
| **S-2.4** | 单测：边界值/超限拒绝/分块重组 | S-2 | `ProtocolUtilTest.cpp` | 100% 覆盖 |

#### Week 1 Day 3 - Week 2 Day 2: 协议解析类型安全重构 (S-3)

| 任务 ID | 任务内容 | 对应问题 | 产出文件 | 验收 |
|---------|----------|----------|----------|------|
| **S-3.1** | 定义 `ProtocolMessage` concept + 字段反射宏 | S-3 | `ProtocolTypes.h` | Concept 约束编译通过 |
| **S-3.2** | 迁移 50+ 消息为结构体 + Concept 约束 | S-3 | `ProtocolTypes.h` | 所有消息结构体化 |
| **S-3.3** | 实现 `Serializer<T>` 模板引擎 (零拷贝) | S-3, P-3 | `ProtocolSerializer.h` | 编译期展开、零拷贝、逐字节→流式 |
| **S-3.4** | 逐个替换 `ProtocolUtil::readf/writef` 调用点 | S-3, Q-5 | 全项目 | 编译期类型安全、`assert`→错误码 |
| **S-3.5** | 编译验证 + 性能基准对比 | S-3 | CI | 编译通过、性能不回退 |
| **S-3.6** | 替换所有 `assert(0)` 为错误码/异常 | Q-5 | `ProtocolUtil.cpp` 等 | Release 构建不崩溃 |

#### Week 2 Day 3-5: 输入验证 + X11 降级 + 证书管理

| 任务 ID | 任务内容 | 对应问题 | 产出文件 | 验收 |
|---------|----------|----------|----------|------|
| **S-4.1** | 实现 `InputValidator` 核心 (范围/频率/敏感键) | S-4 | `InputValidator.h/.cpp` 新增 | 编译通过 |
| **S-4.2** | 集成到 3 平台 `KeyState` 实现 | S-4 | `KeyState.cpp`/`MSWindowsKeyState.cpp`/`XWindowsKeyState.cpp` | 编译通过 |
| **S-4.3** | Windows: Raw Input 替代 Hook 敏感键拦截 | S-4, P-4 | `MSWindowsKeyState.cpp` | 敏感组合拦截生效 |
| **S-4.4** | 单测：频率限制/敏感组合/合法输入 | S-4 | `InputValidatorTest.cpp` | 100% 覆盖 |
| **S-5.1** | `ioErrorHandler` 仅标记状态+发事件 | S-5 | `XWindowsScreen.cpp` | 不退出进程 |
| **S-5.2** | `cleanupXResources()` 释放 X11 资源 | S-5 | `XWindowsScreen.cpp` | 无泄漏 |
| **S-5.3** | 指数退避重连 (1s,2s,4s...max 60s) | S-5 | `XWindowsScreen.cpp` | 断网自动重连 |
| **S-5.4** | GUI 监听 `DisplayLost` 更新托盘 | S-5 | `MainWindow.cpp` | 托盘变灰+提示 |
| **S-5.5** | 移除 `s_screen` 单例 → 依赖注入 | Q-4 | `XWindowsScreen.cpp` | 多实例支持、测试友好 |

#### Phase 1 交付物：证书管理系统 (支撑 S-1)

| 模块 | 文件 | 核心能力 |
|------|------|----------|
| **ICertProvider** | `ICertProvider.h` | 抽象接口 |
| **CertManager** | `CertManager.h/.cpp` | 统一入口、轮换、TOFU |
| **CertConfig/Bundle** | `CertConfig.h`、`CertBundle.h` | 配置/数据结构 |
| **WindowsProvider** | `windows/WindowsCertProvider.h/.cpp` | 证书存储+TPM+Root存储 |
| **MacOSProvider** | `macos/MacOSKeychainProvider.h/.cpp` | Keychain+Secure Enclave+System Roots |
| **LinuxProvider** | `linux/OpenSSLFileProvider.h/.cpp` | 文件存储+systemd |
| **RevocationChecker** | `RevocationChecker.h/.cpp` | CRL/OCSP/Stapling/缓存 |

**Phase 1 验收门槛**：
- [ ] MITM 攻击被拦截 (Wireshark 验证)
- [ ] 协议消息超限自动拒绝 (日志记录)
- [ ] `va_list` 全清零，编译期类型安全 (编译通过)
- [ ] 敏感组合键拦截、频率限制生效 (单测通过)
- [ ] X11 断开不崩溃、自动重连 (手动测试)
- [ ] 证书自动生成/轮换/TOFU 全流程跑通

---

### Phase 2: QtNetwork 迁移 (Week 3-6) — **P0**

| 周次 | 核心任务 | 对应问题 | 关键产出 | 验收 |
|------|----------|----------|----------|------|
| **Week 3** | Q-1.1~Q-1.5 SocketMultiplexer 并发重构 | Q-1 | `SocketMultiplexer.cpp/h` 单 mutex+cv | 死锁风险归零、TSAN 24h 0 报警 |
| **Week 3** | QtNetwork 抽象层 + 双实现骨架 | 迁移基础 | `INetworkTransport`、`QtNetworkTransport`、`LegacyNetworkTransport`、工厂 | 双实现运行时切换 |
| **Week 3** | QtHttpClient 替代 REST 客户端 | 隐含 | `QtHttpClient.cpp/h` | HTTP/2、重定向、取消 |
| **Week 4** | **Q-2.1~Q-2.5** 缓冲区定容量+背压 | Q-2 | `QtTcpTransport`、`QtTlsTransport` 缓冲区定容量+背压 | 慢消费者不 OOM、内存<50MB |
| **Week 4** | QtTlsTransport 替代 SecureSocket | S-1 迁移 | `QtTlsTransport.cpp/h`、TLS 1.3/OCSP Stapling/指纹验证 | TLS 1.3/OCSP Stapling/指纹验证通过 |
| **Week 5** | QtTcpTransport 替代 TCPSocket | 隐含 | `QtTcpTransport.cpp/h`、Happy Eyeballs、背压 | 连接/收发/背压/重连 |
| **Week 5** | **T-3** 网络层 Raw 指针→智能指针 | T-3 | 网络层全面 `unique_ptr`/`shared_ptr` | 无 Raw owning 指针 |
| **Week 6** | 移除 SocketMultiplexer 线程、统一事件循环 | Q-1 收尾 | 删除 `SocketMultiplexer*` 等文件 | 无单独网络线程 |
| **Week 6** | **T-3** 协议层 Raw 指针→智能指针 | T-3 | 协议层智能指针化 | 无 Raw owning 指针 |

**Phase 2 删除文件**：`SocketMultiplexer.cpp/h`、`ISocketMultiplexerJob.h`、`TSocketMultiplexerMethodJob.h`、`TCPSocketFactory.cpp/h`、`TCPSocket.cpp/h`、`SecureSocket.cpp/h`、`HttpClient.cpp/h` 等 ~12 个文件，~3,200 行

**Phase 2 验收**：
- [ ] 代码删减 ≥ 2,000 行
- [ ] TLS 1.3 握手成功 (Wireshark)
- [ ] OCSP Stapling 服务端/客户端通过
- [ ] HTTP/2 `h2` 协商成功
- [ ] Happy Eyeballs IPv6 优先 200ms 回退
- [ ] 背压：10MB/s 发、1MB/s 收，内存 < 50MB
- [ ] 编译 0 警告、TSAN 24h 0 报警

---

### Phase 3: Server 拆分 (Week 5-8) — **P0**

#### 接口优先设计 (Week 5 Day 1-2)

| 接口文件 | 核心职责 |
|----------|----------|
| `IConfigRepository.h` | 配置持久化/Schema迁移/热更通知 |
| `IConnectionManager.h` | 连接生命周期/客户端代理池/心跳/重连 |
| `IScreenManager.h` | 屏幕拓扑/别名/链路/选项/热键 |
| `IInputDispatcher.h` | 键鼠分发/修饰键同步/热键触发 |
| `IClipboardManager.h` | 剪贴板同步/格式转换/分块传输 |
| `IHeartbeatManager.h` | 心跳发送/接收/超时检测/统计 |
| `IIpcGateway.h` | GUI↔Core IPC、Daemon 管理 |

#### 拆分步骤 (按依赖顺序)

| Step | 周次 | 任务 | 对应问题 | 产出文件 | 验收 |
|------|------|------|----------|----------|------|
| **Step 1** | Week 5 | ConfigRepository | Q-3(版本硬编码)、Q-5(assert)、Q-6 | `IConfigRepository`、`FileConfigRepository`、`ConfigSnapshot`、版本自动生成、`assert`→错误码、单测≥85% |
| **Step 2** | Week 6 Day 1-2 | HeartbeatManager | P-1(心跳隔离) | `IHeartbeatManager`、`HeartbeatManager` (专用线程) | 专用线程、精确定时 |
| **Step 3** | Week 6 Day 3-5 | ConnectionManager | Q-6 | `IConnectionManager`、`ClientProxy`、单测≥85% | 连接/重连/心跳全覆盖 |
| **Step 4** | Week 7 Day 1-3 | ScreenManager | Q-4(单例)、P-5(转换器重复)、T-4 | `IScreenManager`、`ScreenManager`、平台抽象接口、转换器基类、单测≥85% | 拓扑/热键/平台抽象覆盖 |
| **Step 5** | Week 7 Day 4-5 | InputDispatcher | P-4(惰性Hook)、S-4(输入验证) | `IInputDispatcher`、`InputDispatcher`、`HotkeyManager`、Hook 惰性加载、Raw Input | 惰性加载、验证集成 |
| **Step 6** | Week 8 Day 1-2 | ClipboardManager | P-2(零拷贝)、P-5(转换器)、T-4 | `IClipboardManager`、`ClipboardManager`、`ClipboardChunkProcessor`、零拷贝/分块/基类 | 零拷贝/分块/基类覆盖 |
| **Step 7** | Week 8 Day 3-4 | IpcGateway | 隐含 | `IIpcGateway`、`IpcGateway`、`DaemonManager` | IPC/Daemon 覆盖 |
| **Step 8** | Week 8 Day 5 | Server Facade 精简 | Q-6、T-2 | `Server.cpp` <300行、Qt5→Qt6 信号槽、全组件单测≥85% | <300行、现代语法、全覆盖 |

**Phase 3 代码行数目标**：
| 文件 | 现状 | 目标 | 删减率 |
|------|------|------|--------|
| `Server.cpp` | 2,071 行 | < 300 行 | 85%+ |
| `Config.cpp` | 2,074 行 | < 300 行 (仅 Schema) | 85%+ |
| 新增接口 | 0 | 7 个 `I*.h` | — |
| 新增实现 | 0 | 7 个 `*Manager.cpp` | — |

---

### Phase 4: 性能优化 (Week 9-10) — **P1**

| 任务 | 对应问题 | 核心产出 | 验收指标 |
|------|----------|----------|----------|
| **P-1** 心跳线程隔离 | P-1 | `HeartbeatManager` 专用线程、`steady_clock` 精确定时 | 大传输时心跳不延迟、不误判 |
| **P-2** 异步 I/O 重构 (可选长期) | P-1 | `AsyncBackend` + `EpollBackend`/`IOCPBackend`/`KqueueBackend` | 10k 连接、CPU 降 50%+ |
| **P-2** 剪贴板零拷贝/分块流式/压缩 | P-2 | `ClipboardChunkProcessor` 共享内存/分块/压缩 | 大文件不 OOM、延迟降低 |
| **S-3.3** 协议零拷贝解析 | P-3 | `ProtocolUtil` `std::span` 零拷贝解析 | CPU 开销降 50%+ |
| **P-4** Windows Hook 惰性加载 | P-4 | `MSWindowsScreen.cpp` 惰性加载、Raw Input | 启动快、资源省 |
| **T-1** C++20 迁移启动 | T-1 | `std::expected`/`std::format`/`concepts`/`std::span` 迁移启动 | 关键路径现代化 |
| **T-3** 智能指针全覆盖完成 | T-3 | 全代码库 Raw→智能指针 | 0 Raw owning 指针 |

---

### Phase 5: 部署打包 (Week 11) — **P1**

| 平台 | 交付物 | 关键工具 | 体积目标 | 关键验收 |
|------|--------|----------|----------|----------|
| **Windows** | `TuPig-Synergy-1.21.2-windows-x64.exe` (NSIS) | `windeployqt` + NSIS | ~45 MB | 安装包双击运行/便携包解压即用/签名验证/卸载干净 |
| | `TuPig-Synergy-1.21.2-portable.zip` | `windeployqt` + zip | ~40 MB | 解压即用、无管理员权限 |
| **macOS** | `TuPig-Synergy-1.21.2-macos-universal.dmg` | `macdeployqt` + `create-dmg` + 公证 | ~52 MB | DMG拖拽安装/Gatekeeper通过/公证通过/签名验证 |
| **Linux** | `TuPig-Synergy-1.21.2-linux-x86_64.AppImage` | `linuxdeployqt` + AppImage | ~38 MB | 双击运行/无系统Qt依赖/Wayland+X11兼容 |
| | `tu-pig-synergy_1.21.2_amd64.deb` | `dpkg-deb` | ~35 MB | `apt install` 可安装 |
| **通用** | 便携包/绿色版 | 三平台统一 | — | 解压即用、无管理员权限 |
| **合规** | LGPL/GPL 合规包 | 文档 | — | 动态链接/许可证文件/源码声明/用户可替换 Qt |

---

### Phase 6: 验收交付 + 技术债偿还 (Week 12)

| 验收维度 | 门槛 | 验收方式 |
|----------|------|----------|
| **编译/静态分析** | 0 warning / 0 high | `-Werror` CI + `clang-tidy`/`cppcheck` |
| **单测覆盖率** | ≥ 85% | `ctest --coverage` + `genhtml` |
| **内存/竞争** | 0 leak / 0 race | `valgrind` + `TSAN` 24h |
| **TLS 1.3/OCSP/HTTP/2/WebSocket** | 全通过 | Wireshark + `autobahn-testsuite` |
| **三平台安装/运行/卸载** | 全通过 | 矩阵验收清单 |
| **文档完整度** | 15+ 份 Markdown | 目录检查 |
| **技术债偿还计划** | 文档化 + 里程碑 | 长期跟踪 Issue |

---

## 4. 风险总览与应急预案

| 等级 | 风险点 | 触发条件 | 应急预案 |
|------|--------|----------|----------|
| **Critical** | TLS 重构破坏现有连接 | 发布后用户无法连接 | 保留兼容模式配置开关 `USE_LEGACY_TLS=ON`，灰度发布 2 周 |
| **Critical** | QtNetwork 迁移破坏网络 | 网络层回归 | 运行时开关 `USE_QT_NETWORK=OFF` 回滚到 Legacy |
| **High** | 异步 I/O 重构引入数据竞争 | TSAN 未覆盖路径 | 回滚到 Qt 事件循环模式，保留回滚分支 |
| **High** | 缓冲区背压导致心跳超时 | 大文件传输 + 慢网络 | 心跳走独立高优先级通道 (P-1) |
| **High** | Server 拆分引入数据竞争 | TSAN 未覆盖路径 | 明确线程归属 + TSAN 24h + 组件级压测 |
| **Medium** | C++20 迁移编译失败 | 旧编译器不支持 | 保留 C++17 兼容分支，分阶段迁移 |
| **Medium** | 平台后端维护成本超预期 | 人员流失/平台变更 | 优先维护 Linux/Windows，macOS 后补 |

---

## 5. 成功度量指标 (KPI)

| 指标类别 | 指标 | 基线 | 目标 |
|----------|------|------|------|
| **安全** | 高危漏洞数 | 5 | 0 |
| **安全** | TLS 1.3 握手成功率 | 0% | 100% |
| **安全** | OCSP Stapling 覆盖率 | 0% | 100% |
| **性能** | P99 延迟 (LAN) | ~50ms | < 10ms |
| **性能** | 空闲内存 | ~80MB | < 30MB |
| **性能** | 10k 连接 CPU 占用 | 基线 | 降 50%+ |
| **质量** | 编译警告 | 数十个 | 0 |
| **质量** | 单测覆盖率 | <30% | ≥ 85% |
| **质量** | 静态分析 high 级 | N/A | 0 |
| **可靠性** | TSAN 数据竞争 | N/A | 0 |
| **部署** | Windows 安装包体积 | ~60MB | < 50MB |
| **部署** | macOS 公证通过率 | 0% | 100% |
| **维护性** | Server.cpp 行数 | 2,071 | < 300 |
| **维护性** | Config.cpp 行数 | 2,074 | < 300 |
| **维护性** | 单测覆盖率 | <30% | ≥ 85% |

---

## 6. 签署与生效

| 角色 | 姓名 | 签名 | 日期 |
|------|------|------|------|
| **项目发起人** | | | |
| **技术负责人** | | | |
| **架构师** | | | |

---

> **下一步**：确认无异议后，立即创建 `refactor/security-baseline` 分支，启动 Phase 0 基础设施搭建。
