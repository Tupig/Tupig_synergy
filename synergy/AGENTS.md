# AGENTS / 协作规则

> **Language / 语言**: [English](#english) | [中文](#中文)
>
> 本文件是本仓库对**所有 AI 协作代理与贡献者**的统一约束。与 `docs/HANDOFF.md`（会话交接）、
> `../.github/ISSUE_TEMPLATE/security-quality-refactoring.md`（问题追踪）配套使用。
>
> This file defines the binding rules for **all AI agents and contributors** working in this
> repository. It complements `docs/HANDOFF.md` and the issue tracker.

---

## English

### Scope

Applies to every change under `synergy/`. When a rule here conflicts with an ad-hoc request, raise the
conflict instead of silently breaking the convention.

### Hard constraints

| # | Rule | Rationale |
|---|---|---|
| R1 | Windows automation is `.bat` only. **No `.ps1` files anywhere.** | One scripting runtime on Windows; avoids execution-policy failures. |
| R2 | `.bat` must work when invoked from both `cmd /c` and PowerShell. | Callers differ; the script must not assume either. |
| R3 | Script text is **ASCII-only**. | Console code pages (GBK/UTF-8) corrupt non-ASCII in batch files and can break parsing. |
| R4 | No machine-specific absolute paths, no required environment variables. | Every machine differs; scripts must self-locate VS, vcpkg and tools. |
| R5 | All third-party dependencies come from vcpkg in **manifest mode**, with one documented exception: **CI** installs Qt from a prebuilt Qt distribution and runs vcpkg in classic mode for OpenSSL. | Single declared dependency source for release builds; the CI exception trades exact parity for build time, and must stay version-compatible (see below). |
| R6 | The vcpkg baseline is pinned in `vcpkg.json` (`builtin-baseline`) and is the **single source of truth**; scripts must read it, never duplicate it. | Keeps clones reproducible. |
| R7 | `vendor/` (vcpkg checkout) and `build/` are never committed — they are bootstrapped locally. | Repository stays small and clonable. |
| R8 | Code must compile on Windows, macOS and Linux. Platform code lives under `src/lib/platform/<os>/`. | One source tree, conditional compilation. |
| R9 | Documentation is bilingual in a single file (sectioned `## English` + `## 中文`). | One document, two audiences. |
| R10 | Commit messages are written in Chinese. | Project convention. |

### Build & dependency policy

- The user-visible entry points are `setup.bat` (one-time host toolchain) and `scripts/build.bat` /
  `scripts/build.sh` (per-platform build). Nothing else should be needed.
- `scripts/bootstrap-vcpkg.{bat,sh}` clones and bootstraps the repository-local vcpkg into
  `vendor/vcpkg`, reads the pinned baseline from `vcpkg.json`, and is idempotent.
- A C++ toolchain (MSVC / clang / gcc), CMake and Ninja are unavoidable host prerequisites and are
  **not** vendored. Everything else — Qt, OpenSSL and their transitive dependencies — is fetched by
  vcpkg.
- On Windows, the MSVC environment must be activated (`vcvarsall.bat x64`) before CMake runs. vcpkg
  resolves its Visual Studio instance from the VC environment variables; without them it fails with
  *"Could not locate a complete Visual Studio instance"* on Build Tools installations whose instance
  metadata lacks an `isComplete` flag. `scripts/build.bat` does this automatically — do not remove it.
- Do not add `/MD` alongside the static vcpkg triplet, and do not reintroduce per-machine vcpkg
  discovery (`VCPKG_ROOT`, `C:\Users\<user>\vcpkg`).
- **The MSVC runtime must follow the vcpkg triplet, and `CMakeLists.txt` is what resolves it.**
  vcpkg does **not** set `CMAKE_MSVC_RUNTIME_LIBRARY` on a plain Windows configure: its
  `buildsystems/vcpkg.cmake` includes the platform toolchain only when
  `VCPKG_CHAINLOAD_TOOLCHAIN_FILE` is set (`scripts/buildsystems/vcpkg.cmake:209`) and never
  defaults that variable, so `toolchains/windows.cmake` — the file that would derive the runtime
  from `VCPKG_CRT_LINKAGE` — does not run. `VCPKG_CRT_LINKAGE` is also empty in project scope
  (measured). The project therefore reads the linkage from the triplet file itself (falling back to
  vcpkg's `-static` naming convention) and sets `/MT` or `/MD` accordingly, logging which source
  answered. Do not hardcode either value: forcing `/MT` made every dynamic-CRT configuration —
  which is exactly what the CI Windows legs use — compile against `/MD` dependencies and fail with
  `LNK2038 RuntimeLibrary mismatch`. ASan is the only genuine constraint; it needs a dynamic
  triplet, which the `windows-msvc-asan` preset provides and the configure step now checks for.

**The CI dependency exception (R5), and its obligations.** `.github/actions/install-dependencies`
installs Qt via `jurplel/install-qt-action` and runs `johnwason/vcpkg-action` with `--classic` for
OpenSSL only, so CI's Qt does **not** come from `vcpkg.json`. This is deliberate: vcpkg builds Qt
from source, which is hours of CI time per leg, while the manifest remains the source of truth for
release builds and for every local build. The exception carries three requirements:

1. The CI Qt version must be **>= the project floor** (`REQUIRED_QT_VERSION` in `CMakeLists.txt`,
   currently 6.7.0, with a 5.13 fallback for RHEL 8). CI currently pins 6.9.3 / 6.10.3 / 6.11.1.
   When bumping it, re-check the floor rather than assuming.
2. CI's Qt is **shared**, whereas the manifest builds Qt **static**. So CI does not exercise the
   static-Qt path that releases ship. Treat a green CI as evidence about *logic*, not about the
   static link model; that path is covered by a local `scripts/build.bat release`.
3. Any divergence that has to be kept (for example a Qt-version-specific workaround) must be
   recorded here or in `docs/build.md` — an undocumented difference between CI and release is how
   the two drift apart silently.

### Delivery constraints — do not "fix" these

This section is the authoritative list (verification status lives in `docs/HANDOFF.md` §5.5).
The following are deliberate, not defects:

- **`synergy-daemon.exe` must stay a separate process.** It runs as a Windows service (session 0) and
  duplicates `winlogon.exe` / `logonui.exe` tokens to start the core on the secure desktop. Folding it
  into the GUI would silently drop UAC-prompt and login-screen support.
- **`synergy-core.exe` stays separate from the GUI.** The GUI launches it as a child process
  (`CoreProcess.cpp`). Merging them is possible for desktop mode only.
- **The portable package deliberately omits the daemon** (`deploy/windows/pre-cpack.cmake.in`); a
  portable archive cannot register a service. Do not "restore" it.
- **Executable names carry no version number** (`synergy` / `synergy-core` / `synergy-daemon`).
  Versioned names break `Constants.h.in`'s `kCoreBinName`, the `.desktop` `Exec=`, and WiX component IDs.
- **macOS ships a `.app` bundle inside a `.dmg`**, not a bare binary — signing and notarization require
  the bundle layout.
- **Nothing in this codebase registers the Windows service, and that is deliberate.**
  `synergy-daemon.exe` cannot install itself: there is no `--install-service` option and no
  `CreateService` call. The MSI provides registration via `ServiceInstall`. Do not implement a
  self-install option to satisfy older documentation — `docs/troubleshooting.md` documents the real
  (MSI or `sc create`) procedures.

### Modification workflow

1. Propose the approach before changing code; prefer the smallest correct change.
2. One logical change per commit, committed immediately after it is made.
3. Verify with evidence (build log, command output, real run). Never report success on reasoning alone.
4. Keep `docs/HANDOFF.md` and the issue tracker in step with code changes.

### Known environment traps
- `synergy/vendor/vcpkg` is a shallow clone; the pinned baseline commit must be present before CMake
  configures.
- Deleting `vendor/` only costs a re-clone (~30 MB); dependency rebuild time is recovered by the vcpkg
  binary cache outside the repository.
- **Flaky or slow links to GitHub**: the first configure downloads large assets (Strawberry Perl is
  ~290 MB, required by OpenSSL; Qt 6 sources follow). On an unreliable connection these transfers can
  stall part-way. vcpkg stores them under `vendor/vcpkg/downloads/` and reuses anything already
  there, so simply re-running the build script resumes effectively — do not delete that directory to
  "start clean". vcpkg verifies every asset against a recorded SHA-512, so a partially written file is
  rejected rather than used.
- `vendor/vcpkg/downloads/` and the vcpkg binary cache are the only reasons a rebuild is cheap; keep
  them on disk between builds.

---

## 中文

### 适用范围

适用于 `synergy/` 下的所有改动。若本文件的规则与临时要求冲突，应当**先提出冲突**，而不是默默破坏约定。

### 强制约束

| # | 规则 | 理由 |
|---|---|---|
| R1 | Windows 自动化脚本一律 `.bat`，**全仓库禁止出现任何 `.ps1`**。 | Windows 上只保留一种脚本运行时，规避执行策略问题。 |
| R2 | `.bat` 必须同时能被 `cmd /c` 与 PowerShell 调用。 | 调用方不固定，脚本不得假定其中一种。 |
| R3 | 脚本内容**只用 ASCII 字符**。 | 控制台代码页（GBK/UTF-8）会破坏批处理中的非 ASCII 字符，甚至导致解析错误。 |
| R4 | 不得出现机器专属绝对路径，不得要求预设环境变量。 | 每台机器环境不同；脚本须自行定位 VS、vcpkg 与工具链。 |
| R5 | 第三方依赖统一来自 vcpkg 的**清单（manifest）模式**，但有**一处已记录的例外**：**CI** 使用预编译 Qt 分发，并以 classic 模式用 vcpkg 仅装 OpenSSL。 | 发布构建的依赖来源唯一；CI 的例外以构建耗时换取了与发布构建的完全一致，故须保持版本兼容（见下）。 |
| R6 | vcpkg baseline 固定在 `vcpkg.json` 的 `builtin-baseline`，是**唯一真源**；脚本须读取它，不得复制一份。 | 保证克隆可复现。 |
| R7 | `vendor/`（vcpkg 检出）与 `build/` 永不入库，一律本地引导生成。 | 保持仓库轻量、可克隆。 |
| R8 | 代码必须能在 Windows、macOS、Linux 三平台编译。平台相关代码放在 `src/lib/platform/<os>/`。 | 单一源码树 + 条件编译。 |
| R9 | 文档在单一文件内中英双语（分节 `## English` + `## 中文`）。 | 一份文档，两类读者。 |
| R10 | 提交信息使用中文。 | 项目约定。 |

### 构建与依赖策略

- 用户可见入口只有两个：`setup.bat`（一次性安装宿主工具链）与 `scripts/build.bat` / `scripts/build.sh`
  （分平台构建）。不应再有其他前置步骤。
- `scripts/bootstrap-vcpkg.{bat,sh}` 负责克隆引导仓库内的 vcpkg 到 `vendor/vcpkg`，从 `vcpkg.json`
  读取固定 baseline，且可重复执行。
- C++ 工具链（MSVC / clang / gcc）、CMake、Ninja 是不可避免的宿主前置条件，**不纳入仓库**。除此之外的
  一切依赖 —— Qt、OpenSSL 及其传递依赖 —— 均由 vcpkg 获取。
- Windows 上必须在 CMake 之前激活 MSVC 环境（`vcvarsall.bat x64`）。vcpkg 依赖 VC 环境变量来定位
  Visual Studio 实例；若缺少这些变量，在实例元数据没有 `isComplete` 标记的 Build Tools 上会报
  *"Could not locate a complete Visual Studio instance"*。`scripts/build.bat` 已自动处理，**不要删除**该步骤。
- 不要在静态 vcpkg triplet 下追加 `/MD`，也不要再引入依赖机器的 vcpkg 查找方式（`VCPKG_ROOT`、
  `C:\Users\<user>\vcpkg`）。
- **MSVC 运行库必须跟随 vcpkg triplet，而由 `CMakeLists.txt` 负责解析出它。**
  vcpkg 在普通 Windows 配置下**并不会**设置 `CMAKE_MSVC_RUNTIME_LIBRARY`：其
  `buildsystems/vcpkg.cmake` 仅在设置了 `VCPKG_CHAINLOAD_TOOLCHAIN_FILE` 时才 include 平台工具链
  （`scripts/buildsystems/vcpkg.cmake:209`），且从不给该变量默认值，因此
  `toolchains/windows.cmake` —— 即「依据 `VCPKG_CRT_LINKAGE` 推导运行库」的那个文件 —— 根本不会
  执行；`VCPKG_CRT_LINKAGE` 在项目作用域亦为空（已实测）。故项目改为**直接读取 triplet 文件**
  中的该设置（退路是 vcpkg 的 `-static` 命名约定），据此设 `/MT` 或 `/MD`，并在配置日志中打印
  实际来源。**不要把任一方写死**：强行指定 `/MT` 会让所有动态 CRT 配置 —— 正是 CI 两条 Windows 腿
  所用的 —— 以 `/MT` 链接 `/MD` 依赖并报 `LNK2038 RuntimeLibrary mismatch`。唯一的真实约束是
  ASan：它需要动态 triplet，`windows-msvc-asan` 预设已提供，配置阶段亦会校验。

**CI 的依赖例外（R5）及其义务。** `.github/actions/install-dependencies` 通过
`jurplel/install-qt-action` 安装 Qt，并以 `--classic` 运行 `johnwason/vcpkg-action` 仅装 OpenSSL，
因此 CI 的 Qt **不来自** `vcpkg.json`。这是有意为之：vcpkg 从源码构建 Qt，每条 CI 腿要数小时，而
manifest 仍是发布构建与所有本地构建的唯一真源。该例外附带三项要求：

1. CI 所用 Qt 版本必须 **≥ 项目下限**（`CMakeLists.txt` 的 `REQUIRED_QT_VERSION`，当前 6.7.0，
   RHEL 8 回退 5.13）。CI 现固定为 6.9.3 / 6.10.3 / 6.11.1。**升级时须重新核对该下限**，不要凭假设。
2. CI 的 Qt 是**共享**的，而 manifest 构建的是**静态** Qt。因此 CI **并未覆盖**发布所用的静态 Qt 路径。
   把 CI 绿灯当作**逻辑**上的证据，而非静态链接模型的证据；后者由本地 `scripts/build.bat release` 覆盖。
3. 任何必须保留的差异（例如针对某个 Qt 版本的适配）都须记录在此处或 `docs/build.md` ——
   CI 与发布之间未记录的差异，正是二者静默漂移的成因。

### 交付约束 —— 不要把这些当缺陷「修复」

本节即权威清单（验证状态见 `docs/HANDOFF.md` §5.5）。以下均为**有意设计**，不是缺陷：

- **`synergy-daemon.exe` 必须保持独立进程。** 它以 Windows 服务（会话 0）运行，通过复制
  `winlogon.exe` / `logonui.exe` 令牌把 core 启动到安全桌面。若合并进 GUI，会静默失去 UAC 提示与
  登录界面支持。
- **`synergy-core.exe` 与 GUI 保持分离。** GUI 以子进程方式启动它（`CoreProcess.cpp`）。仅在桌面
  模式下才谈得上合并。
- **便携包有意不含 daemon**（`deploy/windows/pre-cpack.cmake.in`）；便携归档无法注册服务。不要
  「恢复」它。
- **可执行文件名不带版本号**（`synergy` / `synergy-core` / `synergy-daemon`）。带版本号会破坏
  `Constants.h.in` 的 `kCoreBinName`、`.desktop` 的 `Exec=` 以及 WiX 的组件 ID。
- **macOS 交付的是 `.dmg` 内的 `.app` bundle**，而非裸二进制 —— 签名与公证都要求 bundle 结构。
- **本代码库中没有任何地方注册 Windows 服务，这是有意为之。**
  `synergy-daemon.exe` 无法自行安装：既无 `--install-service` 选项，也无 `CreateService` 调用。
  服务注册由 MSI 通过 `ServiceInstall` 提供。不要为了让旧文档「成真」而去实现自安装选项 ——
  `docs/troubleshooting.md` 记录的是真实做法（MSI 或 `sc create`）。

### 改动流程

1. 动手改代码前先给出方案，优先选择最小且正确的改动。
2. 一个提交只做一件逻辑独立的事，做完立即提交。
3. 必须有证据验证（构建日志、命令输出、真实运行）。严禁仅凭推理声称通过。
4. 代码变更须同步更新 `docs/HANDOFF.md` 与问题追踪文档。

### 已知环境陷阱

- `synergy/vendor/vcpkg` 是浅克隆；CMake 配置前必须确保固定 baseline 提交存在。
- 删除 `vendor/` 只需重新克隆（约 30 MB）；依赖重建耗时可由仓库外的 vcpkg 二进制缓存弥补。
- **GitHub 链路慢或抖动时**：首次配置需要下载大体积资源（Strawberry Perl 约 290 MB，为 OpenSSL
  所需；随后还有 Qt 6 源码）。链路不稳时传输可能中途停住。vcpkg 会把这些文件缓存于
  `vendor/vcpkg/downloads/`，已存在的文件会被复用，因此**重新运行构建脚本即可有效续传** —— 不要为了
  “干净重来”而删除该目录。vcpkg 会用记录的 SHA-512 校验每个资源，写入不完整的文件会被拒绝而非误用。
- `vendor/vcpkg/downloads/` 与 vcpkg 二进制缓存是重复构建成本低的关键，请在两次构建之间保留它们。
