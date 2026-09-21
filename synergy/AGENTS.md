# AGENTS / 协作规则

> **Language / 语言**: [English](#english) | [中文](#中文)
>
> 本文件是本仓库对**所有 AI 协作代理与贡献者**的统一约束。与 `docs/HANDOFF.md`（会话交接）、
> `docs/consistency-audit.md`（一致性审计）、`.github/ISSUE_TEMPLATE/security-quality-refactoring.md`
> （问题追踪）配套使用。
>
> This file defines the binding rules for **all AI agents and contributors** working in this
> repository. It complements `docs/HANDOFF.md`, `docs/consistency-audit.md` and the issue tracker.

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
| R5 | All third-party dependencies come from vcpkg in **manifest mode**. | Single declared dependency source. |
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
| R5 | 第三方依赖统一来自 vcpkg 的**清单（manifest）模式**。 | 依赖来源唯一。 |
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
