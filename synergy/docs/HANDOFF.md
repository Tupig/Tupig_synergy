# HANDOFF — TuPig Synergy 代码库优化重构

> **最后更新**: 2026-09-24
> **当前分支**: `main`
> **最新 Commit**: A-52～A-56 修复（五/六轮 CI 根因批；台账与本 HANDOFF 随笔提交 —— hash 见 `git log`）
> **状态**: Phase 0+1 完成；Phase 2 Step 1–4 完成；身份债 U-07/09/17/18/19 已关（`1269e7cb9`，详情见追踪台账）；B 计划进行中（EventQueue 已泵 Qt；IDataSocket 适配器 + TOFU + 默认 Qt + Step 5/6 待续 —— 阶段 1~3 数周级工程经拍板暂不执行，见 §5.4）；跨屏拖拽见 §5.6 清单（G1）；09-24 二轮审计 P1（A-12/A-13）+ P2（A-14～A-19）已改；CI 首跑暴露 A-28～A-35、二轮 A-36～A-41、三轮 A-42～A-47、四轮 A-48～A-51、五轮+六轮 A-52～A-56 均已修（A-30/A-41 拍板：无 secrets 时 skip；A-39 homepage 改 github 组织页；A-45 拍板：debian-12/ubuntu-24.04 走 Qt5 回退；A-52 为 A-21 xkbfile 探测回归收口）；P3（A-20～A-27）已修（`365f64cb7`）待 CI 实证；docs 已收敛六文件（`3f140c029`）

---

## 1. 会话目标与背景

### 1.1 项目概述
- **项目**: TuPig Synergy — 基于 Synergy/Deskflow 的跨平台键鼠共享工具
- **仓库**: `https://github.com/Tupig/TuPig_Product/tree/main/synergy`
- **技术栈**: C++20, CMake 3.25+, Qt 6.7+, OpenSSL 3.0+
- **版本**: 见 `extra/cmake/Version.cmake`（`SYNERGY_VERSION_*`；`vcpkg.json` 的 `version-string` 须与之对齐）
- **平台**: Windows / macOS / Linux

### 1.2 优化目标
经过系统性代码审计，发现 **23 个关键问题**，映射到 6 个 Phase、12 周实施：

| 优先级 | 数量 | 问题 |
|--------|------|------|
| P0 严重安全 | 5 | S-1~S-5 (TLS/协议/输入/X11) |
| P1 高危质量 | 7 | Q-1~Q-7 (死锁/缓冲/断言/测试/CI) |
| P2 性能瓶颈 | 5 | P-1~P-5 (轮询/拷贝/解析/Hook/转换器) |
| P3 技术债 | 6 | T-1~T-6 (C++17→20/Qt语法/智能指针/平台抽象) |

### 1.3 用户约束 (原文)
- "做任何改动前都需要先思考给出方案，选择最优的来做"
- "可执行文件不希望用户还要自己配备Qt6运行库，所以我希望集成"
- "不是全部都指向我这里啊，是有必须指向我项目的才要改成指向我仓库的地址"
- "不上传Github的，不要出现在可上传的内容里面"
- "路线图去掉，不要捏造不存在的计划"
- "文档最好是中英文版本合并的" / "一份里面中英文双语"
- "确保编写的所有代码和解决方案都具备跨平台兼容性"
- "不要使用硬编码，因为每台机器的环境是不一样的"
- "后续所有相关的修复与代码改动，都必须同步更新记录到此文件内" (Issue tracking document)

---

## 2. 已完成进展

### Phase 0: 基础设施 ✅
- [x] 创建 `refactor/security-baseline` 分支 (已合并删除)
- [x] `CMakePresets.json` 增强 (ASan/TSan/Coverage presets)
- [x] `.github/workflows/static-analysis.yml` (clang-tidy + cppcheck CI)
- [x] 删除误提交的 `CMakeUserPresets.json`

### Phase 1: 安全基线 ✅
- [x] **S-1 TLS 证书验证**: `verifyIgnoreCertCallback` → `verifyCertificateCallback` (链验证+过期+RSA≥2048)
- [x] **S-2 协议消息大小限制**: `MessageSizeLimit` 枚举 (Control=256, InputEvent=4KB, ClipboardChunk=64KB, FileChunk=256KB, AbsoluteMaximum=4MB)
- [x] **S-3 assert→异常**: 15 个 `assert(0)` 替换为 `throw BadClientException`/`throw DeskflowException`
- [x] **S-4 InputValidator**: 新建 `InputValidator.h/.cpp` — 范围验证+频率限制+敏感键拦截
- [x] **S-5 X11 降级**: `m_displayLost` 标记+`ioErrorHandler` 优雅处理+方法守卫
- [x] **Issue tracking 文档**: `.github/ISSUE_TEMPLATE/security-quality-refactoring.md`
- [x] **docs/security.md**: 安全策略文档 (修复 README 断链)
- [x] **README.md**: 移除伪造 Roadmap 和断链
- [x] **3 个审查建议项修复**: 残留 assert、冗余赋值、注释说明

### 代码审查 ✅
- QA 验证: 10 PASS / 1 MINOR (ProtocolUtil.cpp:43 残留 assert 已修复)
- 安全审查: 通过，无高危发现
- 并发审查: `InputValidator::isRateLimited` 非线程安全 (单线程场景 OK)

### 分支合并 ✅
- `refactor/security-baseline` 已合并到 `main` (fast-forward + rebase)
- 分支已删除 (本地 + 远程)
- 所有变更已推送到 `origin/main`

### 全面审计 ✅ (2026-09-23)
- [x] 4 轮多角度审计完成（范围/文档对账/R1–R10 合规/源码与过程/CI 与交付）；报告见 git 历史（原 `docs/audit-2026-09-23.md`，已随 docs 清理删除）
- [x] 新发现 A-01～A-11 登记至 `.github/ISSUE_TEMPLATE/security-quality-refactoring.md` §2.4；U 计数修正为 21 项
- [x] 审计整改：A-01～A-11 已关闭；U-03 / U-10 / U-13 / U-15 已关闭；U-07 / U-09 已随后 `1269e7cb9` 关闭（D1/E2），状态以追踪台账为准

### 二轮全面审计 🔄 (2026-09-24)
- [x] 4 路并行只读取证 + P1/P2 独立复核；报告见 git 历史（原 `docs/audit-2026-09-24.md`，已随 docs 清理删除）
- [x] 新发现 A-12～A-27 登记追踪文档 §2.5；A-10 状态补翻；变更日志补登 09-23 末 6 笔
- [x] P1 A-12/A-13 代码已改（lint cwd + ci-passed needs；flatpak 路径统一 workspace 根）— **待下次 CI 运行实证**
- [x] P2 A-14～A-19 已改（ci push→main；HANDOFF 回滚/工厂对齐；README 两处；U 详情节 Resolution）
- [x] CI 首跑（`35948919314`）：A-14 push→main 触发生效；A-12 lint 实证生效（检出 105 文件漂移）；新发现 A-28～A-30 登记
- [x] A-28 格式已修（应用 CI clang-format-diff，105 源文件）；A-29 已修（ci-passed/report/s3-upload 补 `working-directory: .`）— 待下次 CI 实证
- [x] A-30 已修（拍板：无 AWS secrets 时 s3-upload 上传步全 skip；注意 job 级 `if` 不可用 `secrets`，须 step 级判空）— 待下次 CI 实证
- [x] A-31 已修（CodeQL/Sonar/Valgrind 容器 job 首步补 `working-directory: .`，checkout 前 `synergy/` 不存在）— 待下次 CI 实证
- [x] A-32～A-35 已修（Linux 矩阵首步 cwd 同 A-31 根因；macOS GUI target 名空格 + 五处 TARGET_BUNDLE 同步；CodeQL autobuild 双项目歧义改 manual；metainfo homepage 私仓 404 改公开 URL）— 待下次 CI 实证
- [x] 二轮 CI（`35952577862`）暴露 6 类 26 job 挂，A-36～A-41 已修：qtbase_en.qm 路径+翻译包；macOS ld 拒 -z；XWindowsConfig.h include path；tupig.com TLS 挂改 github 组织页；s3-upload Check AWS 步缺 cwd；report Slack token 判空 — **三轮已实证 10 job 绿（含 s3/report/lint）**
- [x] 三轮 CI（`35957542966`，10 绿 / 19 挂）暴露 6 类根因，A-42～A-47 已修：aarch64 误开 AVX2（架构正则）；macOS `platform/` include 前缀错 ×6；Fedora 缺 qtbase_es.qm 硬 DEPENDS 改 EXISTS 软跳过；debian-12/ubuntu-24.04 Qt6 6.4<6.7 改 Qt5 回退（DependencyFallback + qtbase5-dev + 四腿 BUILD_TESTS=OFF）；rocky QSslServer 加 Qt6 守卫；flatpak Lint manifest 补 exceptions 标志 — **四轮已实证大部分腿绿（windows/flatpak/fedora/opensuse/archlinux/ubuntu-26.04/lint/get-version/s3/report）**
- [x] 四轮 CI（`35961135433`，11 挂含级联 ci-passed）暴露 4 类根因，A-48～A-51 已修：AppUtilUnix `<platform/OSXAutoTypes.h>` 改裸文件名；Qt5 `sslErrors` 用 `qOverload`、`errorOccurred`/`QLocalSocket::error` 加 Qt 5.15 守卫（RHEL 8 floor 5.13）；`QSettingsProxy.h`/`Settings.h` 补 `<memory>`；debian-13 Qt5 误选根因（疑 slim 缺 OpenGL 致 Qt6 组件失败）→ debian 补 `libgl-dev` 等 + `src/CMakeLists.txt` BUILD_TESTS 门控 `QT_VERSION_MAJOR EQUAL 6`（Qt5 自愈跳过）— 待下次 CI 实证
- [x] 五轮 CI（`35964563457`，commit `25f848a25`）7 job 挂（macOS×2 dyld、debian-13-x86_64 测试目录、debian-12×2 `<optional>`、ubuntu-24.04×2 format、ci-passed 级联）；六轮 CI（`35970799694`，commit `3f140c029`）23 job 挂 —— 主因 A-21 xkbfile 探测符号错（`XkbGetKeyboard` 在 libX11）致 UNIX X11 腿 Configure 全灭 + CodeQL；macOS dyld 未愈；flatpak×2 上游 libei 503（瞬态）。已修 A-52～A-56：`find_library` xkbfile；`CoreProcess.h` `<optional>`；`qFatal` `qlonglong` + 拼写；run-tests 缺目录跳过 + `Qt6_DIR` 诊断；`QT_IS_SHARED` 补 `.framework/` — **待 CI 实证；flatpak 需重跑**
- [x] P3 = A-20～A-27 已修（`365f64cb7`）：GoogleTest 幽灵、CMake 批、文档死链、打包卫生、身份残留、许可残留、A-10 行、light 回收站图标 — **待下次 CI 实证**；A-15 工厂接线归入 B 计划阶段 1（§5.4，暂不执行）；docs 收敛六文件（`3f140c029`）

### main 分支提交记录 (最新 6 个)
```
e72b61711 feat(ci): GitHubDesktop2Chinese 构建/发布/安全质量工作流
a1942fe06 fix(ci): 修复自动维护 workflow 的 YAML 语法错误
a8f8aa5be fix(ghdesktop2chinese): 修复自动维护工具的四个缺陷
7afab51fb feat(ghdesktop2chinese): 新增 localization.json 自动维护工具与工作流
3f140c029 docs: synergy/docs 收敛为六文件，迁移 G1/交付验证/B 计划状态至 HANDOFF
365f64cb7 fix(docs,cmake): A-20～A-27 修复将删文档内错误与全仓遗留
```

---

## 3. 关键决策与技术方案

### 3.1 Phase 1 决策
| 决策 | 选择 | 理由 |
|------|------|------|
| TLS 验证方式 | OpenSSL 回调 + TOFU | 兼容现有架构，defense-in-depth |
| 协议限制策略 | 分级枚举 (非单一上限) | 不同消息类型有不同合理上限 |
| 异常类型选择 | `BadClientException` (网络输入) vs `DeskflowException` (内部逻辑) | 区分恶意输入和内部错误 |
| 输入验证位置 | 独立模块 `InputValidator` | 可复用、可测试、平台无关 |
| X11 降级策略 | 标记+事件+守卫 (非重连) | 最小风险，重连逻辑留给 Phase 3 |

### 3.2 Phase 2 方案 (Step 1–4 已落地，Step 5–6 待实施)
- **目标**: QtNetwork 迁移，消除 SocketMultiplexer 死锁
- **6 步**: 并发重构 → 抽象层 → QtTcpTransport → QtTlsTransport → 智能指针 → 清理
- **回滚策略**: 代码级 revert。运行时 `USE_LEGACY_NETWORK` 与 CMake `LEGACY_NETWORK` 均不可用——前者只在未接线的 `NetworkTransportFactory` 内读取，后者全仓不存在；工厂尚未接入 ServerApp/ClientApp（A-15 / §5.4 阶段 1）。默认仍走 legacy `TCPSocketFactory`。
- **文档**: 原 `docs/phase2-qt-network-migration.md` 已随 `docs/archive/` 一并删除；方案已落地 Step 1-4，剩余步骤见第 5 节

---

## 4. 涉及的重要文件及代码位置

### 4.1 核心变更文件 (已在 main)
| 文件 | Phase | 变更内容 |
|------|-------|----------|
| `src/lib/net/SecureSocket.cpp` | S-1 | `verifyCertificateCallback` (行 68-108) |
| `src/lib/deskflow/protocol/ProtocolTypes.h` | S-2 | `MessageSizeLimit` 枚举 (行 125-166) |
| `src/lib/deskflow/protocol/ProtocolUtil.cpp` | S-3 | 15 处 assert→throw |
| `src/lib/deskflow/input/InputValidator.h` | S-4 | 校验器（已接入客户端入站路径） |
| `src/lib/deskflow/input/InputValidator.cpp` | S-4 | 同上 |
| `src/lib/client/ServerProxy.cpp` | S-4 | 入站按键事件接入校验 (keyDown/keyRepeat/keyUp) |
| `src/unittests/deskflow/InputValidatorTests.cpp` | S-4 | 新增单元测试 |
| `src/lib/platform/linux/XWindowsScreen.h` | S-5 | `m_displayLost` 成员 (行 252) |
| `src/lib/platform/linux/XWindowsScreen.cpp` | S-5 | `ioErrorHandler` + 守卫 (行 1638-1651) |

### 4.2 基础设施文件
| 文件 | 用途 |
|------|------|
| `.github/workflows/static-analysis.yml` | clang-tidy + cppcheck CI |
| `CMakePresets.json` | ASan/TSan/Coverage 预设 |
| `.github/ISSUE_TEMPLATE/security-quality-refactoring.md` | 问题追踪台账（A/U 条目） |
| `docs/security.md` | 安全策略文档 |
| `docs/HANDOFF.md` | 本文件 — 会话交接文档 |

> 注：原 `docs/delivery.md`（交付矩阵）、`docs/consistency-audit.md`（U 条目审计）与两份
> audit 报告已按 docs 清理计划删除；交付要点迁入本文件 §5.5～§5.6，结构性约束见
> `AGENTS.md`「交付约束」，U 条目状态以追踪台账为准。

### 4.3 Phase 2 涉及文件
| 文件 | 用途 | 操作 | 状态 |
|------|------|------|------|
| `src/lib/net/SocketMultiplexer.cpp/h` | 网络事件循环 | Step 1 重构 | ✅ 已完成 |
| `src/lib/net/INetworkTransport.h` | 统一传输接口 | Step 2 新增 | ✅ 已完成 |
| `src/lib/net/LegacyNetworkTransport.cpp/h` | Legacy 包装器 | Step 2 新增 | ✅ 已完成 |
| `src/lib/net/QtNetworkTransport.cpp/h` | Qt QTcpSocket 传输（完整实现） | Step 2–3 | ✅ 已完成 |
| `src/lib/net/NetworkTransportFactory.cpp/h` | 运行时切换工厂 | Step 2 新增 | ⚠️ 未接线（A-15：无生产调用者，见 §5.4 阶段 1） |
| `src/lib/net/IDataSocket.h` | 数据 socket 接口 | Step 2 修改 (添加 getSocket) | ✅ 已完成 |
| `src/lib/net/TCPSocket.cpp/h` | TCP 传输 | Step 3 替换 | 待实施 |
| `src/lib/net/SecureSocket.cpp/h` | TLS 传输 | Step 4 替换 | 待实施 |
| `src/lib/net/TCPSocketFactory.cpp/h` | 工厂 | Step 2 替换，Step 6 删除 | 待实施 |
| `src/lib/net/ISocketMultiplexerJob.h` | Job 接口 | Step 6 删除 | 待实施 |
| `src/lib/net/TSocketMultiplexerMethodJob.h` | Job 模板 | Step 6 删除 | 待实施 |
| `src/lib/net/TCPListenSocket.cpp/h` | 监听套接字 | Step 6 删除 | 待实施 |
| `src/lib/net/SecureListenSocket.cpp/h` | TLS 监听 | Step 6 删除 | 待实施 |

---

## 5. 待处理事项与已知问题

### 5.1 Phase 2 实施状态（与追踪文档统一的 Step 定义）

| Step | 含义 | 状态 |
|------|------|------|
| 1 | SocketMultiplexer 并发重构 | ✅ 已完成 |
| 2 | QtNetwork 抽象层 + Legacy/Qt 双实现 | ✅ 已完成 |
| 3 | `QtNetworkTransport` 用 QTcpSocket 完整实现（工厂可选；`TCPSocket` 仍并存） | ✅ 已完成 |
| 4 | Qt TLS（`QSslSocket` / `QSslServer`）路径（`SecureSocket` 仍并存，Legacy 回滚可用） | ✅ 已完成 |
| 5 | 网络层智能指针化 | 待实施 |
| 6 | 移除旧 `TCPSocket` / Multiplexer Job 等遗留代码 | 待实施 |

说明：追踪文档里的「Step 3 完成」指 QTcpSocket 集成，**不是**「已删除 TCPSocket」。二者勿混用。

### 5.2 已知问题
- **XWindowsScreen.cpp/h**: 预存 LSP 报错 (X11 头文件在 Windows 不可用)，非本次改动
- **`export` 命令**: Windows PowerShell 不支持，bash 命令需用 `workdir` 参数
- **cmake/ninja**: 本地不在 PATH，无法本地构建验证
- **子代理余额**: opencode 平台余额不足会导致子代理反复重试失败

### 5.3 Phase 3-6 待规划
- Phase 3: Server 拆分 (Week 5-8) — 7 大模块拆分
- Phase 4: 性能优化 (Week 9-10) — 心跳隔离/异步 I/O/零拷贝
- Phase 5: 部署打包 (Week 11) — 三平台安装包
- Phase 6: 验收交付 (Week 12) — 全维度验收

### 5.4 B 计划（原 plan-B）决策与阶段状态

> 原 `docs/plan-B-D1-E2-F1-G1.md` 已删除，状态迁移至此。**阶段 1~3 为数周级工程，
> 经拍板暂不执行**；下列状态为权威记录。

已锁定决策（2026-09-23）：

| 代号 | 选择 | 状态 |
|---|---|---|
| B | Qt 默认 + Step 5 + Step 6 删除 Legacy | 阶段 1~3 暂不执行 |
| D1 | 仅保留 synergy 图标主题（deskflow SVG 以别名并入 `synergy.qrc`） | ✅ 已完成 |
| E2 | U-09 路径有意保留（`kUpstreamId` 供上游同步 / i18n 边界） | ✅ 已关闭（有意设计） |
| F1 | 关闭 U-17/18/19（显示名 / i18n 耦合 / 文档化） | ✅ 已完成（`1269e7cb9`） |
| G1 | 用户执行双机拖拽；agent 提供清单 | 清单见 §5.6，待用户执行 |

阶段状态：

- **阶段 0（命名/图标/文档）**: ✅ 完成 —— D1 别名折叠、U-17 显示名 `TuPig Synergy`、U-18 保持内部 deskflow 名、U-09/U-19 文档化、G1 清单迁入 §5.6。
- **阶段 1（Qt 适配器 + 指纹 TOFU，A-15 工厂接线）**: ⏸ 暂不执行 —— `QtDataSocket`/`QtListenSocket` 适配器、TOFU 对齐、经 factory 接线 apps 并以 `USE_LEGACY_NETWORK=0` 实证后再切默认。
- **阶段 2（默认 Qt + Step 5 智能指针）**: ⏸ 暂不执行。
- **阶段 3（Step 6 删除 Legacy 栈）**: ⏸ 暂不执行。

验证方式（恢复执行时适用）：每阶段 `scripts\build.bat release` + 单测 + 推送；G1 由用户按 §5.6 执行。

### 5.5 交付验证状态与 R8 风险（自 delivery.md 迁入）

**已在 Windows（Release、静态 triplet）验证：**

- 构建与打包成功，便携归档（7Z）正常产出；MSI 已构建并经 Windows Installer 数据库 API 检查（File/ServiceInstall/ServiceControl 三表），**未执行实际安装**。
- `dumpbin /DEPENDENTS` 确认不依赖 MSVC 运行库 DLL（`/MT` 静态链接，vcpkg 静态 triplet）。
- GUI→core 握手：`gui/startCoreWithGui=true` + 服务端 `coreMode` 下，GUI 约 3 秒内拉起 `synergy-core.exe`（端到端实证 U-01 修复）。
- 界面语言随系统区域解析（`initial language: zh_CN`）。
- 单元测试：Release 与 AddressSanitizer 两种配置均 25/25 通过。

**R8 风险（未验证，沿革）**：macOS 与 Linux 的构建、打包与运行**零实机验证** —— 无对应机器，DEB/RPM/DMG 从未产出；相关代码修复仅为代码级推理 + CI 绿（CI 的 macOS/Linux 腿是第一级证据）。结构性约束（daemon 独立、便携包排除 daemon、macOS `.app`、无自安装服务）见 `AGENTS.md`「交付约束」。

**产物摘要**：Windows = 便携 7Z + MSI；macOS = 含 `.app` 的 DMG；Linux = DEB 或 RPM（依 `/etc/os-release` 二选一）+ CI Flatpak + Arch PKGBUILD；AppImage 未实现。如何产出：`scripts\build.{bat,sh} release` 后 `cmake --build build --target package`。

### 5.6 G1：跨屏拖拽人工验证清单（Windows，自 delivery.md 迁入）

文件传输（拖拽）各层状态：

| 层 | 状态 | 已验证 |
|---|---|---|
| 协议（`DDRG`/`DFTR`）+ 文件名净化 | 已实现 | 单元测试 |
| 客户端接收 + 加固落盘 | 已实现 | 单元测试 |
| 服务端外发（`FileTransferOutbound` + 离开主屏发送） | 已实现 | 单元测试 |
| Windows OLE `IDropTarget` + CF_HDROP 解析 | 已实现 | 解析器单测；跨屏捕获需人工验证 |
| Windows `IDropSource`（投进本机资源管理器） | 已实现 | CF_HDROP 写入往返单测；DoDragDrop 需人工验证 |
| macOS 拖拽剪贴板（`copyDraggedFilePaths`） | 已实现 | **否** —— 无 macOS 主机，仅静态审阅 |
| Linux XDND / Wayland DnD | **未实现**（上游亦从未实现） | 不适用 |

启用条件：`fileTransfer/enabled=true` 且 `fileTransfer/dropDirectory` 非空。

人工验证步骤 —— 两台同局域网机器；一台主屏（服务端），一台客户端；同一提交构建。两端均设置上述启用条件：

1. **主屏 → 客户端（IDropTarget 捕获）**：在主屏从资源管理器拖文件到共享边缘，直到光标跳到客户端。确认客户端落盘目录收到文件（文件名已净化；不覆盖已有文件）。
2. **客户端 → 本机资源管理器（IDropSource）**：接收完成后 Synergy 在光标下发起 OLE 拖拽。投放到本机资源管理器并确认文件出现。
3. **拖拽中按 Escape / 取消**：任一侧不得崩溃。
4. **反例**：`fileTransfer/enabled=false` 时，按住左键离开主屏不得发送文件。

将 OS、`synergy-core --version`、通过/失败记入追踪台账或本文件。

---

## 6. 后续建议的下一步操作

### 立即执行
1. **下次 CI**: 实证 A-12～A-13、A-28～A-51 全链路 + P3（A-20～A-27，`365f64cb7`）——lint 零 diff、ci-passed/report 不因 cwd 挂、s3/report 无 secrets 时 skip、Linux/macOS 矩阵 Configure/Build 过、flatpak lint 过、`365f64cb7` 的 CMake/REUSE 改动不炸 Configure
2. **Step 5 / B 计划阶段 1**: 网络层智能指针化，或 Qt 适配器 + TOFU + 工厂接线（闭合 A-15；阶段 1~3 经拍板暂不执行，恢复前先读 §5.4）
3. **G1**: 本机文件拖拽跨屏人工验证（清单见 §5.6，由用户执行）

### Step 1 具体操作 ✅ 已完成
```
Commit: d84a8c5ef — refactor(net): simplify SocketMultiplexer concurrency model
Commit: bb0e7bb33 — fix(net): resolve use-after-free in SocketMultiplexer::removeSocket
```

### 验证清单
- [x] Step 1: TSAN 验证通过
- [x] Step 1: 编译 0 警告
- [x] Step 2: LSP 诊断 0 errors
- [x] Step 2: 所有权模型明确
- [x] Step 2: 接口文档完整

### 下一步: CI 全绿实证 → Step 5 / B 计划阶段 1 → G1 验证
1. 下次 CI 实证 A-12～A-13、A-28～A-51 + P3（`365f64cb7`）
2. 网络层智能指针化（Step 5）或 B 计划阶段 1（适配器 + TOFU + 工厂接线，闭合 A-15；见 §5.4）
3. Windows 跨屏拖拽人工验证（清单 §5.6：IDropTarget 捕获 + IDropSource 投放到 Explorer）
4. 默认切到 Qt 传输前，用 `USE_LEGACY_NETWORK=0` 做 TLS 互通冒烟（注：工厂尚未接线，见 A-15 / §5.4 阶段 1）

---

## 7. 关键约束提醒

1. **同步更新 Issue tracking**: `.github/ISSUE_TEMPLATE/security-quality-refactoring.md` 必须与代码变更同步
2. **跨平台兼容**: 所有代码必须 Win/Mac/Linux 三平台编译通过
3. **中英文双语**: 所有文档必须中英文合并
4. **不上传 GitHub 的内容**: 不要出现在可上传内容里
5. **不要硬编码路径**: 使用环境变量或 CMake 查找
6. **先方案后实施**: 任何改动前先给出方案，选择最优

---

> **交接完成**。Phase 0+1 与 Phase 2 Step 1–4 已完成（含 Windows IDropSource、U-10/U-13/U-15/A-10）；P3（A-20～A-27）已修待 CI 实证。下一步优先下次 CI 实证，或跨屏拖拽人工验证（§5.6）。
