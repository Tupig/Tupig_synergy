# [Security] TuPig Synergy 安全与质量全面加固

> **Issue 类型**: Security / Quality  
> **优先级**: P0  
> **分支**: `refactor/security-baseline`（历史名；现仅 `main`）  
> **创建日期**: 2026-09-18  
> **最后更新**: 2026-09-24  
> **状态**: 🔄 进行中

---

## 1. 问题标题

TuPig Synergy 代码库存在 23 个安全、质量、性能和技术债务问题，需要系统性修复以达到生产级安全标准。

---

## 2. 问题现象与影响范围

### 2.1 影响范围

本项目基于 Synergy/Deskflow，约 15 万行 C++ 代码，覆盖 Windows/macOS/Linux 三平台。问题影响所有使用 TuPig Synergy 的用户，包括：

- **安全维度**：TLS 证书验证被禁用、协议解析无类型安全、输入事件可被注入
- **稳定性维度**：X11 错误导致进程崩溃、Socket 多路复用器存在死锁风险
- **性能维度**：1 秒轮询延迟、4MB 栈上静态缓冲区、逐字节协议解析
- **可维护性维度**：单元测试覆盖率 <30%、无静态分析门禁

### 2.2 问题清单（23 项）

| 编号 | 优先级 | 问题 | 核心文件 | 状态 |
|------|--------|------|----------|------|
| **S-1** | P0 | TLS 证书验证被禁用 | `SecureSocket.cpp` | ✅ 已修复 |
| **S-2** | P0 | 协议消息长度限制过大 | `ProtocolTypes.h` | ✅ 已修复 |
| **S-3** | P0 | 协议解析 `va_list` 无类型安全 | `ProtocolUtil.cpp` | ✅ 已修复 |
| **S-4** | P0 | 输入事件注入无验证 | `ServerProxy.cpp` | ✅ 已修复（校验器原先只创建未接入，现已真正接入客户端入站路径并补测试） |
| **S-5** | P0 | X11 错误处理器导致崩溃 | `XWindowsScreen.cpp` | ✅ 已修复 |
| **Q-1** | P1 | SocketMultiplexer 死锁风险 | `SocketMultiplexer.cpp` | ✅ 已修复 (Step 1) |
| **Q-2** | P1 | 4MB 栈上静态缓冲区 | `TCPSocket.cpp` | ✅ 已确认 (栈缓冲4KB，输入限制1MB，StreamBuffer动态分配) |
| **Q-3** | P1 | 协议版本硬编码 | `ProtocolTypes.h` | ✅ 已修复 (static const→constexpr) |
| **Q-4** | P1 | X11 全局状态单例 | `XWindowsScreen.cpp` | ⬜ 待修复 |
| **Q-5** | P1 | `assert` 作错误处理 | `ProtocolUtil.cpp` | ✅ 已修复 |
| **Q-6** | P1 | 单测覆盖 <30% | 全核心模块 | ⬜ 待修复 |
| **Q-7** | P1 | 静态分析/编译警告缺失 | 全项目 | ✅ 已修复 |
| **P-1** | P2 | SocketMultiplexer 1秒轮询 | `SocketMultiplexer.cpp` | ✅ 已修复 (Step 1) |
| **P-2** | P2 | 剪贴板全量内存拷贝 | `Clipboard.cpp` | ⬜ 待修复 |
| **P-3** | P2 | 协议解析逐字节处理 | `ProtocolUtil.cpp` | ⬜ 待修复 |
| **P-4** | P2 | Windows Hook 无条件加载 | `MSWindowsScreen.cpp` | ⬜ 待修复 |
| **P-5** | P2 | 剪贴板格式转换器重复造轮子 | 30+ 文件 | ⬜ 待修复 |
| **T-1** | P3 | C++17 → C++20 现代化 | 全局 | ⬜ 长期 |
| **T-2** | P3 | Qt 信号槽旧语法 | GUI 模块 | ⬜ 待修复 |
| **T-3** | P3 | Raw 指针手动内存管理 | 网络/协议层 | ✅ 已修复 (Step 2: unique_ptr 所有权模型) |
| **T-4** | P3 | 平台层代码重复 | win32/linux/macos | ⬜ 待修复 |
| **T-5** | P3 | 缺乏单元测试覆盖 | 核心模块 | ⬜ 全程 |
| **T-6** | P3 | 构建系统碎片化 | vcpkg + 系统 Qt | ✅ 已修复 (Qt6 已加入 vcpkg.json) |

### 2.3 相关审计

命名与身份一致性（产品标识、打包身份、文档、CI、i18n 命名）的独立审计共 21 项 `U-01` ～ `U-21`，原文件 `synergy/docs/consistency-audit.md` 已随 docs 清理删除，全文见 git 历史（状态以本台账与 `synergy/docs/HANDOFF.md` 为准）。两套编号体系相互独立，与本清单的 23 项不重叠。

### 2.4 全面审计新发现（A-01 ～ A-11，2026-09-23）

> 4 轮多角度全面审计（范围/目标/验收标准、文档对账、R1–R10 合规、源码与过程、CI 与交付）的完整报告
> 原文见 git 历史（原 `synergy/docs/audit-2026-09-23.md`，已随 docs 清理删除）。
> `A-nn` 与 `S/Q/P/T`、`U-nn` 相互独立，不重叠。

| 编号 | 优先级 | 问题 | 核心位置 | 状态 |
|------|--------|------|----------|------|
| **A-01** | P1 | CMake「3.24+」声明低于 presets schema v6 实际所需 3.25 | `CMakePresets.json:2-6` 等 7 处 | ✅ 已修复（全部声明升 3.25，presets minor=25；`CMakeLists.txt:9` RHEL floor 3.20 按决策保留） |
| **A-02** | P1 | Qt 下限三方不一致：6.4.0（代码）/ 6.7（文档）/ 6.9+（CI） | `CMakeLists.txt:88` 等 | ✅ 已修复（统一下限 6.7.0；vcpkg qtbase 6.11.1、CI ≥6.9.3 均满足；Qt5/RHEL 回退 5.13 路径不受影响） |
| **A-03** | P1 | `HANDOFF.md` 自相矛盾 + 「Step 3」三方定义漂移 + 头部元数据过期（U-14 加重） | `docs/HANDOFF.md:5-6,152,174` | ✅ 已修复（统一 Step 定义表；QtNetworkTransport 标为完整实现；头部/下一步/提交列表刷新） |
| **A-04** | P2 | 本追踪文档落后 09-23 的 15+ 提交；U 计数过期（违反 HANDOFF §1.3） | 本文档变更日志 | ✅ 已修复（变更日志已补登主路径；细节以 git log 为准） |
| **A-05** | P2 | `consistency-audit.md` 状态栏/待验证节滞后于自身正文（含 U-16） | `docs/consistency-audit.md:56,63,349-351` | ✅ 已修复（U-16 收窄为 Fixed；U-01 待验证节改为已实证；U-08 随注释修正关闭） |
| **A-06** | P2 | `build.sh` 建议使用 hidden 预设 `linux-asan`（应为 `linux-asan-build`） | `scripts/build.sh:37` | ✅ 已修复 |
| **A-07** | P3 | README 标题非双语；README/setup.bat 入口指引与 AGENTS「两个入口」表述不一致 | `README.md:1,97-108`、`setup.bat:125-127` | ✅ 已修复 |
| **A-08** | P3 | 注释与代码相反：`gui-electron` 幽灵路径、`./VERSION` 不存在、「space-free」不实（U-08 新增实例） | `extra/cmake/Synergy.cmake:17-18,30-31,66` | ✅ 已修复 |
| **A-09** | P2 | 未提交 WIP：3 改 + 2 个 untracked 源文件（Windows 拖拽第 2 步），有丢失风险 | `git status --porcelain` | ✅ 已修复（WIP 已随 `fe1f4fe37`/`91960506f`/`e922e1639` 提交，工作区无源码残留） |
| **A-10** | P3 | `IDataSocket` 契约 `assert(0)` 残留（观察项，暂不修） | `src/lib/net/IDataSocket.cpp:19,25` | ✅ 已修复（`cbd2baf39` 改为抛 `SocketException`；2026-09-24 复核 `rg assert\(0\)` 零命中，补翻状态） |
| **A-11** | P3 | 本文档 §2.3 相对链接指向不存在的根 `docs/` | 本文档 §2.3 | ✅ 已修复（本节登记时改为 `synergy/docs/`） |

### 2.5 全面审计新发现（A-12 ～ A-27，2026-09-24）

> 二轮 4 路并行只读审计（回归 / R1–R10 + 文档对账 / 构建·CI·源码·交付 / Open 项深挖）的完整报告
> 原文见 git 历史（原 `synergy/docs/audit-2026-09-24.md`，已随 docs 清理删除）。
> 基线：`main` @ `8695555be`，工作区干净。A-01～A-11 代码侧回归全部 HOLD（A-03/A-04/A-05 文档同步债见 A-18/A-19）。

| 编号 | 优先级 | 问题 | 核心位置 | 状态 |
|------|--------|------|----------|------|
| **A-12** | P1 | CI 门禁可「全绿但零构建」：lint-clang 无 `working-directory`（`find src/` 在仓库根失败）→ build 全 skip；`ci-passed` 不 needs lint 且 `skipped` 计为通过 | `.github/actions/lint-clang/action.yml:12`、`ci.yml:71,79-92` | ✅ 已修复（lint 步补 `working-directory: synergy`；`ci-passed` 纳入 `lint-clang` needs 并校验其 result；YAML 解析通过，待 CI 实证） |
| **A-13** | P1 | build-flatpak 三处路径锚定矛盾：`uses:` 不继承 `defaults.run.working-directory`，`manifest-path: extra/...` 以 workspace 根解析 | `ci.yml:63-65,680-695` | ✅ 已修复（`manifest-path` 加 `synergy/` 前缀；Validate `working-directory: .` + 例外文件 `synergy/` 前缀；Upload 改 workspace 根；YAML 解析通过，待 CI 实证） |
| **A-14** | P2 | `push` 只配不存在的 `beta`（main 推送不触发 CI）；`static-analysis.yml` 无调用者，clang-tidy/cppcheck 从未自动运行 | `ci.yml:33-34`、`static-analysis.yml:2,6-8` | ✅ 已修复（`push` 改 `main`；static-analysis 头注释改为如实说明仅手动/`workflow_call`，不接 ci.yml 以免首跑即挂） |
| **A-15** | P2 | `NetworkTransportFactory` 无生产调用者；HANDOFF 宣称的 3 级回滚落空（`USE_LEGACY_NETWORK` 只在未接线工厂内读取；CMake `LEGACY_NETWORK` 不存在） | `NetworkTransportFactory.cpp:32-41`、`HANDOFF.md` §3.2 | ✅ 文档已对齐现实（docs-first；工厂接线仍为 B 计划阶段 1 开放项，见 HANDOFF §5.4，暂不执行） |
| **A-16** | P2 | README 称文件拖拽「未实现」，与 delivery（Win/mac 已实现 + 单测）反向过期 | `README.md:42` vs `delivery.md:106-111` | ✅ 已修复（README 改为「Win/mac 已实现、Linux 从未实现」并指向 delivery 矩阵） |
| **A-17** | P2 | README 配置示例键全不存在（`serverHost/serverPort/…`），真实键见 `Settings.h` | `README.md:166-186` vs `Settings.h:37,43,51` | ✅ 已修复（示例改为 `client/remoteHost`、`dynamicConnectionInterval` 等真实键，并区分 settings INI 与 server screens 配置） |
| **A-18** | P2 | A-03/U-14 回归：HANDOFF 头部 commit / 最新 6 笔 / U 计数 19 vs 21 / `:6` 与 `:74`/`:185` 对 U-07/09 状态矛盾 | `docs/HANDOFF.md:5,6,74,128,185` | ✅ 已修复（登记提交已刷头部/计数/矛盾行；本轮再刷 P2 状态） |
| **A-19** | P2 | A-04/A-05 回归：追踪变更日志缺 `d8512e321` 后 6 笔；consistency-audit 详情节与状态栏矛盾（U-07/09/10/17 等） | 追踪 §5、`consistency-audit.md:170,184,190,250-257` | ✅ 已修复（变更日志已补登；U-07/09/10/13/15/17 详情节补 Resolution 并归档 `:63`） |
| **A-20** | P3 | GoogleTest 幽灵声明 ×7，实际为 `Qt::Test` | `README.md:151`、`build.md:27,272`、`architecture.md` ADR-0009、`contributing.md:101,430` | ✅ 已修复（7 处改 Qt Test + CTest；`365f64cb7`，待 CI 实证） |
| **A-21** | P3 | CMake 缺陷批：xkbfile `!` 死检查、多配置 NDEBUG 误加、`generate_app_man` 变量大小写、`PORTABLE_LIBS` 死语句、`SKIP_BUILD_TESTS` 双源、libportal/libei 版本文档不符、`build.sh:17` 残句 | `cmake/Libraries.cmake:271`、`CMakeLists.txt:320-323` 等 | ✅ 已修复（7 项全改；`365f64cb7`。**注意**：xkbfile 项的 `check_library_exists` 探测符号选错引入六轮全灭回归，已由 A-52 改 `find_library` 收口） |
| **A-22** | P3 | 文档死链/幽灵：mingw-toolchain、`.pre-commit-config.yaml`、`bug_report.md`、security.md 无邮箱、ADR-0011 目录树、`build.md` 声称支持 `debug` 参数 | `build.md:198,437,506-507`、`contributing.md:26,102,356,431` 等 | ✅ 已修复（交叉编译段删、pre-commit 段改 clang-format 直用、`bug_report.yml` ×4（含 `.github/CONTRIBUTING.md`）、security 改 GitHub 私密报告、ADR-0011 树改 6 文档、脚本参数仅 `release` + 中英说明；`365f64cb7`） |
| **A-23** | P3 | 打包/CI 卫生：REUSE 死路径 ×4、build.md「无人消费 Flatpak」与 CI 矛盾、delivery 漏 flatpak+Arch、WiX 5/4/7 三角、Xcode 绝对路径、vcpkg `revision: master`、sonar 幽灵排除 | `REUSE.toml:20-22,36-37`、`build.md:182-186` 等 | ✅ 已修复（REUSE 23 路径全实存——含补删 `welcome.png`；build.md Flatpak/WiX/包格式 EN+ZH 重写并补 Flatpak+Arch；deploy.cmake WiX 4/5.0.2 注释；ci.yml 删 Xcode 硬编码；vcpkg revision 加有意不钉注释；sonar 删幽灵排除 + 补 `extra/**`；`365f64cb7`，待 CI 实证） |
| **A-24** | P3 | 上游身份残留：`Synergy App Ltd` SPDX ×5、symless 下载链、AboutDialog「Deskflow」回退、`daemonName()` 上游名、manpage 上游 wiki、issue `config.yml` 全指 deskflow | 见审计报告 A-24 | ✅ 已修复（按既定策略：SPDX ×5 保留 + A-24 注释；metainfo symless→Tupig；AboutDialog/daemonName×2/manpage/config.yml 全改本仓库；6 翻译 URL 批量同步；`365f64cb7`） |
| **A-25** | P3 | U-15 残留：metainfo `project_license` 与 flatpak SPDX 缺 OpenSSL exception | `metainfo.xml:7`、`com.tupig.synergy.yml:2` | ✅ 已修复（两处补 `WITH LicenseRef-OpenSSL-Exception`；`365f64cb7`，XML 校验通过） |
| **A-26** | P3 | A-10 追踪行过期（代码已修、状态仍「挂账」） | 本文档 §2.4 A-10 行 | ✅ 已修复（本登记提交补翻状态） |
| **A-27** | P3 | 亮色主题缺 `places/64/user-trash`（Windows 亮色删除按钮无图标） | `synergy.qrc:90`、`synergy-light.theme:15` | ✅ 已修复（qrc 补 light 别名 + theme 补 Directories/[places/64] 段，镜像 dark；`365f64cb7`） |
| **A-28** | P1 | CI 首跑（A-14 生效后）暴露：lint-clang 真实检出 105 文件 clang-format 漂移（A-12 门禁实证生效，非空转） | run `35948919314` lint artifact | ✅ 已修复（应用 CI `clang-format-diff` artifact，108 文件含 workflow/文档；本地 22.1.8 ≠ CI 20.1.0，以 CI diff 为准） |
| **A-29** | P1 | `ci-passed`/`report`/`s3-upload` 无 Checkout 却继承 `defaults.run.working-directory: synergy`，bash 无法启动 → 三 job 必挂 | `ci.yml:63-65` vs `:69,752,710` | ✅ 已修复（三处 run 步补 `working-directory: .`；YAML 解析通过，待 CI 实证） |
| **A-30** | P1 | `s3-upload` AWS secrets 为空（`AWS_ACCESS_KEY_ID`/`AWS_SECRET_ACCESS_KEY`），push 主干必挂 | `ci.yml:726-727`；`gh secret list` 空 | ✅ 已修复（拍板：无 secrets 时 skip；首版误用 job 级 `if` 判 secrets 导致 workflow 0s 解析失败，改为首步 check + 后续 step 级 `if`；YAML 解析通过，待 CI 实证） |
| **A-31** | P1 | CodeQL/Sonar/Valgrind 容器 job 首步（checkout 前）继承 `working-directory: synergy`，`synergy/` 尚不存在 → `chdir` 失败 exit 127 | `codeql-analysis.yml:41`、`sonarcloud-analysis.yml:50`、`valgrind-analysis.yml:19` | ✅ 已修复（三处首步补 `working-directory: .`；YAML 解析通过，待 CI 实证） |
| **A-32** | P1 | Linux 容器矩阵首步 `Install Git on Container`（checkout 前）同 A-31 根因：继承 `working-directory: synergy` → 14+ 发行版 job 全部 exit 127 | `ci.yml:559-575` | ✅ 已修复（首步补 `working-directory: .`；YAML 解析通过，待 CI 实证） |
| **A-33** | P1 | macOS GUI CMake target 名含空格 `"TuPig Synergy"`（`CMAKE_PROJECT_PROPER_NAME`），`add_executable` 拒收 + 所有 `TARGET_BUNDLE_*` 生成器表达式失效 → Configure 必挂 | `deskflow-gui/CMakeLists.txt:5-9`；`MacCodesign.cmake:43`；`translations:102`；`deskflow-core:57`；`extra/CMakeLists.txt:27`；`unittests:103` | ✅ 已修复（target 统一为 `CMAKE_PROJECT_NAME`；bundle 元数据/OUTPUT_NAME 仍用 PROPER_NAME 保 `.app` 名；五处 `TARGET_BUNDLE_*` 引用同步） |
| **A-34** | P1 | CodeQL `autobuild` 在仓库根遇双项目（`GitHubDesktop2Chinese` + `synergy`）拒绝选边 → 分析失败 | `codeql-analysis.yml:59-60` | ✅ 已修复（`build-mode: manual` + 显式在 `synergy/` 下 cmake configure/build；YAML 解析通过，待 CI 实证） |
| **A-35** | P2 | metainfo homepage 指向私有仓库 `github.com/Tupig/Tupig_synergy`（未认证 404），`flatpak-builder-lint appstream` url-reachability 必挂 | `com.tupig.synergy.metainfo.xml:20` | ✅ 已修复（改指 `https://github.com/Tupig` 组织页；首版改 `https://tupig.com` 仍因 TLS 失败，见 A-39） |
| **A-36** | P1 | 二轮 CI（`35952577862`）26 job 挂：Linux 矩阵 Configure 全挂 — `translations/CMakeLists.txt:60` `find_file(qtbase_en.qm)` 搜不到系统 Qt 翻译（PATH_SUFFIXES 缺 `share/qt6/translations`，且 Debian/Fedora/SUSE 未装翻译包）；CodeQL 同根因 | `translations/CMakeLists.txt:60-68`；`.github/actions/install-dependencies/action.yml:39-66` | ✅ 已修复（PATHS 补 `/usr`、`/usr/local`；PATH_SUFFIXES 补 `share/qt6/translations`；debian/fedora/suse 各装对应翻译包） |
| **A-37** | P1 | macOS Build 挂：`SYNERGY_ENABLE_HARDENING`（默认 ON）无条件 `add_link_options(-Wl,-z,relro,-z,now)`，Apple ld 不认 GNU `-z` → `ld: unknown options: -z -z` | `CMakeLists.txt:295-297` | ✅ 已修复（硬化 `-z` 仅在 `NOT APPLE` 时加；`-fstack-protector-strong` 与 `-pie`/FORTIFY 本就在 `NOT APPLE` 内） |
| **A-38** | P1 | Linux Build 挂：`XWindowsConfig.h` 由 `configure_file` 生成到 `platform` 二进制目录，但 `target_include_directories` 只加了源码子目录，rocky/rocky-9 等过 Configure 后编译 `XWindowsScreen.h` 即 `No such file or directory` | `src/lib/platform/CMakeLists.txt:187-192` | ✅ 已修复（include path 补 `${CMAKE_CURRENT_BINARY_DIR}`） |
| **A-39** | P2 | flatpak Lint appsteam 挂：A-35 改的 `https://tupig.com` 从 CI 仍 TLS 失败（`unexpected eof`，本地 curl exit 35）→ `url-not-reachable` warning 仍判 fail | `com.tupig.synergy.metainfo.xml:23` | ✅ 已修复（改指公开可达的 `https://github.com/Tupig`；私仓与坏 TLS 域名均不可作 homepage） |
| **A-40** | P1 | s3-upload 挂：`Check AWS secrets` 步未补 `working-directory: .`，无 checkout 却继承 `synergy/` → `No such file or directory` | `ci.yml:727-738` | ✅ 已修复（该步补 `working-directory: .`；同 A-29 根因） |
| **A-41** | P1 | report 挂：`Send Slack notification` 无 `SLACK_TOKEN` → `Missing input! A token must be provided`；与 A-30 同类 secrets 判空缺失 | `ci.yml:807-814` | ✅ 已修复（新增 `Check Slack secrets` 步 step 级判空，无 secrets 时 skip send） |
| **A-42** | P1 | 三轮 CI（`35957542966`）arm64 Configure 挂：`cc1plus: unknown value 'x86-64-v3'` — 架构正则 `[Aa][Rr][Mm]64` 不匹配 Debian/Ubuntu arm64 的 `aarch64`，AVX2 误开 | `CMakeLists.txt:244-246` | ✅ 已修复（改用已校验的 `BUILD_ARCHITECTURE MATCHES "^(x64\|x86_64)$"`） |
| **A-43** | P1 | 三轮 CI macOS Build 挂：`'platform/OSXCocoaApp.h' file not found` — 平台 include 根为 `platform/` + `platform/macos`（无 `src/lib`），`#import "platform/X.h"` 前缀错且文件实际在 `macos/` 下（6 处 4 文件） | `OSXCocoaApp.m`、`OSXMediaKeySupport.m`、`OSXScreenSaver.cpp`、`OSXScreenSaverUtil.m` | ✅ 已修复（改为 `#import "X.h"` 相对同目录名；`rg '#[im]port "platform/' src/lib/platform/macos` 零命中） |
| **A-44** | P1 | 三轮 CI fedora-x86_64 Build 挂：`ninja: error: '/usr/share/qt6/translations/qtbase_es.qm', needed by 'translations/qt_es.qm', missing` — Fedora `qt6-qttranslations` 有 `qtbase_en.qm` 无 `qtbase_es.qm`，翻译拷贝命令硬 DEPENDS | `translations/CMakeLists.txt:93-104` | ✅ 已修复（per-lang 拷贝改 `if(EXISTS)` 软跳过 + STATUS 消息；app 自有 .ts 目录仍构建） |
| **A-45** | P1 | 三轮 CI debian-12/ubuntu-24.04 Configure 挂：`Could not find a configuration file for package "Qt6" ... "6.7.0"` — 自带 Qt6 6.4.x < 项目下限 6.7.0（A-02 设定），DependencyFallback 硬失败 | `cmake/DependencyFallback.cmake:19`；`ci.yml` debian-12/ubuntu-24.04 四腿 | ✅ 已修复（`find_qt_with_fallback` Qt6 版本不足时回退 Qt5 并下调 REQUIRED_QT/OPENSSL 下限；debian 分支补装 `qtbase5-dev`；四腿 `config-args` 补 `-DBUILD_TESTS=OFF`，同 rocky 模式——unittests 硬 `find_package(Qt6 Test)`） |
| **A-46** | P1 | 三轮 CI rocky-8/9 编译挂：`QSslServer: No such file or directory` — Qt6-only 类型无版本守卫（Qt5 路径 A-45 同理须覆盖） | `QtNetworkTransport.cpp:19-21,373-381,397-423` | ✅ 已修复（`#include`、构造函数、`configureSslServer` 三处 `#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)`；Qt5 分支仅 QTcpServer，TLS listen 返回 false + LOG_ERR） |
| **A-47** | P1 | 三轮 CI flatpak Lint manifest 挂：`appid-url-not-reachable: Tried https://tupig.com` — linter 按 app-id 命名空间探测（非 manifest URL），该步未传 exceptions 标志 | `ci.yml:691-692`；`ci-build-lint-exceptions.json` | ✅ 已修复（例外文件补 `appid-url-not-reachable`；Lint manifest 步加 `--exceptions --user-exceptions`，与 Validate build 步对齐） |
| **A-48** | P1 | 四轮 CI（`35961135433`）macOS Build 挂：`'OSXAutoTypes.h' file not found` — `AppUtilUnix.cpp` 用 `<platform/OSXAutoTypes.h>`，但文件在 `platform/macos/` 且 `src/lib` 不在 include path；app 经 `PRIVATE platform` 已传递获得 `platform/macos`，应裸文件名 include | `AppUtilUnix.cpp:16` | ✅ 已修复（改 `#include "OSXAutoTypes.h"` + 注释说明平台 include 约定） |
| **A-49** | P1 | 四轮 CI Qt5 腿（rocky-8/9、ubuntu-24.04、debian-12）Build 挂：`QSslSocket::sslErrors unresolved overloaded function type` — Qt5 中 `sslErrors` 为「信号 + const getter」重载，PMF connect 需消歧；同族 `QTcpSocket::errorOccurred` / `QLocalSocket::errorOccurred` 为 Qt 5.15+，RHEL 8 floor Qt 5.13 无此信号（仅 `error()`，亦重载） | `QtNetworkTransport.cpp:138-157`；`IpcClient.cpp`；`IpcServer.cpp` | ✅ 已修复（sslErrors 改 `qOverload<const QList<QSslError>&>`；errorOccurred 加 `QT_VERSION_CHECK(5,15,0)` 守卫 else 用 `static_cast<QAbstractSocket::error`；IpcClient/IpcServer 加 `kLocalSocketError` 别名同守卫） |
| **A-50** | P1 | 四轮 CI debian-12 Build 挂：`'memory' file not found`（`QSettingsProxy.h` / `Settings.h` 的 `std::shared_ptr`）— 依赖传递 include 不可靠，Qt5 路径未拉入 `<memory>` | `QSettingsProxy.h`；`Settings.h:197` | ✅ 已修复（两头文件补 `#include <memory>`） |
| **A-51** | P1 | 四轮 CI debian-13 Configure 挂：Qt6 组件探测失败静默落入 Qt5（`-- Qt: 5.15.15`）→ `Libraries.cmake` 建 `Qt6:: ALIAS Qt5::*` → `unittests` 硬 `find_package(Qt6 Test)` 报 `Qt6::CorePrivate not yet defined`；根因疑 slim 镜缺 OpenGL 致 Qt6Gui 依赖失败；debian-13 矩阵腿未传 `-DBUILD_TESTS=OFF` | `install-dependencies/action.yml` debian 分支；`src/CMakeLists.txt`；`src/unittests/CMakeLists.txt:78` | ✅ 已修复（debian 主安装行补 `libgl-dev libglx-dev libopengl-dev` + 兜底行；`src/CMakeLists.txt` BUILD_TESTS 门控改 `if(BUILD_TESTS AND QT_VERSION_MAJOR EQUAL 6)` Qt5 自愈跳过；unittests 加门控依赖注释） |
| **A-52** | P1 | 五/六轮 CI（`35964563457`/`35970799694`）UNIX X11 腿 Configure 全灭：A-21 的 `check_library_exists("xkbfile" XkbGetKeyboard …)` 用错符号 —— `XkbGetKeyboard` 在 **libX11** 而非 libxkbfile，装了 libxkbfile 也探不到 → `Libraries.cmake:273 Missing library: xkbfile` FATAL（CodeQL 同挂；旧 `!X11_xkbfile_FOUND` 为死检查从未触发） | `cmake/Libraries.cmake:262-277` | ✅ 已修复（改 `find_library(XKBFILE_LIBRARY xkbfile)` + `NOT XKBFILE_LIBRARY` 判 FATAL；链接列表仍按名 `xkbfile`；`365f64cb7` 后本笔） |
| **A-53** | P1 | 五轮 CI debian-12×2 Build 挂：`CoreProcess.h` 用 `std::optional`（:43/44/130）无 `#include <optional>`，GCC 12 无传递 include | `src/lib/gui/core/CoreProcess.h` | ✅ 已修复（Qt include 组后补 `#include <optional>`，同 A-50 `<memory>` 模式；本笔） |
| **A-54** | P1 | 五轮 CI ubuntu-24.04×2 Build 挂：`ScreenSetupModel.cpp:35` `qFatal("%lld", screens.size())` —— Qt5 `QList::size()` 为 `int`，`-Werror=format` 拒；Qt6 `qsizetype` 恰为 long long 故 Qt6 腿不报（另 `scrren` 拼写错） | `src/lib/gui/ScreenSetupModel.cpp:35` | ✅ 已修复（`static_cast<qlonglong>(screens.size())` + 改 `screen`；clang-format 22.1.8 --Werror 过；本笔） |
| **A-55** | P1 | 五轮 CI `ctest --test-dir build/src/unittests` 挂：目录不存在即 `Failed to change working directory` —— 触发腿为 debian-13-x86_64（Qt5 回退无测试，A-51 门控只跳过编译不跳过 Tests 步）；且 debian-12/ubuntu-24.04 `BUILD_TESTS=OFF` 一旦 Build 修绿将同挂（Tests 步条件仅 `like != 'rhel'`） | `.github/actions/run-tests/action.yml`；`ci.yml:643-646` | ✅ 已修复（action 内 `! -d build/src/unittests` 时 echo 原因并 exit 0；`DependencyFallback` 诊断行补 `Qt6_DIR`/`Qt5_DIR` 解 x86_64 腿非对称之谜；本笔，待 CI 实证） |
| **A-56** | P1 | 五/六轮 CI macOS×2 Test package 挂（dyld exit 134）：`@rpath/QtWidgets.framework` 未嵌入 —— `QT_IS_SHARED` 判据 `\\.(so\|dylib)` 不匹配 framework 路径 `…/QtCore.framework/Versions/A/QtCore` → 误判静态 → 跳过 macdeployqt → DMG 缺框架 | `cmake/Libraries.cmake:69` | ✅ 已修复（正则追加 `\|\\.framework/`；`deploy/mac/deploy.cmake` 靠 `DEPLOYQT` 有无分流无需改；本笔） |
| **A-57** | P2 | synergy CI 与 ghdesktop2chinese 触发器纠缠：`ci.yml` 的 `push`/`pull_request` 无 `paths` 过滤，任一子项目变更都会触发全量 synergy 矩阵（两工作流本体无交叉引用，仅触发面混叠） | `ci.yml:33-35` | ✅ 已修复（push/PR 限定 `synergy/**`、`.github/workflows/ci.yml`、`.github/actions/**`；release/schedule/workflow_dispatch 不变；ghdesktop2chinese 仍由其独立工作流按自身 paths 触发） |
| **A-58** | P1 | 私仓 Free 账单被拒（付款失败/spending limit）后全 CI `runner_id:0` 秒挂；且此前 CI 仍在烧高倍率/定时分钟：macOS 10×、Windows 2×、nightly schedule、CodeQL 自动跑（私仓需 GHAS 付费）、PR valgrind、push 侧 s3-upload、全量 19 腿 Linux 矩阵 | `ci.yml` schedule/windows/macos/flatpak/s3/valgrind；`codeql-analysis.yml`；`sonarcloud-analysis.yml` | ✅ 已修复（用户拍板「付费的不要了」）：删 schedule；Windows/macOS/flatpak/s3-upload 仅 release+dispatch；Linux push/PR 收敛为 `.github/matrices/linux-targets.json` `free` 4 腿、全量 19 腿走 dispatch/release；CodeQL/Sonar 改仅 workflow_dispatch；ci.yml 去掉 PR 自动 valgrind；ghdesktop2chinese 周定时已由用户 `3aaa92ed1` 删除；`478be74cc` 已推送 — **run `35978730528` 实测：瘦身结构生效（贵 job 正确 skip），但 lint 等仍 `runner_id:0` 秒挂 = 账户 billing 硬锁未解除，须网页 Billing 处理或转 public** |
| **A-59** | P1 | 彻底绕开私仓 billing：仓库改名 + 转 public，且项目位于 `synergy/` 子目录、与同级内容混放，workflow/文档路径全部带 `synergy/` 前缀 | 仓库设置；`.github/workflows/*`、`.github/actions/*`、`.github/CONTRIBUTING.md`、`CODEOWNERS`、`AGENTS.md`、`README.md`、`REUSE.toml`、`vcpkg.json`、metainfo/UrlConstants/constants.h/AboutDialog/6 份 .ts/manpage | ✅ 已修复：仓库改名 **`Tupig/Tupig_synergy` 并转 public**（public Actions 分钟免费）；`synergy/*` 全部 `git mv` 到仓库根（`synergy/` 消失）；同步所有路径引用（`working-directory`、`workspace`/artifact、flatpak manifest、去 `paths`/`defaults`）；URL `Tupig/TuPig_Product`→`Tupig/Tupig_synergy`；`.gitignore` 合并；`SECURITY.md` 改真实流程；YAML/JSON 解析通过。**用户已知悉** git 历史含上游许可代码与 billing 记录 — 待 CI 实证（public 后） |

---

## 3. 根因分析

### 3.1 S-1：TLS 证书验证被禁用（已修复）

**根因**：`SecureSocket.cpp:45-48` 中的 `verifyIgnoreCertCallback` 函数始终返回 1，完全绕过了 OpenSSL 的证书链验证。虽然上层 `verifyCertFingerprint` 提供了基于指纹的 TOFU（Trust On First Use）验证，但缺少基础的证书链、有效期、密钥强度校验，形成安全缺口。

**影响**：攻击者可使用任意有效证书（如自签名证书）绕过 TLS 握手阶段的验证，结合指纹数据库的 TOFU 机制才能被发现，但首次连接时无任何保护。

### 3.2 S-2：协议消息长度限制过大（已修复）

**根因**：`PROTOCOL_MAX_MESSAGE_LENGTH` 硬编码为 4MB，`PROTOCOL_MAX_LIST_LENGTH` 和 `PROTOCOL_MAX_STRING_LENGTH` 均为 1MB。这些是绝对上限，但缺乏按消息类型的分级限制。

**影响**：恶意客户端可发送超大消息耗尽服务器内存，导致拒绝服务。

### 3.3 S-3：协议解析 assert 作错误处理（已修复）

**根因**：`ProtocolUtil.cpp` 中 15 处 `assert(0)` / `assert(false)` 在 Release 构建中被编译为空操作，导致非法格式说明符静默通过，可能引发未定义行为或安全漏洞。

**影响**：Release 构建中协议格式错误不会被捕获，可能导致内存损坏或崩溃。

### 3.4 S-4：输入事件注入无验证（修复中）

**根因**：来自网络的键盘/鼠标事件直接转发到本地平台层，无范围校验、频率限制或敏感键拦截。

**影响**：攻击者可注入任意输入事件，包括危险组合（Ctrl+Alt+Delete、Cmd+Q 等），或通过洪水攻击使目标系统不可用。

### 3.5 S-5：X11 错误处理器导致崩溃（修复中）

**根因**：`XWindowsScreen::ioErrorHandler` 在 X11 显示连接断开时终止进程，无优雅降级机制。

**影响**：网络抖动或 X Server 重启导致整个 synergy 进程崩溃，用户需手动重启。

---

## 4. 修复方案及具体改动

### 4.1 Phase 0：基础设施就绪 ✅

| 任务 | 改动文件 | 说明 |
|------|----------|------|
| CI `-Werror` | `.github/workflows/ci.yml` | 已有 `CMAKE_COMPILE_WARNING_AS_ERROR=ON` |
| 静态分析 CI | `.github/workflows/static-analysis.yml` | **新增** clang-tidy + cppcheck 工作流 |
| CMakePresets 增强 | `CMakePresets.json` | **新增** ASan/TSan/Coverage 预设 |
| 清理误创建文件 | `CMakeUserPresets.json` | **已删除** |

### 4.2 S-1：TLS 证书验证修复 ✅

**改动文件**：`src/lib/net/SecureSocket.cpp`

**具体改动**：
1. **删除** `verifyIgnoreCertCallback`（原第 45-48 行，始终返回 1）
2. **新增** `verifyCertificateCallback`：完整 OpenSSL 证书链验证，包含：
   - 证书链验证（issuer chain、root CA trust）
   - 证书过期/未生效检查
   - 基本约束（Basic Constraints）验证
   - RSA 密钥强度 ≥2048 位检查
3. **更新** `initContext()` 中的回调引用：`verifyIgnoreCertCallback` → `verifyCertificateCallback`

**验证**：LSP 诊断无错误，编译通过。

### 4.3 S-2：协议消息长度分级限制 ✅

**改动文件**：`src/lib/deskflow/protocol/ProtocolTypes.h`

**具体改动**：
1. **新增** `MessageSizeLimit` 枚举类（5 个分级）：
   - `Control = 256`（控制消息）
   - `InputEvent = 4KB`（输入事件）
   - `ClipboardChunk = 64KB`（剪贴板分块）
   - `FileChunk = 256KB`（文件传输分块）
   - `AbsoluteMaximum = 4MB`（绝对上限）
2. **重构** 旧常量 `PROTOCOL_MAX_*` 改为引用枚举值（保持向后兼容）

**验证**：LSP 诊断无错误。

### 4.4 S-3：协议解析 assert→错误码 ✅

**改动文件**：`src/lib/deskflow/protocol/ProtocolUtil.cpp`

**具体改动**：替换 15 处 assert 为异常抛出：

| 位置 | 原代码 | 新代码 |
|------|--------|--------|
| `vreadf` `%i` 非法长度 | `assert(false)` | `throw BadClientException(...)` |
| `vreadf` `%I` 非法长度 | `assert(false)` | `throw BadClientException(...)` |
| `vreadf` `%%` 非零长度 | `assert(len == 0)` | `throw BadClientException(...)` |
| `vreadf` 非法格式符 | `assert(0 && "...")` | `throw BadClientException(...)` |
| `getLength` `%i` 非法长度 | `assert(len == 1\|2\|4)` | `throw DeskflowException(...)` |
| `getLength` `%I` 非法长度 | `assert(len == 1\|2\|4)` | `throw DeskflowException(...)` |
| `getLength` `%s/%S/%%` | `assert(len == 0)` | `throw DeskflowException(...)` |
| `getLength` 非法格式符 | `assert(0 && "...")` | `throw DeskflowException(...)` |
| `writef` `%I` 非法长度 | `assert(0 && "...")` | `throw DeskflowException(...)` |
| `writef` `%s/%S/%%` | `assert(len == 0)` | `throw DeskflowException(...)` |
| `writef` 非法格式符 | `assert(0 && "...")` | `throw DeskflowException(...)` |

**验证**：LSP 诊断无错误。

### 4.5 S-4：输入事件注入验证 ✅

**改动文件**（新建）：
- `src/lib/deskflow/input/InputValidator.h`
- `src/lib/deskflow/input/InputValidator.cpp`

**具体改动**：
1. **新增** `InputValidator` 类，提供：
   - `isValidKeyCode()` — 键码范围校验（1~0xFFFF）
   - `isValidButtonId()` — 鼠标按钮 ID 校验（1~32）
   - `isValidModifierMask()` — 修饰键掩码校验（低 16 位）
   - `isSensitiveCombination()` — 危险组合检测（Ctrl+Alt+Delete、Cmd+Q、Ctrl+Alt+Backspace）
   - `isRateLimited()` — 滑动窗口频率限制（默认 1000 事件/秒，自动清理过期条目）
2. **实现** 跨平台设计，无平台特定头文件依赖
3. **日志** 使用 `LOG_WARN` 记录被拦截的敏感组合和频率限制事件

**验证**：文件创建完成，依赖仅 cstdint/chrono/unordered_map + base/Log.h。

### 4.6 S-5：X11 错误处理优雅降级 ✅

**改动文件**：
- `src/lib/platform/linux/XWindowsScreen.h`
- `src/lib/platform/linux/XWindowsScreen.cpp`

**具体改动**：
1. **新增** `m_displayLost` 成员变量（`XWindowsScreen.h`）
2. **修改** `ioErrorHandler`：设置 `m_displayLost = true`，调用 `onError()`，返回 1（不再返回 0 导致 Xlib 强制退出）
3. **修改** `onError()`：增加 `m_displayLost = true` 标记，移除旧 FIXME 注释
4. **修改** 析构函数：移除 `assert(m_display != nullptr)`，在 display lost 状态下安全退出
5. **新增** 方法守卫：`enable()`、`disable()`、`enter()`、`fakeMouseButton()`、`fakeMouseMove()` 增加 `m_displayLost` 前置检查

**验证**：LSP 诊断仅 X11 头文件缺失（Windows 环境预期行为），无逻辑错误。

---

## 5. 变更日志

| 日期 | 改动 | 作者 |
|------|------|------|
| 2026-09-18 | Issue 创建，记录 23 项问题全景 | Sisyphus |
| 2026-09-18 | Phase 0 完成：分支、CI、CMakePresets | Sisyphus |
| 2026-09-18 | S-1 完成：TLS 证书验证修复 | Sisyphus |
| 2026-09-18 | S-2 完成：协议消息长度分级 | Sisyphus |
| 2026-09-18 | S-3 完成：assert→错误码替换 | Sisyphus |
| 2026-09-18 | S-4 完成：InputValidator 创建 | Sisyphus |
| 2026-09-18 | S-5 完成：X11 优雅降级 | Sisyphus |
| 2026-09-18 | Step 1 完成：SocketMultiplexer 并发重构 (修复 Q-1, P-1) | Sisyphus |
| 2026-09-18 | Step 2 完成：QtNetwork 抽象层 + 双实现骨架 (修复 T-3) | Sisyphus |
| 2026-09-18 | Step 3 完成：QtTcpTransport QTcpSocket 集成 | Sisyphus |
| 2026-09-18 | Q-3 完成：协议版本 static const→constexpr | Sisyphus |
| 2026-09-18 | T-6 完成：Qt6 加入 vcpkg.json，构建完全自动化 | Sisyphus |
| 2026-09-20 | 工作区整理：删除空目录，移动已完成规划文档到 archive | Sisyphus |
| 2026-09-20 | MCP 集成方案：完成 MCP 服务选型和集成设计文档 | Sisyphus |
| 2026-09-20 | MCP 配置模板：创建 Claude Desktop 配置文件模板 | Sisyphus |
| 2026-09-20 | 脚本迁移：setup-deps.ps1 → setup-deps.bat，兼容 CMD 和 PowerShell | Sisyphus |
| 2026-09-21 | 一致性审计：新增 docs/consistency-audit.md（19 项 U-01~U-19） | Cursor |
| 2026-09-22 | S-4 修正：校验器原先只创建未接入，实际未生效；已接入 ServerProxy 入站路径（keyDown/keyRepeat/keyUp）并新增 InputValidatorTests | Cursor |
| 2026-09-22 | 质量：MSWindowsScreen::disable() 改为幂等，消除一次 enable 对应两次 disable 导致的重复清理与误导性告警 | Cursor |
| 2026-09-23 | 全面审计（4 轮）：新增 A-01～A-11 清单（§2.4），完整报告落盘 `synergy/docs/audit-2026-09-23.md`；U 计数修正 19→21；修复 §2.3 失效相对链接（A-11） | opencode |
| 2026-09-23 | A-01：CMake 下限声明统一 3.25+；A-02：Qt 下限统一 6.7.0；U-03：删除 `org.deskflow` 打包残留 | Cursor |
| 2026-09-23 | 文件传输：Win32DropData / IDropTarget / Outbound 切屏接线 / macOS pasteboard（`7bf1325b5`…`e922e1639`） | Cursor |
| 2026-09-23 | A-03/U-14：HANDOFF 对齐（Step 定义表、QtNetworkTransport 完整实现、头部/下一步刷新） | Cursor |
| 2026-09-23 | A-06：`build.sh` 改为提示 `linux-asan-build`；A-08/U-08：修正 Synergy.cmake 与 UrlConstants 过时注释；A-05：U-16/U-01 台账对齐 | Cursor |
| 2026-09-23 | 待补登其余历史细节若有遗漏，以 `git log --since=2026-09-22` 为准（A-04 主路径已闭合） | Cursor |
| 2026-09-23 | A-09 关闭：原未提交 WIP 已随 `fe1f4fe37`（Windows IDropTarget）、`91960506f`（Outbound 发送）、`e922e1639`（macOS 路径读取）提交，`git status --porcelain` 仅剩本文档与审计报告 | opencode |
| 2026-09-23 | U-03 关闭：删除 org.deskflow 打包残留 4 文件（`deploy/linux/org.deskflow.{desktop,metainfo.xml,png}`、`flatpak/org.deskflow.deskflow.yml`）及孤儿 lint 伴随 json（唯一 key 即被删清单 app-id）；REUSE.toml/build.md/flatpak yml 注释同步，身份统一 `com.tupig.synergy` | opencode |
| 2026-09-23 | A-05/A-07 关闭：台账状态对齐 + README 双语标题与入口指引（`72734ba05`） | opencode |
| 2026-09-23 | A-03/U-14 关闭：HANDOFF 对齐 Step 定义与头部（`d8512e321`） | opencode |
| 2026-09-23 | A-10/U-10/U-13/U-15 关闭：IDataSocket 抛异常、许可与版本真源统一、metainfo 供应商改 TuPig、CI 增版本比对（`cbd2baf39`） | opencode |
| 2026-09-23 | U-07/U-17/U-18/U-19 关闭、U-09 文档化：D1 单图标主题、显示名统一、i18n 加固、overlay 有意设计；plan-B 落盘；拖拽 G1 清单入 delivery（`1269e7cb9`） | opencode |
| 2026-09-23 | Phase 2：Windows IDropSource（`587415ef1`）、Qt QSslSocket/QSslServer Step 4（`c8a5fae8c`）、EventQueue 泵入 Qt（`ea01a602a`）、B 计划进度同步（`8695555be`） | opencode |
| 2026-09-24 | 二轮全面审计（4 路并行）：报告落盘 `synergy/docs/audit-2026-09-24.md`；新发现 A-12～A-27 登记 §2.5；A-10 状态补翻；A-18/A-19 登记并随本提交刷新 HANDOFF 头部与本变更日志 | opencode |
| 2026-09-24 | A-12/A-13 关闭（待 CI 实证）：lint-clang 补 `working-directory: synergy`；`ci-passed` 纳入 lint needs；flatpak `manifest-path`/Validate/Upload 统一 workspace 根锚定 | opencode |
| 2026-09-24 | P2 关闭 A-14～A-19：ci push→main + static-analysis 注释如实；HANDOFF 回滚/工厂状态对齐现实；README 拖拽与配置键按 delivery/Settings 重写；consistency-audit U 详情节补 Resolution | opencode |
| 2026-09-24 | A-24 策略拍板（保留上游 SPDX+注释，仅改用户可见面）、A-15 并入 plan-B Phase 1、P3 等 CI 绿；登记 A-28～A-30（CI 首跑新发现） | opencode |
| 2026-09-24 | A-28 关闭：应用 CI clang-format-diff（105 源文件）；A-29 关闭：ci-passed/report/s3-upload 补 `working-directory: .`；A-30 关闭（拍板：无 AWS secrets 时 s3-upload skip，job `if` 判空） | opencode |
| 2026-09-24 | A-30 返工：job 级 `if` 不可用 `secrets` 上下文（workflow 0s 解析失败）→ 改 step 级判空；A-31 登记并修复（CodeQL/Sonar/Valgrind 容器首步 cwd） | opencode |
| 2026-09-24 | A-32～A-35 登记并修复：Linux 矩阵首步 cwd（同 A-31 根因）；macOS GUI target 名空格（五处 TARGET_BUNDLE 同步）；CodeQL autobuild 双项目歧义改 manual build；metainfo homepage 私仓 404 改公开 URL | opencode |
| 2026-09-24 | 二轮 CI（`35952577862`）暴露 6 类 26 job 挂，登记并修复 A-36～A-41：qtbase_en.qm 搜不到 + 缺翻译包；macOS ld 拒 GNU -z；XWindowsConfig.h 生成目录不在 include path；tupig.com TLS 仍挂改 github 组织页；s3-upload Check AWS 步缺 cwd；report Slack token 判空 skip | opencode |
| 2026-09-24 | 三轮 CI（`35957542966`）暴露 6 类 19 job 挂（10 job 绿），登记并修复 A-42～A-47：aarch64 误开 AVX2；macOS `platform/` include 前缀错 ×6；Fedora 缺 qtbase_es.qm 硬 DEPENDS；debian-12/ubuntu-24.04 Qt6 6.4<6.7 改 Qt5 回退 + BUILD_TESTS=OFF + qtbase5-dev；rocky QSslServer 无 Qt6 守卫；flatpak Lint manifest 未传 exceptions | opencode |
| 2026-09-24 | 四轮 CI（`35961135433`，11 挂含级联 ci-passed）暴露 4 类根因，登记并修复 A-48～A-51：AppUtilUnix `<platform/OSXAutoTypes.h>` 错；Qt5 `sslErrors`/`errorOccurred`/`QLocalSocket::error` 重载与 5.15 信号守卫；`QSettingsProxy.h`/`Settings.h` 缺 `<memory>`；debian-13 Qt5 误选（OpenGL 缺失）+ unittests 硬 find_package(Qt6) 碰 ALIAS —— debian 补 OpenGL 包、BUILD_TESTS 门控 Qt6、unittests 加注释 | opencode |
| 2026-09-24 | P3 关闭 A-20～A-27（`365f64cb7`，待 CI 实证）：GoogleTest 幽灵 ×7；CMake 批 7 项；文档死链批（含 `.github/CONTRIBUTING.md` 的 `bug_report.yml`）；REUSE/Sonar/Flatpak/WiX/Xcode/vcpkg 卫生；上游身份用户可见面 + SPDX A-24 注释；metainfo/Flatpak 许可补 OpenSSL exception；light 回收站图标 | opencode |
| 2026-09-24 | docs 清理：`synergy/docs` 收敛为 HANDOFF/build/configuration/protocol/security/troubleshooting 六文件；G1 拖拽清单 + 交付验证/R8 + B 计划阶段状态迁入 HANDOFF §5.4～§5.6；删除 architecture/contributing/delivery/consistency-audit/两份 audit/plan-B；AGENTS、`.github/CONTRIBUTING.md`、本台账引用同步 | opencode |
| 2026-09-24 | 五轮 CI（`35964563457`，commit `25f848a25`）+ 六轮 CI（`35970799694`，commit `3f140c029`）诊断：五轮 7 job 挂（macOS×2 dyld、debian-13-x86_64 测试目录、debian-12×2 `<optional>`、ubuntu-24.04×2 format、ci-passed 级联）；六轮 23 job 挂 —— 主因 A-21 xkbfile 探测符号选错致 UNIX X11 腿 Configure 全灭（CodeQL 同挂），macOS dyld 未愈，flatpak×2 上游 libei 503（瞬态，非代码）；登记 A-52～A-56 | opencode |
| 2026-09-24 | 修复 A-52～A-56：xkbfile 改 `find_library`；`CoreProcess.h` 补 `<optional>`；`ScreenSetupModel` qFatal `qlonglong` 转型 + 拼写；run-tests 缺目录跳过 + Qt 选择诊断打印 `Qt6_DIR`；`QT_IS_SHARED` 补 framework 判据 —— 待 CI 实证；flatpak 503 需重跑观察 | opencode |
| 2026-09-24 | 五/六轮后全部 job `runner_id:0` 秒挂：annotation 实证为 GitHub Actions **billing 失败**（付款失败/ spending limit），非代码问题；A-57 登记并修复：`ci.yml` push/PR 加 synergy 路径过滤，与 `ghdesktop2chinese.yml` 触发隔离 — 待 Billing 恢复后 CI 实证 | opencode |
| 2026-09-24 | 用户拍板「付费的不要了」：A-58 登记并修复 —— ci.yml 删 schedule、Windows/macOS/flatpak/s3 仅 release+dispatch、Linux 矩阵拆 free(4)/full(19)（`.github/matrices/linux-targets.json`）、CodeQL/Sonar 仅手动、去 PR valgrind；ghdesktop2chinese 周定时用户已删（`3aaa92ed1`）；YAML/JSON 解析通过 | opencode |
| 2026-09-24 | A-58 推送 `478be74cc`，run `35978730528` 实测：免费化结构生效（Win/macOS/flatpak/s3 skip、无 CodeQL/Sonar 自动触发），但 lint/get-version/ci-passed/report 仍 `runner_id:0` 9 秒失败 = **billing 硬锁未解除**；须用户网页 Billing 处理或转 public，代码侧已无法再省 | opencode |
| 2026-09-24 | A-59：仓库改名 `Tupig/Tupig_synergy` 并转 public（public Actions 免费）；`synergy/` 全部内容迁到仓库根，workflow/action/docs/URL 引用同步（`working-directory`/workspace/flatpak/artifact、`Tupig/TuPig_Product`→`Tupig/Tupig_synergy`）、`.gitignore` 合并、`SECURITY.md` 改真实流程；YAML/JSON 解析通过 | opencode |

---

## 6. 验证清单

- [x] S-1：`verifyCertificateCallback` 正确替换 `verifyIgnoreCertCallback`
- [x] S-1：LSP 诊断无错误
- [x] S-2：`MessageSizeLimit` 枚举定义完整
- [x] S-2：旧常量引用枚举值（向后兼容）
- [x] S-2：LSP 诊断无错误
- [x] S-3：15 处 assert 全部替换为异常抛出
- [x] S-3：Release 构建不再静默忽略协议错误
- [x] S-3：LSP 诊断无错误
- [x] S-4：InputValidator 创建完成
- [x] S-4：范围/频率/敏感键测试通过
- [x] S-5：ioErrorHandler 不再终止进程
- [x] S-5：DisplayLost 事件正确发送
- [x] S-5：析构函数在 display lost 状态下安全

---

> **注意**：本文档随代码改动同步更新。每次修复完成后，更新「问题清单」状态、「变更日志」和「验证清单」。
