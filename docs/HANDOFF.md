# HANDOFF — TuPig Synergy 代码库优化重构

> **最后更新**: 2026-10-09
> **当前分支**: `main`
> **仓库**: `https://github.com/Tupig/Tupig_synergy`（public，项目在仓库根目录）
> **最新 Commit**: 以 `git log -1` 为准；阶段 2 关键提交：T1 `a7541a6`、T2 `68c61a5`、T3 `610d0e8`，批1/批2 修复随后（见 §2 提交记录）
> **状态**: Phase 0+1 完成；**阶段 2（结构整理）完成** — extra/ 溶解（cmake/、deploy/、config/、src/lib/synergy）、REUSE 全绿（893/893）、deskflow→synergy 全仓改名（issue #146）、CI 脚本归置 scripts/ci/；**批1 源码修复（C-01～C-09）与批2 CI 修复（B-01～B-11）已落地并关闭对应 issue**；**网络层（N-01/N-03/N-04/N-05/N-06 + A-15）已完成并默认切换 Qt**（环回端到端测试：明文/TOFU 拒绝/TOFU 信任+TLS；`USE_LEGACY_NETWORK=1` 回退 raw 栈）；批3 文档完成；**N-07（删遗留栈）按验收条件保留一个稳定期**；阶段 4 回归已重跑（见下）。GitHub Issues 为唯一问题追踪载体（144 条）。**阶段 4 回归完成**：free 子集 run 37754662586/37768850901 与全量 19 腿矩阵 run 37755828508/37770107215 均 SUCCESS（后者含 Qt 默认传输切换后的全网回归：Windows x64+arm64、macOS x64+arm64、Flatpak x2、全 Linux 矩阵含 rocky/debian Qt5 腿）（Windows x64/arm64、macOS x64/arm64、Flatpak x86_64/aarch64、全 Linux 矩阵含 rocky Qt5 腿、lint/get-version/ci-passed/s3-upload；commit d4e57c1 起）。

---

## 0. 2026-10-09 首轮审查修复

登记并处理的 issue：[#147](https://github.com/Tupig/Tupig_synergy/issues/147)–[#160](https://github.com/Tupig/Tupig_synergy/issues/160)。
空 SSL 上下文改为抛 `SocketException`（`net` 不依赖 `SynergyException`）、握手 retry 改为局部变量、SecureSocket 读缓冲改为成员、TLS 注释与 TOFU 对齐、daemon `--version` 打印 `kDisplayVersion`、ClientListener 空指针改为抛 `SocketException`、图标目录 `deskflow-*` 改为 `synergy-*`、文件接收失败仍消耗文件名且发布不覆盖已有文件、IPC 只按第一个 `=` 切开、daemon 拒绝命令行元字符路径、按键限速键码上限、文件大小设置不再回绕、IPC 待发队列上限 32。
Windows GUI 产物名为 `synergy_<X.Y.Z>.exe`（`Version.cmake` 自动生成，issue #161）。`synergy-core.exe` 与 `synergy-daemon.exe` 保持固定文件名。工作流只保留 `.github/workflows/ci.yml`（issue #162）。`package-type=release` 一次构建 Windows、macOS、Linux（issue #163）。`get-version` 必须把 `base` 传给 Windows 的 `synergy_<版本>.exe` 检查（issue #164）。
验证：`cmake --build --preset windows-msvc-release` 成功；`ctest` 于 `build/src/unittests`（Release）34 项中修复前 33 通过、`FileTransferReceiverTests` 修复后通过；`InputValidatorTests` 与 `FileTransferReceiverTests` 复跑通过。发布构建（`package-type=release`，`SYNERGY_VERSION_RELEASE`）的版本字符串是 `1.21.2`，不含 `-dev`，daemon `--version` 只打印该字符串。版权行中的 Deskflow 归属不改。根目录 `ctest` 看不到用例，是因为 `enable_testing()` 在 `src/unittests`，测试注册在 `build/src/unittests`。

## 1. 会话目标与背景

### 1.1 项目概述
- **项目**: TuPig Synergy — 基于 Synergy/Deskflow 的跨平台键鼠共享工具
- **仓库**: `https://github.com/Tupig/Tupig_synergy`
- **技术栈**: C++20, CMake 3.25+, Qt 6.7+, OpenSSL 3.0+
- **版本**: 见 `cmake/Version.cmake`（`SYNERGY_VERSION_*`；`vcpkg.json` 的 `version-string` 须与之对齐）
- **平台**: Windows / macOS / Linux

### 1.2 优化目标
经过系统性代码审计，发现 **23 个关键问题**，映射到 6 个 Phase、12 周实施：

| 优先级 | 数量 | 问题 |
|--------|------|------|
| P0 严重安全 | 5 | S-1 (#2)~S-5 (#6) (TLS/协议/输入/X11) |
| P1 高危质量 | 7 | Q-1 (#7)~Q-7 (#13) (死锁/缓冲/断言/测试/CI) |
| P2 性能瓶颈 | 5 | P-1 (#14)~P-5 (#18) (轮询/拷贝/解析/Hook/转换器) |
| P3 技术债 | 6 | T-1 (#19)~T-6 (#24) (C++17→20/Qt语法/智能指针/平台抽象) |

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
- [x] `.github/workflows/ci.yml` 内的 clang-tidy / cppcheck（手动 analyze）
- [x] 删除误提交的 `CMakeUserPresets.json`

### Phase 1: 安全基线 ✅
- [x] **S-1 (#2) TLS 证书验证**: `verifyIgnoreCertCallback` → `verifyCertificateCallback` (链验证+过期+RSA≥2048)
- [x] **S-2 (#3) 协议消息大小限制**: `MessageSizeLimit` 枚举 (Control=256, InputEvent=4KB, ClipboardChunk=64KB, FileChunk=256KB, AbsoluteMaximum=4MB)
- [x] **S-3 (#4) assert→异常**: 15 个 `assert(0)` 替换为 `throw BadClientException`/`throw SynergyException`
- [x] **S-4 (#5) InputValidator**: 新建 `InputValidator.h/.cpp` — 范围验证+频率限制+敏感键拦截
- [x] **S-5 (#6) X11 降级**: `m_displayLost` 标记+`ioErrorHandler` 优雅处理+方法守卫
- [x] **Issue tracking**: 原 `.github/ISSUE_TEMPLATE/security-quality-refactoring.md` 于 2026-10-08 **全量迁移到 GitHub Issues**（144 条 = 存量 103 + 四域审查新发现 41），本地台账已删除
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
- [x] 新发现 A-01 (#25)～A-11 (#35) 登记至原台账 §2.4（2026-10-08 迁移为 GitHub Issues）；U 计数修正为 21 项
- [x] 审计整改：A-01 (#25)～A-11 (#35) 已关闭；U-03 (#86) / U-10 (#93) / U-13 (#96) / U-15 (#98) 已关闭；U-07 (#90) / U-09 (#92) 已随后 `1269e7cb9` 关闭（D1/E2），状态以 GitHub Issues 为准

### 二轮全面审计 🔄 (2026-09-24)
- [x] 4 路并行只读取证 + P1/P2 独立复核；报告见 git 历史（原 `docs/audit-2026-09-24.md`，已随 docs 清理删除）
- [x] 新发现 A-12 (#36)～A-27 (#51) 登记追踪文档 §2.5；A-10 (#34) 状态补翻；变更日志补登 09-23 末 6 笔
- [x] P1 A-12 (#36)/A-13 (#37) 代码已改（lint cwd + ci-passed needs；flatpak 路径统一 workspace 根）— **已实证：run `35988655131`（A-12 lint 生效）**
- [x] P2 A-14 (#38)～A-19 (#43) 已改（ci push→main；HANDOFF 回滚/工厂对齐；README 两处；U 详情节 Resolution）
- [x] CI 首跑（`35948919314`）：A-14 (#38) push→main 触发生效；A-12 (#36) lint 实证生效（检出 105 文件漂移）；新发现 A-28 (#52)～A-30 (#54) 登记
- [x] A-28 (#52) 格式已修（应用 CI clang-format-diff，105 源文件）；A-29 (#53) 已修（ci-passed/report/s3-upload 补 `working-directory: .`）— 待下次 CI 实证（后由 run `35988655131` 全绿覆盖）
- [x] A-30 (#54) 已修（拍板：无 AWS secrets 时 s3-upload 上传步全 skip；注意 job 级 `if` 不可用 `secrets`，须 step 级判空）— 待下次 CI 实证（后由 run `35988655131` 全绿覆盖）
- [x] A-31 (#55) 已修（CodeQL/Sonar/Valgrind 容器 job 首步补 `working-directory: .`，checkout 前 `synergy/` 不存在）— 待下次 CI 实证（后由 run `35988655131` 全绿覆盖）
- [x] A-32 (#56)～A-35 (#59) 已修（Linux 矩阵首步 cwd 同 A-31 (#55) 根因；macOS GUI target 名空格 + 五处 TARGET_BUNDLE 同步；CodeQL autobuild 双项目歧义改 manual；metainfo homepage 私仓 404 改公开 URL）— 待下次 CI 实证（后由 run `35988655131` 全绿覆盖）
- [x] 二轮 CI（`35952577862`）暴露 6 类 26 job 挂，A-36 (#60)～A-41 (#65) 已修：qtbase_en.qm 路径+翻译包；macOS ld 拒 -z；XWindowsConfig.h include path；tupig.com TLS 挂改 github 组织页；s3-upload Check AWS 步缺 cwd；report Slack token 判空 — **三轮已实证 10 job 绿（含 s3/report/lint）**
- [x] 三轮 CI（`35957542966`，10 绿 / 19 挂）暴露 6 类根因，A-42 (#66)～A-47 (#71) 已修：aarch64 误开 AVX2（架构正则）；macOS `platform/` include 前缀错 ×6；Fedora 缺 qtbase_es.qm 硬 DEPENDS 改 EXISTS 软跳过；debian-12/ubuntu-24.04 Qt6 6.4<6.7 改 Qt5 回退（DependencyFallback + qtbase5-dev + 四腿 BUILD_TESTS=OFF）；rocky QSslServer 加 Qt6 守卫；flatpak Lint manifest 补 exceptions 标志 — **四轮已实证大部分腿绿（windows/flatpak/fedora/opensuse/archlinux/ubuntu-26.04/lint/get-version/s3/report）**
- [x] 四轮 CI（`35961135433`，11 挂含级联 ci-passed）暴露 4 类根因，A-48 (#72)～A-51 (#75) 已修：AppUtilUnix `<platform/OSXAutoTypes.h>` 改裸文件名；Qt5 `sslErrors` 用 `qOverload`、`errorOccurred`/`QLocalSocket::error` 加 Qt 5.15 守卫（RHEL 8 floor 5.13）；`QSettingsProxy.h`/`Settings.h` 补 `<memory>`；debian-13 Qt5 误选根因（疑 slim 缺 OpenGL 致 Qt6 组件失败）→ debian 补 `libgl-dev` 等 + `src/CMakeLists.txt` BUILD_TESTS 门控 `QT_VERSION_MAJOR EQUAL 6`（Qt5 自愈跳过）— 待下次 CI 实证（后由 run `35988655131` 全绿覆盖）
- [x] 五轮 CI（`35964563457`，commit `25f848a25`）7 job 挂（macOS×2 dyld、debian-13-x86_64 测试目录、debian-12×2 `<optional>`、ubuntu-24.04×2 format、ci-passed 级联）；六轮 CI（`35970799694`，commit `3f140c029`）23 job 挂 —— 主因 A-21 (#45) xkbfile 探测符号错（`XkbGetKeyboard` 在 libX11）致 UNIX X11 腿 Configure 全灭 + CodeQL；macOS dyld 未愈；flatpak×2 上游 libei 503（瞬态）。已修 A-52 (#76)～A-56 (#80)：`find_library` xkbfile；`CoreProcess.h` `<optional>`；`qFatal` `qlonglong` + 拼写；run-tests 缺目录跳过 + `Qt6_DIR` 诊断；`QT_IS_SHARED` 补 `.framework/` — **待 CI 实证；flatpak 需重跑**
- [x] P3 = A-20 (#44)～A-27 (#51) 已修（`365f64cb7`）：GoogleTest 幽灵、CMake 批、文档死链、打包卫生、身份残留、许可残留、A-10 (#34) 行、light 回收站图标 — **待下次 CI 实证（后由 run `35988655131` 全绿覆盖）**；A-15 (#39) 工厂接线归入网络层任务（2026-10 已批准执行）；docs 收敛六文件（`3f140c029`）
- [x] A-57 (#81) 已修：`ci.yml` push/PR 增加 `synergy/**` 等路径过滤，与 `ghdesktop2chinese.yml` 触发隔离（两工作流本体本无交叉（**A-59 (#83) 已移除该 paths 过滤**，ci.yml 现无 paths 过滤；本条以此为准关闭））— 待 CI 实证
- [x] **阻断→拍板**：五/六轮后所有 run job `runner_id:0`、`steps:[]`、日志空 — check annotation 实证 *「recent account payments have failed or your spending limit needs to be increased」*（GitHub Actions billing）。用户拍板「付费的不要了」→ **A-58 (#82) 免费化瘦身**（见下）
- [x] **A-58 (#82) 免费化瘦身**：ci.yml 删 `schedule`；Windows(2×)/macOS(10×)/flatpak/s3-upload 仅 `release`+`workflow_dispatch`；Linux 矩阵拆 `free`（push/PR，4 腿 x86_64：ubuntu-26.04/debian-13/debian-12/ubuntu-24.04）与 `full`（19 腿，dispatch/release），源在 `.github/matrices/linux-targets.json`；CodeQL/Sonar 改仅 `workflow_dispatch`（私仓 CodeQL 需 GHAS 付费）；去掉 ci.yml 内 PR 自动 valgrind；ghdesktop2chinese 周定时用户已删（`3aaa92ed1`）— `478be74cc` 已推送
- [x] **A-58 (#82) 推送后实测（run `35978730528`）**：瘦身结构生效（Windows/macOS/flatpak/s3-upload 正确 skip、无 CodeQL/Sonar 自动跑），但 **lint/get-version/ci-passed/report 仍 `runner_id:0`、`steps:[]` 9 秒失败 —— 账户 billing 硬锁未解除**。**用户须在 GitHub Settings → Billing & plans 处理（移除失败付款方式 / 调 spending limit）或将仓库转 public；代码侧免费化已完成，无法再省**
- [x] **A-59 (#83) 仓库重构（转 public + 扁平化）**：仓库由 `Tupig/TuPig_Product` 改名 **`Tupig/Tupig_synergy` 并转为 public**（public repo Actions 分钟免费，从根上绕开私仓 billing）；`synergy/` 下全部内容 `git mv` 到仓库根目录（`src/`、`docs/`、`cmake/`、`extra/`、`CMakeLists.txt`、`AGENTS.md` …），`synergy/` 目录消失；同步修正所有路径引用：workflows/actions 的 `working-directory`、`${{ github.workspace }}/synergy/build`、flatpak `manifest-path`/exceptions、artifact 路径、`ci.yml` 去掉 `paths` 过滤与 `defaults.working-directory`；仓库名 URL `Tupig/TuPig_Product`→`Tupig/Tupig_synergy`（含 README/REUSE/vcpkg/metainfo/UrlConstants/constants.h/AboutDialog/6 份 .ts/manpage）；`.gitignore` 合并；`.github/CONTRIBUTING.md`、`CODEOWNERS`、`AGENTS.md` 引用改根；`SECURITY.md` 由 GitHub 模板改为真实上报流程；flatpak metainfo「私仓 404」注释删除。**注意**：git 历史中仍含上游许可代码（`kOfflineActivationHex`/`web-muskoka`）与 billing 记录（转 public 前已向用户说明并获同意）。ghdesktop2chinese 工作流此前已拆出为独立仓库，本地目录由根 `.gitignore` 忽略
- [x] **A-59 (#83) 校验**：全部 workflow/action YAML 与 `linux-targets.json` 解析通过；`git grep` 确认无残留 `synergy/` 路径引用（历史文档中的旧路径文字保留）；代码侧 `synergy/` 仅为 C++ include 命名空间与产品名，未改
- [x] **A-59 (#83) 实证（run `35988655131`，`47953e9af`）**：**CI 全绿 `success`** —— `lint-clang` ✅、`get-version` ✅、4 腿 free 矩阵（`ubuntu-26.04-x86_64`/`debian-12-x86_64`/`debian-13-x86_64`/`ubuntu-24.04-x86_64`）✅、`ci-passed` ✅；Windows/macOS/flatpak/s3-upload/report 按设计 skip。**转 public 后 Actions 不再 billing 硬锁、真实运行**，同时实证 A-12 (#36)（lint）、A-52 (#76)（xkbfile 探测）、A-53 (#77)（debian-12 `<optional>`）、A-54 (#78)（ubuntu-24.04 format）、A-55 (#79)（测试目录）等历轮修复；A-56 (#80)（macOS dyld）与全量 19 腿矩阵须手动 `workflow_dispatch`/release 覆盖

### main 分支提交记录（阶段 2 / 批1 / 批2 精选；完整见 `git log`）
```
610d0e8 refactor(rename): 注释与字符串 Deskflow 清理（T3）
68c61a5 refactor(rename): namespace deskflow → synergy（T2）
a7541a6 refactor(rename): 目录/文件名与 i18n 联动（T1）
1c4eca1 docs/chore(tree): 阶段2 清扫 — REUSE 全绿
fbc5fe1 refactor(tree): extra/deploy 迁入 deploy/
ea186f3 refactor(tree): 配置样例迁入 config/ + 孤儿删除
e806cbb fix(net): SecureSocket doWrite 重试状态成员化（C-01）
4b73768 fix(net): serviceThread job 异常兜底（C-02）
e84f465 fix: 25 处 assert(0&&) 处理（C-03）
7152a67 fix(ci): static-analysis 门禁真实化（B-01）
f429133 fix(ci): valgrind 真插桩（B-02）
6ba66ae fix(ci): esigner 钉 SHA / permissions / retention（B-03/04/11）
```

---

## 3. 关键决策与技术方案

### 3.1 Phase 1 决策
| 决策 | 选择 | 理由 |
|------|------|------|
| TLS 验证方式 | OpenSSL 回调 + TOFU | 兼容现有架构，defense-in-depth |
| 协议限制策略 | 分级枚举 (非单一上限) | 不同消息类型有不同合理上限 |
| 异常类型选择 | `BadClientException` (网络输入) vs `SynergyException` (内部逻辑) | 区分恶意输入和内部错误（T2 后命名空间为 `synergy::`） |
| 输入验证位置 | 独立模块 `InputValidator` | 可复用、可测试、平台无关 |
| X11 降级策略 | 标记+事件+守卫 (非重连) | 最小风险，重连逻辑留给 Phase 3 |

### 3.2 Phase 2 方案 (Step 1–4 已落地；Step 5–6 与 Qt 接线已批准恢复执行，见 §5.4)
- **目标**: QtNetwork 迁移，消除 SocketMultiplexer 死锁
- **6 步**: 并发重构 → 抽象层 → QtTcpTransport → QtTlsTransport → 智能指针 → 清理
- **回滚策略**: 代码级 revert。运行时 `USE_LEGACY_NETWORK` 与 CMake `LEGACY_NETWORK` 均不可用——前者只在未接线的 `NetworkTransportFactory` 内读取，后者全仓不存在；工厂尚未接入 ServerApp/ClientApp（A-15 (#39) / §5.4 阶段 1）。默认仍走 legacy `TCPSocketFactory`，直到网络层任务（N-01～N-07）完成切换。
- **文档**: 原 `docs/phase2-qt-network-migration.md` 已随 `docs/archive/` 一并删除；方案已落地 Step 1-4，剩余步骤见第 5 节

---

## 4. 涉及的重要文件及代码位置

### 4.1 核心变更文件 (已在 main)
| 文件 | Phase | 变更内容 |
|------|-------|----------|
| `src/lib/net/SecureSocket.cpp` | S-1 (#2) | `verifyCertificateCallback` (行 68-108) |
| `src/lib/synergy/protocol/ProtocolTypes.h` | S-2 (#3) | `MessageSizeLimit` 枚举 (行 125-166) |
| `src/lib/synergy/protocol/ProtocolUtil.cpp` | S-3 (#4) | 15 处 assert→throw |
| `src/lib/synergy/input/InputValidator.h` | S-4 (#5) | 校验器（已接入客户端入站路径） |
| `src/lib/synergy/input/InputValidator.cpp` | S-4 (#5) | 同上 |
| `src/lib/client/ServerProxy.cpp` | S-4 (#5) | 入站按键事件接入校验 (keyDown/keyRepeat/keyUp) |
| `src/unittests/synergy/InputValidatorTests.cpp` | S-4 (#5) | 新增单元测试 |
| `src/lib/platform/linux/XWindowsScreen.h` | S-5 (#6) | `m_displayLost` 成员 (行 252) |
| `src/lib/platform/linux/XWindowsScreen.cpp` | S-5 (#6) | `ioErrorHandler` + 守卫 (行 1638-1651) |

### 4.2 基础设施文件
| 文件 | 用途 |
|------|------|
| `.github/workflows/ci.yml`（analyze） | clang-tidy + cppcheck |
| `CMakePresets.json` | ASan/TSan/Coverage 预设 |
| GitHub Issues（`Tupig/Tupig_synergy`） | 问题追踪（原台账 A/U/S/Q/P/T 144 条已全量迁移，2026-10-08 删除本地文件） |
| `docs/security.md` | 安全策略文档 |
| `docs/HANDOFF.md` | 本文件 — 会话交接文档 |

> 注：原 `docs/delivery.md`（交付矩阵）、`docs/consistency-audit.md`（U 条目审计）与两份
> audit 报告已按 docs 清理计划删除；交付要点迁入本文件 §5.5～§5.6，结构性约束见
> `AGENTS.md`「交付约束」，U 条目状态以 GitHub Issues 为准。

### 4.3 Phase 2 涉及文件
| 文件 | 用途 | 操作 | 状态 |
|------|------|------|------|
| `src/lib/net/SocketMultiplexer.cpp/h` | 网络事件循环 | Step 1 重构 | ✅ 已完成 |
| `src/lib/net/INetworkTransport.h` | 统一传输接口 | Step 2 新增 | ✅ 已完成 |
| `src/lib/net/LegacyNetworkTransport.cpp/h` | Legacy 包装器 | Step 2 新增 | ✅ 已完成 |
| `src/lib/net/QtNetworkTransport.cpp/h` | Qt QTcpSocket 传输（完整实现） | Step 2–3 | ✅ 已完成 |
| `src/lib/net/NetworkTransportFactory.cpp/h` | 运行时切换工厂 | Step 2 新增 | ⚠️ 未接线（A-15 (#39)：无生产调用者，见 §5.4 阶段 1） |
| `src/lib/net/IDataSocket.h` | 数据 socket 接口 | Step 2 修改 (添加 getSocket) | ✅ 已完成 |
| `src/lib/net/TCPSocket.cpp/h` | TCP 传输（回滚路径） | 保留至 N-07 稳定期 | ⏸ 回滚用 |
| `src/lib/net/SecureSocket.cpp/h` | TLS 传输（回滚路径） | 保留至 N-07 稳定期 | ⏸ 回滚用 |
| `src/lib/net/TCPSocketFactory.cpp/h` | raw 工厂（`USE_LEGACY_NETWORK=1`） | 保留至 N-07 | ⏸ 回滚用 |
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
| 5 | 网络层智能指针化（N-04：ISocketFactory 全链 unique_ptr） | ✅ 已完成 |
| 6 | 移除旧 `TCPSocket` / Multiplexer Job 等遗留代码 | ⏸ 稳定期后（N-07；当前为 `USE_LEGACY_NETWORK=1` 回滚路径） |

说明：追踪文档里的「Step 3 完成」指 QTcpSocket 集成，**不是**「已删除 TCPSocket」。二者勿混用。

### 5.2 本地环境实测（2026-10-08，Windows）
- **工具链可用**：cmake、ninja、MSVC 2022 Build Tools、gh 均可用；`scripts\build.bat release` + `ctest`（33/33）是本机权威验证路径。此前「cmake/ninja 不在 PATH、无法本地构建」的记录已过时。
- **clang-format 版本敏感**：PATH 中 LLVM 为 22.x，CI 用 20.1.0；请显式使用 20.1.0（`pip install clang-format==20.1.0`，位于 `%LOCALAPPDATA%\Programs\Python\Python3xx\Scripts\`）。
- **REUSE 校验**：`pip install reuse` 后 `python -m reuse lint`（当前 893/893 合规）。
- **vcpkg 浅克隆**：如缺 pinned baseline 需 `git -C vendor/vcpkg fetch --depth 1 origin <baseline>`；`vendor/vcpkg/downloads/` 是断点续传关键，勿删。
- **XWindowsScreen.cpp/h LSP 报错**：X11 头在 Windows 缺失所致，非代码缺陷。
- **PowerShell 注意**：`Start-Process` 带重定向转义易出错 → 后台任务用包装 `.bat`；Python 中文输出需 `PYTHONIOENCODING=utf-8`。

### 5.3 Phase 3-6 待规划
- Phase 3: Server 拆分 (Week 5-8) — 7 大模块拆分
- Phase 4: 性能优化 (Week 9-10) — 心跳隔离/异步 I/O/零拷贝
- Phase 5: 部署打包 (Week 11) — 三平台安装包
- Phase 6: 验收交付 (Week 12) — 全维度验收

### 5.4 B 计划（原 plan-B）决策与阶段状态

> 原 `docs/plan-B-D1-E2-F1-G1.md` 已删除，状态迁移至此。**阶段 1~3 已随 2026-10 结构方案批准恢复执行**（网络层 N-01～N-07，issues #139-145）；下列状态为权威记录。

已锁定决策（2026-09-23）：

| 代号 | 选择 | 状态 |
|---|---|---|
| B | Qt 默认 + Step 5 + Step 6 删除 Legacy | 阶段 1~3 暂不执行 |
| D1 | 仅保留 synergy 图标主题（deskflow SVG 以别名并入 `synergy.qrc`） | ✅ 已完成 |
| E2 | U-09 (#92) 路径有意保留（`kUpstreamId` 供上游同步 / i18n 边界） | ✅ 已关闭；**2026-10 决议更新**：内部 deskflow 命名已全仓统一为 synergy（T1~T3，issue #146），`kUpstreamId` 机制保留但其值即项目名 |
| F1 | 关闭 U-17 (#100) / U-18 (#101) / U-19 (#102)（显示名 / i18n 耦合 / 文档化） | ✅ 已完成（`1269e7cb9`） |
| G1 | 用户执行双机拖拽；agent 提供清单 | 清单见 §5.6，待用户执行 |

阶段状态：

- **阶段 0（命名/图标/文档）**: ✅ 完成 —— D1 别名折叠、U-17 (#100) 显示名 `TuPig Synergy`、U-18 (#101)（**后经 #146 决议改为全仓统一 synergy**）、U-09 (#92)/U-19 (#102) 文档化、G1 清单迁入 §5.6。
- **阶段 1（Qt 适配器 + 指纹 TOFU，A-15 (#39) 工厂接线）**: ✅ 完成 —— QtSocketFactory/QtDataSocket/QtListenSocket 适配器落地，TOFU 与 SecureSocket 对齐（客户端校验 trusted-servers、服务端 PeerAuth 校验 trusted-clients、RSA≥2048、peerFingerprint IPC），Qt5 手动 TLS 升级到位（N-05），工厂接线 apps 且默认 Qt（`USE_LEGACY_NETWORK=1` 回退）。
- **阶段 2（默认 Qt + Step 5 智能指针）**: ✅ 完成（N-04：ISocketFactory 全链 unique_ptr；默认 Qt 已切换）。
- **阶段 3（Step 6 删除 Legacy 栈）**: ⏸ 稳定期后执行（N-07 #145；当前保留为回滚路径）。

验证方式（恢复执行时适用）：每阶段 `scripts\build.bat release` + 单测 + 推送；G1 由用户按 §5.6 执行。

### 5.5 交付验证状态与 R8 风险（自 delivery.md 迁入）

**已在 Windows（Release、静态 triplet）验证：**

- 构建与打包成功，便携归档（7Z）正常产出；MSI 已构建并经 Windows Installer 数据库 API 检查（File/ServiceInstall/ServiceControl 三表），**未执行实际安装**。
- `dumpbin /DEPENDENTS` 确认不依赖 MSVC 运行库 DLL（`/MT` 静态链接，vcpkg 静态 triplet）。
- GUI→core 握手：`gui/startCoreWithGui=true` + 服务端 `coreMode` 下，GUI 约 3 秒内拉起 `synergy-core.exe`（端到端实证 U-01 (#84) 修复）。
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

将 OS、`synergy-core --version`、通过/失败记入对应 GitHub issue 或本文件。

---

## 6. 后续建议的下一步操作

### 立即执行
1. **阶段 4 回归 ✅ 已完成**（2026-10-08）：30+ 提交已推送；free 子集 run 37754662586 与全量矩阵 run 37755828508 全绿（含 macOS 腿，A-56 (#80) 随之覆盖）。剩余：批1/批2 门禁修复（static-analysis、valgrind）可用 workflow_dispatch 手动验证真实拦截能力
2. **网络层（N-01～N-07）**: test(net) 基线 → 工厂接线（闭合 A-15 (#39)）→ Qt5 手动 TLS 升级 → 默认切 Qt → Step 6 删遗留栈；详见 §5.4 与 issues #139-145
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

### 下一步: CI 已全绿 → Step 5 / B 计划阶段 1 → G1 验证
1. A-59 (#83) 已完成并实证（run `35988655131` 全绿）；A-56 (#80)/全量矩阵走手动 `workflow_dispatch` 或 release
2. 网络层智能指针化（Step 5）或 B 计划阶段 1（适配器 + TOFU + 工厂接线，闭合 A-15 (#39)；见 §5.4）
3. Windows 跨屏拖拽人工验证（清单 §5.6：IDropTarget 捕获 + IDropSource 投放到 Explorer）
4. 默认切到 Qt 传输前，用 `USE_LEGACY_NETWORK=0` 做 TLS 互通冒烟（注：工厂尚未接线，见 A-15 (#39) / §5.4 阶段 1）

---

## 7. 关键约束提醒

1. **同步更新 Issue tracking**: GitHub Issues（<https://github.com/Tupig/Tupig_synergy/issues>）必须与代码变更同步
2. **跨平台兼容**: 所有代码必须 Win/Mac/Linux 三平台编译通过
3. **中英文双语**: 所有文档必须中英文合并
4. **不上传 GitHub 的内容**: 不要出现在可上传内容里
5. **不要硬编码路径**: 使用环境变量或 CMake 查找
6. **先方案后实施**: 任何改动前先给出方案，选择最优

---

> **交接完成**。Phase 0+1 与 Phase 2 Step 1–4 已完成（含 Windows IDropSource、U-10 (#93)/U-13 (#96)/U-15 (#98)/A-10 (#34)）；P3（A-20 (#44)～A-27 (#51)）已修待 CI 实证。下一步优先下次 CI 实证，或跨屏拖拽人工验证（§5.6）。
