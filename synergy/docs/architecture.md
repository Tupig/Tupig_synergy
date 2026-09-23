# Architecture Decision Records / 架构决策记录

> **Language / 语言**: [English](#english) | [中文](#中文)

---

## English

### ADR Template

Each ADR follows this structure:

```markdown
# ADR-XXXX: Title

**Status**: Proposed | Accepted | Superseded | Deprecated
**Date**: YYYY-MM-DD
**Deciders**: @username1, @username2
**Technical Story**: Link to issue/PR

## Context
What problem are we solving? What constraints exist?

## Decision
What did we decide? What is the chosen approach?

## Consequences
### Positive
- Benefit 1
- Benefit 2

### Negative
- Trade-off 1
- Trade-off 2

### Neutral
- Consideration 1
```

---

### ADR-0001: Qt 6 as GUI Framework

**Status**: Accepted
**Date**: 2024-01-15
**Deciders**: @tupig-team
**Technical Story**: #1

#### Context
Need cross-platform GUI framework supporting Windows, macOS, Linux (X11/Wayland). Requirements:
- Native look/feel on all platforms
- High DPI support
- Mature ecosystem, long-term support
- C++17/20 compatible

#### Decision
Adopt **Qt 6** (minimum 6.7) as the sole GUI framework.

#### Consequences
**Positive:**
- Excellent cross-platform abstraction
- Native HiDPI on Windows/macOS/Linux
- Rich widget set, QML optional
- Commercial support available
- Qt 6 modernized (C++17, RHI, CMake)

**Negative:**
- Large dependency (~50MB)
- Licensing complexity (LGPL v3 / Commercial)
- Build system complexity (CMake + Qt)

**Neutral:**
- Qt 5 migration path exists but not needed

---

### ADR-0002: CMake 3.24+ as Build System

**Status**: Accepted
**Date**: 2024-01-15
**Deciders**: @tupig-team
**Technical Story**: #2

#### Context
Need build system supporting:
- Multi-platform (Win/macOS/Linux)
- Qt 6 integration
- vcpkg / Conan / system packages
- Modern C++ (modules, presets)
- Cross-compilation

#### Decision
Standardize on **CMake 3.24+** with presets (`CMakePresets.json`).

#### Consequences
**Positive:**
- Native Qt 6 support via `find_package(Qt6)`
- Presets enable CI/CD and IDE integration
- vcpkg toolchain integration
- Ninja/Generator Expressions for speed
- Industry standard

**Negative:**
- CMake learning curve
- Complex cross-platform logic

**Neutral:**
- Alternative: Meson (not mature for Qt)

---

### ADR-0003: C++20 as Language Standard

**Status**: Accepted
**Date**: 2024-01-15
**Deciders**: @tupig-team
**Technical Story**: #3

#### Context
Modern C++ features improve safety, expressiveness, performance.

#### Decision
Require **C++20** minimum. Use: concepts, ranges, coroutines, modules (when available), `std::format`, `std::expected`, `std::span`, concepts.

#### Consequences
**Positive:**
- Concepts for template constraints
- `std::format` type-safe formatting
- `std::expected` for error handling
- `std::span` for non-owning views
- Modules reduce compile times (future)

**Negative:**
- Compiler support: MSVC 19.35+, GCC 11+, Clang 14+
- Some features not in all stdlibs yet

**Neutral:**
- C++23 features adopted incrementally

---

### ADR-0004: OpenSSL 3.0 for TLS

**Status**: Accepted
**Date**: 2024-01-15
**Deciders**: @tupig-team
**Technical Story**: #4

#### Context
Protocol v1.4+ requires TLS. Need:
- TLS 1.2/1.3 support
- Certificate validation
- FIPS 140-3 option
- Active maintenance

#### Decision
Use **OpenSSL 3.0+** via `SecureSocket` wrapper.

#### Consequences
**Positive:**
- TLS 1.3 native
- Provider architecture (FIPS, legacy)
- Active LTS (3.0, 3.1, 3.2)
- Apache 2.0 license

**Negative:**
- API breaking from 1.1.1
- Larger binary
- Provider loading complexity

**Neutral:**
- Alternative: mbedTLS (smaller, less features)

---

### ADR-0005: Protocol v1.8 as Wire Format

**Status**: Accepted
**Date**: 2024-06-01
**Deciders**: @tupig-team
**Technical Story**: #42

#### Context
Synergy protocol evolved over 20 years. Need to maintain compatibility while adding features.

#### Decision
Implement **Protocol v1.8** as baseline. Support negotiation down to v1.0.

Key v1.8 additions:
- `LSYN` (Language Synchronisation)
- Extended key codes
- Improved clipboard streaming

#### Consequences
**Positive:**
- Backward compatible with Deskflow/Synergy clients
- Feature negotiation via Hello/HelloBack
- Extensible message format

**Negative:**
- Legacy message variants (v1.0 key codes)
- Complex state machine

**Neutral:**
- Protocol spec in `ProtocolTypes.h`

---

### ADR-0006: License-Free Distribution Model

**Status**: Accepted
**Date**: 2024-03-01
**Deciders**: @tupig-team
**Technical Story**: #10

#### Context
Original Synergy requires serial key activation. Goal: remove all licensing barriers.

#### Decision
- Remove all license validation code (`LicenseHandler`, `ActivationDialog`)
- Disable activation enforcement (return success)
- Remove auto-update checks
- Keep GPL v2 license for source

#### Consequences
**Positive:**
- Zero-friction adoption
- No activation servers needed
- Community trust
- Simplified codebase

**Negative:**
- No commercial differentiation
- Fork maintenance burden

**Neutral:**
- GPL v2 requires source distribution

---

### ADR-0007: Platform Abstraction Layer (PAL)

**Status**: Accepted
**Date**: 2024-01-15
**Deciders**: @tupig-team
**Technical Story**: #5

#### Context
Core logic must be platform-agnostic. Windows/macOS/Linux have vastly different APIs for:
- Input injection
- Clipboard
- Screen enumeration
- Global hotkeys
- Secure desktop (Windows UAC)

#### Decision
Implement **Platform Abstraction Layer** in `src/lib/platform/` with per-platform implementations:
- `platform/win32/` — Win32 API, Raw Input, UAC
- `platform/macos/` — Cocoa, CGEvent, Accessibility
- `platform/linux/` — X11, Wayland, libei, Portal

Interface defined in `src/lib/arch/`.

#### Consequences
**Positive:**
- Core logic 100% platform-independent
- Testable with mock platforms
- Clear separation of concerns
- New platforms: implement interface only

**Negative:**
- Abstraction overhead (minimal)
- Platform-specific bugs isolated but harder to debug

**Neutral:**
- ~114 files in platform layer

---

### ADR-0008: Event-Driven Architecture with Mutex-Guarded Event Queues

**Status**: Accepted (revised 2026-09-23)
**Date**: 2024-01-15
**Deciders**: @tupig-team
**Technical Story**: #6

#### Context
Input events (mouse/keyboard) must be processed with minimal latency. Threading model:
- Network thread: receives protocol messages
- Input thread: synthesizes OS events
- GUI thread: Qt event loop

#### Decision
Inter-thread communication goes through the event queue in `src/lib/base/`
(`IEventQueue` / `EventQueue`), which is guarded by a **mutex and a condition
variable** (`mt/Mutex.h`, `mt/CondVar.h`). The `src/lib/mt/` library supplies
those primitives plus thread wrappers; it holds **no** lock-free or atomic-based
queue.

> **Correction (2026-09-23).** This ADR originally claimed "lock-free MPSC
> queues (`src/lib/mt/`)". No such implementation exists. `src/lib/mt/` contains
> only `Mutex`, `CondVar`, `Lock` and `Thread`; `EventQueue.h:121-122` declares
> `std::unique_ptr<Mutex> m_readyMutex` and `std::unique_ptr<CondVar<bool>>
> m_readyCondVar`; and no file in the tree uses `std::atomic` or
> `std::memory_order`. The text below records what the code actually does. A
> lock-free queue remains a possible future optimisation, not a description of
> the present design.

Event processing pipeline:

```
Network → Decode → Queue → Input Thread → OS Synthesis
                ↓
           GUI Thread (status updates)
```

#### Consequences
**Positive:**
- Straightforward, well-understood synchronisation
- No ABA problem to reason about
- Debuggable with ordinary tooling

**Negative:**
- Contention on the shared queue, so latency is bounded by wake-up cost rather
  than sub-millisecond by construction
- Priority inversion is possible in principle

**Neutral:**
- A lock-free MPSC queue was considered and **not** implemented

---

### ADR-0009: GoogleTest + CTest for Testing

**Status**: Accepted
**Date**: 2024-01-15
**Deciders**: @tupig-team
**Technical Story**: #7

#### Context
Need testing framework supporting:
- Unit tests (C++)
- Integration tests (multi-process)
- Protocol compliance tests
- CI integration

#### Decision
Use **GoogleTest** for C++ unit tests + **CTest** for orchestration. Target >80% coverage.

Structure:
```
src/unittests/
  base/        # String, logging, settings
  common/      # Settings, constants
  deskflow/    # Clipboard, input, protocol
  gui/         # Widgets, dialogs
  net/         # SecureSocket, fingerprints
  platform/    # Platform mocks
```

#### Consequences
**Positive:**
- Rich assertions, death tests, parameterized tests
- CTest: parallel, sharding, labels
- XML/JUnit output for CI
- Mocking via GMock

**Negative:**
- Compile time overhead
- Flaky tests in multi-threaded code

**Neutral:**
- Integration tests in separate target

---

### ADR-0010: vcpkg for Windows Dependencies

**Status**: Accepted
**Date**: 2024-01-15
**Deciders**: @tupig-team
**Technical Story**: #8

#### Context
Windows lacks system package manager. Need consistent Qt, OpenSSL, libraries.

#### Decision
Use **vcpkg** in manifest mode (`vcpkg.json`), pinned by `builtin-baseline`. **All** third-party
dependencies — Qt, OpenSSL and their transitive dependencies — come from vcpkg, statically linked so
each platform produces a self-contained executable. The repository-local vcpkg checkout is
bootstrapped by `scripts/bootstrap-vcpkg.{bat,sh}`; no system Qt is used.

#### Consequences
**Positive:**
- Reproducible builds
- Binary caching
- Version pinning
- CMake toolchain integration

**Negative:**
- Qt compilation time (hours)
- Disk space (~5-10GB)

**Neutral:**
- Alternative: Conan, manual install

---

### ADR-0011: Flat Docs Structure with Bilingual Content

**Status**: Accepted
**Date**: 2024-09-18
**Deciders**: @tupig-team
**Technical Story**: #156

#### Context
Documentation was split across `docs/dev/`, `docs/user/`, with English-only content and CMake build artifacts.

#### Decision
Flatten to `docs/` with bilingual (EN/ZH) markdown files:

```
docs/
  build.md           # 编译指南
  contributing.md    # 贡献指南
  protocol.md        # 协议参考
  configuration.md   # 配置参考
  architecture.md    # 架构决策 (本文件)
  troubleshooting.md # 故障排查
  security.md        # 安全策略
```

Each file: English first, Chinese second, shared diagrams/tables.

#### Consequences
**Positive:**
- Single source of truth
- GitHub renders natively
- No build step for docs
- Contributors edit directly

**Negative:**
- Larger markdown files
- Translation sync manual

**Neutral:**
- Doxygen for API reference (separate)

---

## 中文

### ADR 模板

每个 ADR 遵循以下结构：

```markdown
# ADR-XXXX: 标题

**状态**: 提议中 | 已接受 | 已替代 | 已弃用
**日期**: YYYY-MM-DD
**决策者**: @username1, @username2
**技术背景**: Issue/PR 链接

## 背景
解决什么问题？存在什么约束？

## 决策
做出什么决定？选择什么方案？

## 后果
### 正面
- 收益 1
- 收益 2

### 负面
- 权衡 1
- 权衡 2

### 中性
- 考量 1
```

---

### ADR-0001: Qt 6 作为 GUI 框架

**状态**: 已接受
**日期**: 2024-01-15
**决策者**: @tupig-team
**技术背景**: #1

#### 背景
需要跨平台 GUI 框架支持 Windows、macOS、Linux (X11/Wayland)。要求：
- 全平台原生外观/手感
- 高 DPI 支持
- 生态成熟、长期维护
- C++17/20 兼容

#### 决策
采用 **Qt 6**（最低 6.7）作为唯一 GUI 框架。

#### 后果
**正面：**
- 优秀的跨平台抽象
- Windows/macOS/Linux 原生 HiDPI
- 丰富控件集，可选 QML
- 商业支持可用
- Qt 6 现代化 (C++17, RHI, CMake)

**负面：**
- 依赖体积大 (~50MB)
- 许可证复杂 (LGPL v3 / 商业)
- 构建系统复杂

**中性：**
- Qt 5 迁移路径存在但不需要

---

### ADR-0002: CMake 3.24+ 作为构建系统

**状态**: 已接受
**日期**: 2024-01-15
**决策者**: @tupig-team
**技术背景**: #2

#### 背景
需要支持以下场景的构建系统：
- 多平台 (Win/macOS/Linux)
- Qt 6 集成
- vcpkg / Conan / 系统包
- 现代 C++ (modules, presets)
- 交叉编译

#### 决策
标准化使用 **CMake 3.24+**，配合预设 (`CMakePresets.json`)。

#### 后果
**正面：**
- 原生 Qt 6 支持 via `find_package(Qt6)`
- 预设支持 CI/CD 与 IDE 集成
- vcpkg 工具链集成
- Ninja/生成器表达式提速
- 行业标准

**负面：**
- CMake 学习曲线
- 跨平台逻辑复杂

**中性：**
- 替代方案: Meson (Qt 支持不成熟)

---

### ADR-0003: C++20 作为语言标准

**状态**: 已接受
**日期**: 2024-01-15
**决策者**: @tupig-team
**技术背景**: #3

#### 背景
现代 C++ 特性提升安全性、表达力、性能。

#### 决策
要求 **C++20** 最低标准。使用：concepts、ranges、coroutines、modules(当可用)、`std::format`、`std::expected`、`std::span`、concepts。

#### 后果
**正面：**
- Concepts 约束模板
- `std::format` 类型安全格式化
- `std::expected` 错误处理
- `std::span` 非拥有视图
- Modules 减少编译时间 (未来)

**负面：**
- 编译器要求: MSVC 19.35+, GCC 11+, Clang 14+
- 部分特性标准库未完全实现

**中性：**
- C++23 特性增量采用

---

### ADR-0004: OpenSSL 3.0 用于 TLS

**状态**: 已接受
**日期**: 2024-01-15
**决策者**: @tupig-team
**技术背景**: #4

#### 背景
协议 v1.4+ 要求 TLS。需求：
- TLS 1.2/1.3 支持
- 证书验证
- FIPS 140-3 选项
- 活跃维护

#### 决策
通过 `SecureSocket` 封装使用 **OpenSSL 3.0+**。

#### 后果
**正面：**
- 原生 TLS 1.3
- Provider 架构 (FIPS, legacy)
- 活跃 LTS (3.0, 3.1, 3.2)
- Apache 2.0 许可

**负面：**
- 1.1.1 API 破坏性变更
- 二进制体积增大
- Provider 加载复杂

**中性：**
- 替代方案: mbedTLS (更小但功能少)

---

### ADR-0005: 协议 v1.8 作为线路格式

**状态**: 已接受
**日期**: 2024-06-01
**决策者**: @tupig-team
**技术背景**: #42

#### 背景
Synergy 协议演进 20 年。需在保持兼容的前提下增加特性。

#### 决策
实现 **协议 v1.8** 作为基线。支持向下协商至 v1.0。

v1.8 关键新增：
- `LSYN` (语言同步)
- 扩展键码
- 改进剪贴板流式传输

#### 后果
**正面：**
- 兼容 Deskflow/Synergy 客户端
- Hello/HelloBack 特性协商
- 可扩展消息格式

**负面：**
- 遗留消息变体 (v1.0 键码)
- 复杂状态机

**中性：**
- 协议规范在 `ProtocolTypes.h`

---

### ADR-0006: 免许可证分发模式

**状态**: 已接受
**日期**: 2024-03-01
**决策者**: @tupig-team
**技术背景**: #10

#### 背景
原版 Synergy 需序列号激活。目标：移除所有许可障碍。

#### 决策
- 移除所有许可验证代码 (`LicenseHandler`, `ActivationDialog`)
- 禁用激活强制 (直接返回成功)
- 移除自动更新检查
- 保留 GPL v2 源码许可

#### 后果
**正面：**
- 零摩擦采用
- 无需激活服务器
- 社区信任
- 代码库简化

**负面：**
- 无商业差异化
- Fork 维护负担

**中性：**
- GPL v2 要求源码分发

---

### ADR-0007: 平台抽象层 (PAL)

**状态**: 已接受
**日期**: 2024-01-15
**决策者**: @tupig-team
**技术背景**: #5

#### 背景
核心逻辑必须平台无关。Windows/macOS/Linux 在以下方面 API 差异巨大：
- 输入注入
- 剪贴板
- 屏幕枚举
- 全局热键
- 安全桌面

#### 决策
在 `src/lib/platform/` 实现 **平台抽象层**，各平台独立实现：
- `platform/win32/` — Win32 API, Raw Input, UAC
- `platform/macos/` — Cocoa, CGEvent, Accessibility
- `platform/linux/` — X11, Wayland, libei, Portal

接口定义在 `src/lib/arch/`。

#### 后果
**正面：**
- 核心逻辑 100% 平台无关
- 可用 Mock 平台测试
- 关注点清晰分离
- 新平台仅需实现接口

**负面：**
- 抽象开销 (极小)
- 平台特有 Bug 隔离但难调试

**中性：**
- 平台层约 114 文件

---

### ADR-0008: 基于互斥量保护事件队列的事件驱动架构

**状态**: 已接受（2026-09-23 修订）
**日期**: 2024-01-15
**决策者**: @tupig-team
**技术背景**: #6

#### 背景
输入事件(鼠标/键盘)需极低延迟处理。线程模型：
- 网络线程: 接收协议消息
- 输入线程: 合成 OS 事件
- GUI 线程: Qt 事件循环

#### 决策
线程间通信经由 `src/lib/base/` 中的事件队列（`IEventQueue` / `EventQueue`），由**互斥量 + 条件变量**保护（`mt/Mutex.h`、`mt/CondVar.h`）。`src/lib/mt/` 提供这些原语与线程封装，其中**没有**任何无锁或基于原子操作的队列。

> **更正（2026-09-23）**：本 ADR 原先声称使用「无锁 MPSC 队列（`src/lib/mt/`）」，但该实现并不存在。`src/lib/mt/` 只有 `Mutex`、`CondVar`、`Lock`、`Thread`；`EventQueue.h:121-122` 声明的是 `std::unique_ptr<Mutex> m_readyMutex` 与 `std::unique_ptr<CondVar<bool>> m_readyCondVar`；全仓亦无任何文件使用 `std::atomic` 或 `std::memory_order`。以下文字记录代码的真实行为。无锁队列仍可作为未来的优化方向，但它不是当前设计的描述。

事件处理流水线：

```
网络 → 解码 → 队列 → 输入线程 → OS 合成
              ↓
         GUI 线程 (状态更新)
```

#### 后果
**正面：**
- 同步机制直观且成熟
- 无需处理 ABA 问题
- 可用常规工具调试

**负面：**
- 共享队列存在争用，延迟取决于唤醒开销，并非结构上即达亚毫秒级
- 原理上存在优先级反转

**中性：**
- 曾考虑无锁 MPSC 队列，但**未**实现

---

### ADR-0009: GoogleTest + CTest 测试框架

**状态**: 已接受
**日期**: 2024-01-15
**决策者**: @tupig-team
**技术背景**: #7

#### 背景
需要支持以下场景的测试框架：
- 单元测试 (C++)
- 集成测试 (多进程)
- 协议合规测试
- CI 集成

#### 决策
使用 **GoogleTest** 编写 C++ 单元测试 + **CTest** 编排。目标覆盖率 >80%。

结构：
```
src/unittests/
  base/        # 字符串、日志、设置
  common/      # 设置、常量
  deskflow/    # 剪贴板、输入、协议
  gui/         # 控件、对话框
  net/         # SecureSocket、指纹
  platform/    # 平台 Mock
```

#### 后果
**正面：**
- 丰富断言、死亡测试、参数化测试
- CTest: 并行、分片、标签
- XML/JUnit 输出供 CI
- GMock 支持 Mock

**负面：**
- 编译时间开销
- 多线程代码易产生脆弱测试

**中性：**
- 集成测试独立目标

---

### ADR-0010: vcpkg 管理 Windows 依赖

**状态**: 已接受
**日期**: 2024-01-15
**决策者**: @tupig-team
**技术背景**: #8

#### 背景
Windows 缺系统包管理器。需要一致的 Qt、OpenSSL、库。

#### 决策
使用 **vcpkg** 清单模式 (`vcpkg.json`)，并通过 `builtin-baseline` 锁定版本。**全部**第三方依赖 ——
Qt、OpenSSL 及其传递依赖 —— 均来自 vcpkg，且采用静态链接，使各平台产出独立可运行的可执行文件。
仓库内的 vcpkg 检出由 `scripts/bootstrap-vcpkg.{bat,sh}` 引导；不使用系统 Qt。

#### 后果
**正面：**
- 可复现构建
- 二进制缓存
- 版本锁定
- CMake 工具链集成

**负面：**
- Qt 编译耗时 (数小时)
- 磁盘占用 (~5-10GB)

**中性：**
- 替代方案: Conan, 手动安装

---

### ADR-0011: 扁平化双语文档结构

**状态**: 已接受
**日期**: 2024-09-18
**决策者**: @tupig-team
**技术背景**: #156

#### 背景
文档分散在 `docs/dev/`, `docs/user/`，仅英文，且含 CMake 构建产物。

#### 决策
扁平化为 `docs/` 双语 Markdown：

```
docs/
  build.md           # 编译指南
  contributing.md    # 贡献指南
  protocol.md        # 协议参考
  configuration.md   # 配置参考
  architecture.md    # 架构决策 (本文件)
  troubleshooting.md # 故障排查
  security.md        # 安全策略
```

每个文件：英文优先，中文次之，共享图表/表格。

#### 后果
**正面：**
- 单一事实来源
- GitHub 原生渲染
- 无文档构建步骤
- 贡献者直接编辑

**负面：**
- Markdown 文件变大
- 翻译同步手动

**中性：**
- Doxygen 生成 API 参考 (独立)