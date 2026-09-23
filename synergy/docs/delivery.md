# Delivery Matrix / 交付矩阵

> **Language / 语言**: [English](#english) | [中文](#中文)
>
> **Last updated / 最后更新**: 2026-09-23
> **Related / 相关**: `docs/build.md`, `docs/consistency-audit.md`, `AGENTS.md`

---

## English

### Purpose

States exactly what each platform produces, what is inside it, which of those
artifacts has actually been verified, and which constraints are structural rather
than unfinished work. Read this before promising a "single self-contained binary":
two of the requirements below cannot be satisfied simultaneously on every platform.

### Summary

| Platform | Generator | Artifact | Self-contained | Verified |
|---|---|---|---|---|
| Windows | CPack `7Z` | `synergy_<ver>_windows_x64-portable.7z` | Yes | **Yes** (built, packaged, tested) |
| Windows | CPack `WIX` | `.msi` | Yes | **Built and inspected** — see below |
| macOS | CPack `DragNDrop` | `.dmg` containing `TuPig Synergy.app` | Yes | **No** — no macOS machine available |
| Linux | CPack `DEB` / `RPM` | `.deb` / `.rpm` | Needs system libs | **No** — no Linux machine available |

`deploy/linux/deploy.cmake` picks `DEB` or `RPM` from `/etc/os-release`, so a given
build host produces one of the two, not both.

**MSI status.** WiX v7 refuses to run until its Open Source Maintenance Fee EULA is
accepted (`WIX7015`). After running `wix.exe eula accept wix7` once, the MSI builds.
The Free-or-Fee terms exempt projects under US$10,000 annual revenue, but acceptance is
a licensing decision for the project owner, not something a build script should do
implicitly. The produced MSI was inspected with the Windows Installer database API:

- `File` table: `synergy.exe`, `synergy-core.exe`, `synergy-daemon.exe`, 7 translation
  catalogues, `LICENSE`, `LICENSE_EXCEPTION`.
- `ServiceInstall`: name and display name `TuPig Synergy`, `StartType=2` (auto), with the
  secure-desktop description.
- `ServiceControl`: event `163` = install + uninstall + start + stop, i.e. the service is
  started on install and stopped and removed on uninstall.

An actual installation was **not** performed, since that modifies the host machine.

### Self-containment: what is actually achieved

Dependencies are static, so the executables carry their runtime inside:

- Qt 6 comes from the vcpkg **static** triplet (`x64-windows-static`), not a shared Qt.
- The MSVC CRT is linked statically (`/MT`) via
  `CMAKE_MSVC_RUNTIME_LIBRARY` in the root `CMakeLists.txt`.
- Consequence, verified with `dumpbin /DEPENDENTS`: the Windows executables list
  **no** `msvcp140`, `vcruntime140` or `concrt140`; only system DLLs plus Qt when Qt
  is shared. `deploy/windows/deploy.cmake` therefore skips bundling the VC++
  redistributable (`Static CRT in use; not bundling the VC++ redistributable`).
- Translations are deployed next to the executables at build time
  (`deploy_translations`); they are data files, not a runtime dependency.

### Structural constraints

These are not outstanding bugs. Each has a reason, and removing it costs a feature.

**1. The portable Windows package intentionally excludes `synergy-daemon.exe`.**
`deploy/windows/pre-cpack.cmake.in` deletes it and writes a README explaining why:

> The portable version does not include the daemon, so the client will not work at
> UAC prompts or the login screen.

The daemon runs as a Windows **service** (session 0) and duplicates the token of
`winlogon.exe` / `logonui.exe` to start the core on the secure desktop. A portable
archive cannot register a service, so the capability is dropped rather than shipped
broken. Use the MSI to get daemon-backed behaviour.

**2. One executable per platform is not reachable on Windows.**
`synergy.exe` (GUI) launches `synergy-core.exe` as a child process
(`CoreProcess.cpp`), and `synergy-daemon.exe` must be a separate service process.
Collapsing the GUI and core is possible in desktop mode, but the daemon cannot be
folded in without losing secure-desktop and UAC support. The realistic Windows
result is therefore **three binaries plus a `translations/` directory**, delivered
inside one archive.

**3. macOS must ship a `.app` bundle.**
A bare Mach-O binary cannot be launched from Finder, signed, or notarized. The
deliverable is a `.dmg` containing the bundle — structurally a directory, not a
single file.

**4. Linux AppImage is not implemented.**
No `appimage` reference exists anywhere in `deploy/`, `extra/` or `.github/`; only
`DEB` and `RPM` are produced. Any document claiming AppImage support is stale and
has been corrected. AppImage would satisfy "one self-contained file" on Linux if
implemented, and it is the recommended path if that goal is prioritised.

**5. UAC / login-screen support requires a registered Windows service.**
`synergy-daemon.exe` runs as a service (session 0) and duplicates `winlogon.exe` /
`logonui.exe` tokens to start the core on the secure desktop. Nothing in this codebase
registers that service — there is no `--install-service` option and no `CreateService`
call — so it must come from the MSI (which declares `ServiceInstall`) or a manual
`sc create`. See `docs/troubleshooting.md`. The portable package intentionally omits the
daemon, so portable builds cannot offer this capability at all.

### File transfer (drag-and-drop) status

| Layer | Status | Verified |
|---|---|---|
| Protocol (`DDRG`/`DFTR`) + path sanitisation | Implemented | Unit tests |
| Client receive + hardened drop directory | Implemented | Unit tests |
| Server outbound (`FileTransferOutbound` + leave-primary send) | Implemented | Unit tests |
| Windows OLE `IDropTarget` + CF_HDROP parser | Implemented | Unit tests (parser); cross-screen capture needs manual check |
| Windows `IDropSource` (drop into local Explorer) | Implemented | Unit tests (CF_HDROP writer round-trip); DoDragDrop needs manual check |
| macOS drag pasteboard (`copyDraggedFilePaths`) | Implemented | **No** — no macOS host; static review only |
| Linux XDND / Wayland DnD | **Not implemented** (never was upstream) | N/A |

Enable with `fileTransfer/enabled=true` and a non-empty `fileTransfer/dropDirectory`.

### How to produce each artifact

```bat
REM Windows: build, then package
scripts\build.bat release
cmake --build build --config Release --target package
```

```bash
# Linux / macOS
./scripts/build.sh release
cmake --build build --target package
```

Windows output lands in `build/`, for example
`synergy_1.21.2-dev+<sha>_windows_x64-portable.7z`. The archive contains
`synergy.exe`, `synergy-core.exe`, `translations/*.qm`, `LICENSE`,
`LICENSE_EXCEPTION`, `README.txt` and a `settings/` directory (portable mode).

### Verification status

Verified on Windows (Release, static triplet):

- Build and packaging succeed; the portable archive is produced.
- `dumpbin /DEPENDENTS` confirms no MSVC runtime DLL dependency.
- GUI→core handshake: with `gui/startCoreWithGui=true` and a server `coreMode`, the
  GUI starts `synergy-core.exe` within ~3 s. This exercises the same code path as
  audit item U-01, whose fix (unversioned executable names) is thereby confirmed
  end-to-end rather than by file name alone.
- Interface language resolves to the system locale (`initial language: zh_CN`).
- Unit tests: 25/25 pass, in both the Release and AddressSanitizer configurations.

**Not verified:** macOS and Linux builds, packaging and runtime. No machine of
either platform was available, so nothing here should be read as a claim about
them — including the DEB/RPM and DMG artifacts listed above.

---

## 中文

### 目的

明确各平台**实际产出什么**、内含什么、哪些已**真正验证过**，以及哪些限制属于**结构性约束而非未完成工作**。在承诺「单一自包含二进制」之前请先读本文：以下有两条要求无法在所有平台同时满足。

### 汇总

| 平台 | 生成器 | 产物 | 自包含 | 已验证 |
|---|---|---|---|---|
| Windows | CPack `7Z` | `synergy_<ver>_windows_x64-portable.7z` | 是 | **是**（构建、打包、测试均通过） |
| Windows | CPack `WIX` | `.msi` | 是 | **已构建并检查** —— 见下 |
| macOS | CPack `DragNDrop` | 含 `TuPig Synergy.app` 的 `.dmg` | 是 | **否** —— 无 macOS 机器 |
| Linux | CPack `DEB` / `RPM` | `.deb` / `.rpm` | 需系统库 | **否** —— 无 Linux 机器 |

`deploy/linux/deploy.cmake` 依据 `/etc/os-release` 在 `DEB` 与 `RPM` 之间二选一，因此单次构建只产出其中一种，而非两者。

**MSI 状态**：WiX v7 在未接受其开源维护费（OSMF）EULA 前拒绝运行（`WIX7015`）。一次性执行
`wix.exe eula accept wix7` 后 MSI 即可构建。其免费条款豁免年收入低于 1 万美元的项目，但接受与否
属于项目所有者的许可决策，不应由构建脚本擅自代劳。产出的 MSI 已用 Windows Installer 数据库 API 检查：

- `File` 表：`synergy.exe`、`synergy-core.exe`、`synergy-daemon.exe`、7 个翻译目录、`LICENSE`、`LICENSE_EXCEPTION`。
- `ServiceInstall`：名称与显示名均为 `TuPig Synergy`，`StartType=2`（自动），描述为安全桌面用途。
- `ServiceControl`：事件 `163` = 安装 + 卸载 + 启动 + 停止，即安装时启动服务、卸载时停止并删除。

**未执行实际安装**，因为那会改动本机系统。

### 自包含：实际达成的程度

依赖均为静态，运行时被编入可执行文件：

- Qt 6 来自 vcpkg **静态** triplet（`x64-windows-static`），非共享 Qt。
- MSVC 运行库通过根 `CMakeLists.txt` 的 `CMAKE_MSVC_RUNTIME_LIBRARY` 静态链接（`/MT`）。
- 实测结论（`dumpbin /DEPENDENTS`）：Windows 可执行文件的依赖列表中**不含** `msvcp140`、`vcruntime140`、`concrt140`，仅含系统 DLL（Qt 共享时另含 Qt）。因此 `deploy/windows/deploy.cmake` 跳过打包 VC++ 可再发行组件，并输出 `Static CRT in use; not bundling the VC++ redistributable`。
- 翻译文件在构建时部署到可执行文件旁（`deploy_translations`）；它们是数据文件，不是运行时依赖。

### 结构性约束

以下均非待办缺陷，每条都有其原因，去掉就要牺牲对应能力。

**1. Windows 便携包有意排除 `synergy-daemon.exe`。**
`deploy/windows/pre-cpack.cmake.in` 主动删除它并写入 README 说明：

> 便携版不包含 daemon，因此客户端在 UAC 提示与登录界面无法工作。

daemon 以 Windows **服务**形式运行（会话 0），通过复制 `winlogon.exe` / `logonui.exe` 的令牌把 core 启动到安全桌面。便携归档无法注册服务，因此宁可不提供该能力，也不交付一个坏掉的版本。需要该能力请用 MSI。

**2. Windows 上无法做到「每平台一个可执行文件」。**
`synergy.exe`（GUI）会把 `synergy-core.exe` 作为子进程启动（`CoreProcess.cpp`），而 `synergy-daemon.exe` 必须是独立的服务进程。GUI 与 core 在桌面模式下可以合并，但 daemon 一旦合并就会失去安全桌面与 UAC 支持。因此 Windows 的现实结果是**三个二进制 + 一个 `translations/` 目录**，整体装在一个归档内。

**3. macOS 必须交付 `.app` bundle。**
裸 Mach-O 二进制无法从 Finder 启动，也无法签名与公证。交付物是含 bundle 的 `.dmg` —— 结构上是目录而非单文件。

**4. Linux AppImage 未实现。**
`deploy/`、`extra/`、`.github/` 中不存在任何 `appimage` 引用，实际只产出 `DEB` 与 `RPM`。任何声称支持 AppImage 的文档均为过时描述，已更正。若优先考虑该目标，AppImage 是在 Linux 上实现「单一自包含文件」的推荐路径。

**5. UAC / 登录界面支持需要一个已注册的 Windows 服务。**
`synergy-daemon.exe` 以服务身份运行（会话 0），通过复制 `winlogon.exe` / `logonui.exe` 的令牌把
core 启动到安全桌面。本代码库中没有任何地方注册该服务 —— 既无 `--install-service` 选项，也无
`CreateService` 调用 —— 因此必须由 MSI（其声明了 `ServiceInstall`）或手工 `sc create` 完成。
详见 `docs/troubleshooting.md`。便携包有意不含 daemon，故便携构建完全无法提供该能力。

### 文件传输（拖拽）状态

| 层 | 状态 | 已验证 |
|---|---|---|
| 协议（`DDRG`/`DFTR`）+ 文件名净化 | 已实现 | 单元测试 |
| 客户端接收 + 加固落盘 | 已实现 | 单元测试 |
| 服务端外发（`FileTransferOutbound` + 离开主屏发送） | 已实现 | 单元测试 |
| Windows OLE `IDropTarget` + CF_HDROP 解析 | 已实现 | 解析器单测；跨屏捕获需人工验证 |
| Windows `IDropSource`（投进本机资源管理器） | 已实现 | CF_HDROP 写入往返单测；DoDragDrop 需人工验证 |
| macOS 拖拽剪贴板（`copyDraggedFilePaths`） | 已实现 | **否** —— 无 macOS 主机，仅静态审阅 |
| Linux XDND / Wayland DnD | **未实现**（上游亦从未实现） | 不适用 |

需设置 `fileTransfer/enabled=true` 且 `fileTransfer/dropDirectory` 非空。

### 如何产出各产物

```bat
REM Windows：先构建，再打包
scripts\build.bat release
cmake --build build --config Release --target package
```

```bash
# Linux / macOS
./scripts/build.sh release
cmake --build build --target package
```

Windows 产物落在 `build/` 下，例如 `synergy_1.21.2-dev+<sha>_windows_x64-portable.7z`。归档内含 `synergy.exe`、`synergy-core.exe`、`translations/*.qm`、`LICENSE`、`LICENSE_EXCEPTION`、`README.txt` 与 `settings/` 目录（便携模式）。

### 验证状态

已在 Windows（Release、静态 triplet）验证：

- 构建与打包成功，便携归档正常产出。
- `dumpbin /DEPENDENTS` 确认不依赖 MSVC 运行库 DLL。
- GUI→core 握手：设置 `gui/startCoreWithGui=true` 与服务端 `coreMode` 后，GUI 在约 3 秒内启动 `synergy-core.exe`。这走的正是审计条目 U-01 所涉代码路径，因此其修复（可执行文件名去掉版本号）得到端到端确认，而非仅凭文件名判断。
- 界面语言随系统区域解析（`initial language: zh_CN`）。
- 单元测试：Release 与 AddressSanitizer 两种配置均 25/25 通过。

**未验证**：macOS 与 Linux 的构建、打包与运行。当前没有这两个平台的机器，因此上文所列 DEB/RPM 与 DMG 产物不应被理解为已经过验证。
