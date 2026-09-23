# Consistency Audit / 一致性审计

> **Language / 语言**: [English](#english) | [中文](#中文)
>
> **Last updated / 最后更新**: 2026-09-21
> **Scope / 范围**: Cross-cutting naming + identity consistency / 跨模块命名与身份一致性
> **Related / 相关**: `docs/HANDOFF.md`, `.github/ISSUE_TEMPLATE/security-quality-refactoring.md`

---

## English

### Purpose

This is a findings inventory, not a fix plan. Each item has a stable ID (`U-nn`) so fixes, commits and issues can reference it. Items are **not** ordered by fix priority within a severity band.

Items were found by static analysis (ripgrep + reading the build/packaging files). No local build was available to confirm runtime behaviour — see [Verification needed](#verification-needed).

### How to use

1. Pick an ID, apply the fix, then update its **Status** here.
2. Per the project constraint in `docs/HANDOFF.md` §1.3, any accompanying code change must also be recorded in `.github/ISSUE_TEMPLATE/security-quality-refactoring.md`.
3. When closing an item, state the verification evidence (build log, package listing, or run) — do not flip Status to Fixed on reasoning alone.

### Summary

| ID | Severity | Area | Finding | Status |
|---|---|---|---|---|
| U-01 | P0 | Runtime / build | Versioned output names vs unversioned references | **Fixed** (see below) |
| U-02 | P0 | Packaging (Linux) | Install source paths use an identity that has no files | Open |
| U-03 | P1 | Packaging (all) | Three coexisting product identities; macOS and Linux differ | Open |
| U-04 | P1 | Packaging (macOS) | Bundle icon filename does not exist | **Fixed** |
| U-05 | P2 | Packaging (macOS) | Dead plist templates referencing removed variables | **Fixed** |
| U-06 | P2 | Packaging (Arch) | PKGBUILD declares a conflict with itself | Open |
| U-07 | P2 | Runtime (icons) | Two icon themes compiled into the same binary | Open |
| U-08 | P3 | Comments | Three comments contradict the code | Open |
| U-09 | P3 | Runtime (paths) | Directory names and file names use different app identifiers | Open |
| U-10 | P3 | Packaging | Vendor / copyright strings still name the upstream commercial vendor | Open |
| U-11 | P1 | Docs | Three bilingual formats coexist; two docs have no English | **Fixed** |
| U-12 | P1 | Docs | Stale references: presets, a deleted file, moved docs | **Fixed** |
| U-13 | P2 | Docs | Version numbers hardcoded in six places | Open |
| U-14 | P2 | Docs | `HANDOFF.md` has drifted from reality | Open |
| U-15 | P2 | Docs | Licence description disagrees across four files | Open |
| U-16 | P2 | CI | CI enforces unversioned binary names and the upstream vendor identity | Open |
| U-17 | P3 | Naming | Four different product display names | Open |
| U-18 | P3 | Naming | `deskflow` / `synergy` boundary plus a fragile i18n coupling | Open |
| U-19 | P3 | Naming | Overlay layout and test placement are inconsistent | Open |
| U-20 | P2 | Docs | Documented CLI option `--install-service` does not exist | **Fixed** |

Already fixed in this session: stale binary names in `README.md`, `setup.bat`, `docs/build.md`, `docs/configuration.md` and `src/apps/res/manpage.txt` (commit `baa5afe76`, branch `cursor/docs-fix-binary-names`).

### Findings

#### U-01 — Versioned output names vs unversioned references

**Severity**: P0 · **Verification**: needs a real build (see below)

`set_output_name_with_version()` (`extra/cmake/Synergy.cmake:110-132`), called from all three app `CMakeLists.txt` files, gives every executable a version suffix: `synergy-1.21.2`, `synergy-core-1.21.2`, `synergy-daemon-1.21.2`. Consumers still use unversioned names:

| Location | Reference | Actual artifact |
|---|---|---|
| `src/lib/common/CMakeLists.txt:4` → `Constants.h.in:24,35` | `synergy-core[.exe]` | `synergy-core-1.21.2[.exe]` |
| `src/lib/gui/core/CoreProcess.cpp:107-111` | `<exeDir>/synergy-core` | same |
| `src/apps/deskflow-daemon/DaemonApp.cpp:92` | `<exeDir>/synergy-core` | same |
| `src/lib/platform/win32/MSWindowsWatchdog.cpp:39` | compares to `kCoreBinNameW` | same |
| `Constants.h.in:15` | `synergy-daemon` | `synergy-daemon-1.21.2` |
| `extra/deploy/linux/com.symless.synergy.desktop:9` | `Exec=synergy` | `synergy-1.21.2` |
| `.github/actions/test-package/action.yml:20,25,44` | probes `synergy-core` | same |

No unversioned alias or symlink is produced anywhere (no `create_symlink` in the tree; the versioned `OUTPUT_NAME` is the only one set). `CoreProcess.cpp:108` fails its existence check and logs `core server binary does not exist`.

**Impact**: the GUI may not be able to launch the core; the Linux launcher points at a missing file; the CI packaging probe fails.

**Resolution (2026-09-22)**: confirmed empirically once a build succeeded, and the failure was worse than predicted — it also broke packaging outright. `deploy/windows/wix-patch.xml.in` keys its WiX fragments on the component IDs CPack derives from the installed file names (`CM_CP_synergy_core.exe`, `CM_CP_synergy_daemon.exe`), so `package` aborted with *"Some XML patch fragments did not have matching IDs"*.

Fixed by dropping the version from executable names: `set_output_name_with_version()` became `set_output_name()` (`extra/cmake/Synergy.cmake`) and now sets `synergy` / `synergy-core` / `synergy-daemon`, matching every consumer listed above. The version remains available via `--version`, the binary version resource, and package file names.

Evidence: `build/bin/Release/` now contains `synergy.exe`, `synergy-core.exe`, `synergy-daemon.exe`; the portable package contains the same unversioned names. The WiX failure that blocked packaging — *"Some XML patch fragments did not have matching IDs"* — is gone; `package` now proceeds past the patch stage and only stops at an unrelated external constraint (the installed WiX v7 requires accepting its OSMF EULA).

#### U-02 — Linux install source paths resolve to a non-existent identity

`deploy/linux/deploy.cmake:10` and `:30` install `${CMAKE_SOURCE_DIR}/extra/deploy/linux/${CMAKE_PROJECT_REV_FQDN}.desktop` and `.metainfo.xml`. `CMAKE_PROJECT_REV_FQDN` is defined in exactly one place — `extra/cmake/Synergy.cmake:10` = `com.tupig.synergy`. The files on disk are `com.symless.synergy.desktop` and `com.symless.synergy.metainfo.xml`.

The same file (`:17`) renames the icon to `com.tupig.synergy.png`, while the desktop entry declares `Icon=com.symless.synergy` (`com.symless.synergy.desktop:10`) — so the icon would not resolve even if it installed.

**Impact**: Linux DEB/RPM packaging fails or produces packages without a desktop entry.

#### U-03 — Three coexisting product identities

| Identity | Where it appears | Role |
|---|---|---|
| `org.deskflow.deskflow` | `deploy/linux/*.desktop`, `.metainfo.xml`, `.png`; `deploy/linux/flatpak/org.deskflow.deskflow.yml`; `src/apps/res/deskflow.qrc` (`icons/deskflow-{dark,light}/`, `apps/64/org.deskflow.deskflow.svg`) | Upstream; CI no longer uses it |
| `com.symless.synergy` | `extra/deploy/linux/*`; `extra/deploy/linux/flatpak/com.symless.synergy.yml`; `.github/workflows/ci.yml:678,681,687`; `extra/src/apps/res/synergy.qrc:7-10` | **Upstream commercial vendor** — but it is the identity CI actually builds |
| `com.tupig.synergy` | `extra/cmake/Synergy.cmake:10` | Used by `deploy/linux/deploy.cmake` install paths and by macOS `BUNDLE_GUI_IDENTIFIER` (`src/apps/deskflow-gui/CMakeLists.txt:24`) |

**Impact**: macOS and Linux ship different application identities; the Linux install path asks for one identity while only files for another exist.

**Decision needed**: which identity is canonical, then align the other two.

#### U-04 — macOS bundle icon filename does not exist

On `APPLE`, `target = CMAKE_PROJECT_PROPER_NAME` = `"TuPig Synergy"` (`src/apps/deskflow-gui/CMakeLists.txt:6`), so `:25` sets `BUNDLE_ICON_FILE` to `TuPig Synergy.icns` and `:35` points at `extra/deploy/mac/bundle/Contents/Resources/TuPig Synergy.icns`. That directory contains only `Synergy.icns` and `Volume.icns`. The path is added to `add_executable` (`:39-44`), so macOS configuration fails on a missing source file.

**Resolution (2026-09-23)**: confirmed by reading the code path — `CMAKE_PROJECT_PROPER_NAME` is `"TuPig Synergy"` (`extra/cmake/Synergy.cmake:6`), so `${target}.icns` really does expand to `TuPig Synergy.icns`. The stray `Volume.icns` was deleted and `Synergy.icns` renamed to `TuPig Synergy.icns`, matching the reference. The removed `Volume.icns` belonged to the dead `dmgbuild` mechanism (U-05).

#### U-05 — Dead macOS plist templates

`extra/deploy/mac/bundle/Contents/Info.plist.in` and `PkgInfo.in` reference `@DESKFLOW_APP_NAME@`, `@DESKFLOW_APP_ID@`, `@DESKFLOW_MAC_BUNDLE_CODE@`, `@DESKFLOW_VERSION@` and `@DESKFLOW_BUILD_YEAR@`. Nothing in the tree sets those variables (the upstream changelog recorded in `deploy/linux/org.deskflow.deskflow.metainfo.xml:346-355` notes they were removed). No `configure_file` references this template either — the GUI uses `src/apps/res/deskflow.plist.in` (`src/apps/deskflow-gui/CMakeLists.txt:30`). `Info.plist.in:27` also still carries the upstream commercial copyright line.

**Resolution (2026-09-23)**: both templates deleted, along with the rest of the never-wired `dmgbuild` mechanism they belonged to (`dmgbuild/settings.py`, `Resources/Background.tiff`, `Resources/Volume.icns`). Verified beforehand that no `configure_file` and no build script referenced any of them. `deploy/mac/deploy.cmake` is the one live macOS packaging path.

#### U-06 — PKGBUILD conflicts with itself

`deploy/linux/arch/PKGBUILD.in:5-6` derives `pkgname=synergy-git` from `CMAKE_PROJECT_NAME`, but the `conflicts` array at `:13` still begins with `'synergy-git'` and ends with `'deskflow'`. The rebrand changed `_basename` without cleaning the list.

#### U-07 — Two icon themes compiled into the same binary

The GUI links both `../res/deskflow.qrc` and `extra/src/apps/res/synergy.qrc` (`src/apps/deskflow-gui/CMakeLists.txt:41-42`). The first ships `icons/deskflow-{dark,light}/` with the app icon named `org.deskflow.deskflow.svg`; the second ships `icons/synergy-{dark,light}/` (theme names `synergy-dark` / `synergy-light`) with the app icon named `com.symless.synergy.svg` (`extra/src/apps/res/synergy.qrc:5-10`). Two theme names and two app-icon identifiers coexist.

#### U-08 — Comments that contradict the code

- `src/lib/common/UrlConstants.h:13-15` says the domain is converted to reverse-DNS "e.g. org.deskflow"; the actual `kAppDomain` is `tupig.com` (`extra/cmake/Synergy.cmake:11`). Behaviour is fine; the comment is stale.
- `extra/cmake/Synergy.cmake:14-18` claims keeping `CMAKE_PROJECT_PROPER_NAME` makes paths and Windows globals "space-free" — but `"TuPig Synergy"` contains a space, and `Constants.h.in:36-40` uses it to build `Global\TuPig SynergyClose`-style names, with `MSWindowsScreen.cpp:112,785` using it as a Win32 window class name.
- `extra/cmake/Synergy.cmake:66` says "Base semver lives in `./VERSION` (read by the root CMakeLists.txt)". There is no `VERSION` file; the root `CMakeLists.txt:24` includes `extra/cmake/Version.cmake`.

#### U-09 — Directory names and file names use different identifiers

Directories are derived from `kAppName` (`TuPig Synergy`) while file names inside them use `kAppId` (`synergy`) — e.g. `~/.config/TuPig Synergy/` alongside `~/synergy.log`. See `src/lib/common/Settings.h:22-33` and `Settings.cpp:178,184,193,210`. `kUpstreamId` (`deskflow`) is also retained in `Constants.h.in:12`.

**Decision needed**: intentional, or to be aligned.

#### U-10 — Vendor strings still name the upstream commercial vendor

`extra/deploy/linux/com.symless.synergy.metainfo.xml:9-11` declares developer `com.symless` / "Synergy App Ltd", and `:20` sets the homepage to the upstream commercial site. `extra/deploy/linux/com.symless.synergy.desktop:1`, `extra/deploy/PackageFileName.cmake:1` and `Info.plist.in:27` carry the same copyright line.

**Decision needed**: legitimate upstream attribution, or to be replaced.

#### U-11 — Three bilingual documentation formats

| Format | Files |
|---|---|
| Sectioned (`## English` + `## 中文`) | `build.md`, `configuration.md`, `troubleshooting.md`, `architecture.md`, `protocol.md`, `contributing.md`, `delivery.md`, `consistency-audit.md`, `security.md`, `.github/CONTRIBUTING.md`, `.github/CODE_OF_CONDUCT.md` |
| Chinese only, no English | `HANDOFF.md`, `.github/ISSUE_TEMPLATE/security-quality-refactoring.md` |
| English only, no Chinese | `.github/pull_request_template.md` |

**Fixed 2026-09-23**: `security.md` was converted from the inline per-heading pattern
to sectioned. `mcp-integration.md` was Chinese-only prose behind bilingual headings and
is now **deleted** — it described an integration with no code anywhere in the tree.
`.github/CODE_OF_CONDUCT.md` gained a Chinese section while keeping the upstream English
body unedited. `FORK.md` was merged into `contributing.md` as an "Upstream Sync" section
and deleted; that let `.github/CONTRIBUTING.md` become a short pointer to
`docs/contributing.md` (its previous wiki link pointed at a repository that does not
exist, and `REUSE.toml` carried the same wrong URL).

The three files still outside the bilingual convention are operational rather than
reference material — `HANDOFF.md` and the issue template are session-scoped, and the
pull request template is consumed by GitHub's PR form — so they were left as they are.

`docs/build.md` also carries a `## Build Presets & Scripts / 构建预设与脚本` section after
the Chinese section. It was Chinese-only and sat outside the bilingual structure; it now
carries bilingual headings, so it no longer breaks the convention.

#### U-12 — Stale documentation references

- `docs/build.md:525` references `CMakeUserPresets.json`, deleted in Phase 0 (`security-quality-refactoring.md:102`, `HANDOFF.md:48`).
- `docs/build.md:527-548` documents presets named `release` / `debug`. The real presets in `CMakePresets.json` are `windows-msvc`, `windows-msvc-release`, `windows-msvc-debug`, `linux`, `linux-release`, `macos`, `macos-release` — so `cmake --preset=release` fails.
- `docs/HANDOFF.md:98,122,123` reference `docs/phase2-qt-network-migration.md` and `docs/optimization-plan.md`; both now live in `docs/archive/`.

**Resolution (2026-09-23)**: all three corrected. `build.md` now documents the real
presets (`windows-msvc`, `windows-msvc-release`, `windows-msvc-debug`, `linux`,
`linux-release`, `macos`, `macos-release`, plus the diagnostics presets) and states
explicitly that there is no `CMakeUserPresets.json`. `HANDOFF.md` now records that the
two archived documents were deleted rather than moved.

#### U-13 — Hardcoded version numbers

The single source of truth is `extra/cmake/Version.cmake`, but `1.21.2` is hardcoded in `vcpkg.json` (`version-string`), `docs/HANDOFF.md:16`, `docs/protocol.md:38,256`, `docs/troubleshooting.md:282,568` and `docs/archive/optimization-plan.md:247-251`. All of these go stale on a version bump.

#### U-14 — `HANDOFF.md` has drifted

`docs/HANDOFF.md:2` says the current branch is `main`; `:13` gives a repo URL under `github.com/Tupig/TuPig_Product/tree/main/synergy`; `:71-79` lists a "latest 6 commits" set whose newest entry is `13d448213` while HEAD is `88926ce09`; `:115-124` lists `HANDOFF.md` without the `docs/` prefix it now has.

#### U-15 — Licence description disagrees across files

`README.md` states "GNU General Public License v2.0"; source SPDX headers and `deploy/linux/arch/PKGBUILD.in:12` use `GPL-2.0-only WITH LicenseRef-OpenSSL-Exception`; `vcpkg.json` (`license`) uses `GPL-2.0-only` without the exception.

#### U-16 — CI enforces the unversioned names and the upstream identity

- `.github/actions/test-package/action.yml:20,25,44` probes unversioned `synergy-core` / `synergy-core.exe` (also U-01).
- `.github/workflows/ci.yml:678,681,687` lints and builds `extra/deploy/linux/com.symless.synergy.*`, actively enforcing the upstream commercial identity (see U-03).
- `.github/workflows/ci.yml:179` assumes unversioned artifact names in a comment.
- `.github/ISSUE_TEMPLATE/security-quality-refactoring.md` is a status tracker, not an issue template, and is filed where templates live.

#### U-17 — Four product display names

| Name | Source | Used for |
|---|---|---|
| `TuPig Synergy` | `CMAKE_PROJECT_PROPER_NAME` | config/data directories, Windows global object names, `Constants.h.in:10` |
| `TuPig Synergy 1` | default `SYNERGY_DISPLAY_NAME` | window title, About dialog (`extra/cmake/Synergy.cmake:14-24`) |
| `TuPig Synergy Core` | `SYNERGY_CORE_FLAVOR=ON` | same |
| `Synergy` | hardcoded in `extra/deploy/linux/*` | Linux desktop entry, AppStream |

#### U-18 — The `deskflow` / `synergy` boundary and a fragile i18n coupling

Internal identifiers deliberately keep `deskflow` to stay mergeable with upstream (see the "Upstream Sync" section of `docs/contributing.md`): the `deskflow::` namespace, `src/lib/deskflow/`, `src/apps/deskflow-*/` including the `deskflow-*.cpp` filenames, `src/unittests/deskflow/`, `src/apps/res/deskflow.qrc|deskflow.plist.in`, and the `set(filename ...)` values in the CMake files.

**Warning**: the translation chain depends on this name. Translations are `translations/deskflow_*.ts` (`translations/CMakeLists.txt:7`), and at runtime `src/lib/common/I18N.cpp:41,178` filters for files matching `kUpstreamId`. `kUpstreamId` (`Constants.h.in:12`) is `@UPSTREAM_PROJECT_NAME@`, assigned at root `CMakeLists.txt:58` *before* `extra/cmake/Synergy.cmake:64` renames the project, so its value is `deskflow`. `src/unittests/common/I18NTests.cpp:34,107` hardcodes the same names.

The chain is currently consistent, so translations load correctly. But renaming the `.ts` files or `kUpstreamId` to `synergy` would silently disable them. Any unification of these three sites must be done together.

#### U-19 — Overlay layout and test placement

- The overlay uses two layouts: `extra/src/apps/res/` (mirrors `apps/res`) and `extra/src/lib/synergy/gui/` (carries a product-name directory).
- Test placement is inconsistent for the same platform: `X11LayoutParserTests` sits in `src/unittests/deskflow/` while `XWindowsClipboardTests` sits in `src/unittests/platform/`.

#### U-20 — Documented CLI option does not exist

**Severity**: P2 · **Verification**: code search (no build required)

**Fixed.**

`docs/troubleshooting.md` told users to run `synergy-core --install-service` when service mode
fails. That option does not exist: the core's only options are `CoreArgs.h`'s `help` / `version` /
`new-instance` / `settings`, and `rg` finds no `install-service` anywhere under `src/`.

This is worse than a cosmetic doc error, because it hides the fact that **nothing in the codebase
can register the Windows service** — there is no `CreateService` call, and `IArchDaemon.h` declares
only `daemonize()` and `commandLine()` (its class comment still claimed install/uninstall support).
Following the documented command just fails, leaving the user unable to obtain UAC-prompt or
login-screen support.

**Resolution**: replaced the bogus command with the two mechanisms that actually work — the MSI
(which declares `ServiceInstall` for `synergy-daemon.exe`) and manual `sc create` — in
`docs/troubleshooting.md`, in both languages. Corrected the stale comment on `IArchDaemon`. Recorded
the constraint in `AGENTS.md` and `docs/delivery.md` so a future agent does not implement a
self-install option merely to make the old documentation true.

### Verified consistent (not defects)

These were checked and are **not** problems — recorded so a future audit does not re-open them:

- **Translation loading**: `.ts` names, the `kUpstreamId` filter and `I18NTests` all agree (U-18 explains why).
- **CI package naming**: `PACKAGE_PREFIX` defaults to `synergy` (`ci.yml:48`) and the `synergy_<version>_<os>_<arch>` scheme matches `extra/deploy/PackageFileName.cmake:46`.
- **`PACKAGE_VERSION_LABEL`**: derived in `deploy/CMakeLists.txt:22-23` from the same version function, and not overridden by CI.

### Verification needed

U-01 could not be confirmed empirically: `build/` is empty and `cmake`/`ninja` are not on `PATH` locally (`HANDOFF.md:158`). Confirm by configuring and building once, then checking whether `synergy-core-1.21.2[.exe]` exists and whether the GUI starts the core. If it does, U-01 is confirmed.

---

## 中文

### 目的

本文档是**问题清单**，不是修复计划。每条问题都有稳定 ID（`U-nn`），便于修复、提交和 issue 互相引用。同一优先级内未按修复顺序排列。

结论来自静态分析（ripgrep + 通读构建与打包文件）。本地无法构建，因此运行时行为未被实证 —— 见[待验证项](#待验证项)。

### 使用方式

1. 取一个 ID 修复后，更新此处的**状态**。
2. 按 `docs/HANDOFF.md` §1.3 的约束，配套的代码改动必须同步记录到 `.github/ISSUE_TEMPLATE/security-quality-refactoring.md`。
3. 关闭条目时必须给出验证证据（构建日志、打包产物列表或实际运行），不能仅凭推理就把状态改成 Fixed。

### 汇总

| ID | 级别 | 范围 | 结论 | 状态 |
|---|---|---|---|---|
| U-01 | P0 | 运行时 / 构建 | 产物名带版本号，引用处不带 | **已修复**（见下） |
| U-02 | P0 | 打包 (Linux) | 安装源路径指向一个没有文件的身份 | 待处理 |
| U-03 | P1 | 打包 (全平台) | 三套产品身份并存；macOS 与 Linux 不一致 | 待处理 |
| U-04 | P1 | 打包 (macOS) | Bundle 图标文件名不存在 | **已修复** |
| U-05 | P2 | 打包 (macOS) | 死模板引用已被删除的变量 | **已修复** |
| U-06 | P2 | 打包 (Arch) | PKGBUILD 声明与自己冲突 | 待处理 |
| U-07 | P2 | 运行时 (图标) | 两套图标主题编进同一个二进制 | 待处理 |
| U-08 | P3 | 注释 | 三处注释与代码相反 | 待处理 |
| U-09 | P3 | 运行时 (路径) | 目录名与文件名用了不同的应用标识 | 待处理 |
| U-10 | P3 | 打包 | 供应商/版权字样仍指上游商业厂商 | 待处理 |
| U-11 | P1 | 文档 | 三种双语格式并存；两份文档没有英文 | **已修复** |
| U-12 | P1 | 文档 | 过时引用：预设、已删除文件、已移动文档 | **已修复** |
| U-13 | P2 | 文档 | 版本号硬编码在六处 | 待处理 |
| U-14 | P2 | 文档 | `HANDOFF.md` 与实际状态脱节 | 待处理 |
| U-15 | P2 | 文档 | 许可描述在四个文件里不一致 | 待处理 |
| U-16 | P2 | CI | CI 强制无版本产物名与上游厂商身份 | 待处理 |
| U-17 | P3 | 命名 | 四个不同的产品显示名 | 待处理 |
| U-18 | P3 | 命名 | `deskflow` / `synergy` 边界，以及一个脆弱的 i18n 耦合 | 待处理 |
| U-19 | P3 | 命名 | overlay 目录结构与测试归属不一致 | 待处理 |
| U-20 | P2 | 文档 | 文档中的 CLI 选项 `--install-service` 并不存在 | **已修复** |

本次已修复：`README.md`、`setup.bat`、`docs/build.md`、`docs/configuration.md`、`src/apps/res/manpage.txt` 中过时的产物文件名（提交 `baa5afe76`，分支 `cursor/docs-fix-binary-names`）。

### 问题明细

#### U-01 — 产物名带版本号，引用处不带

**级别**: P0 · **验证**: 需要一次真实构建（见下）

`set_output_name_with_version()`（`extra/cmake/Synergy.cmake:110-132`，被三个 app 的 CMakeLists 调用）给每个可执行文件加了版本后缀：`synergy-1.21.2`、`synergy-core-1.21.2`、`synergy-daemon-1.21.2`。引用处仍在用无版本名：

| 位置 | 引用 | 实际产物 |
|---|---|---|
| `src/lib/common/CMakeLists.txt:4` → `Constants.h.in:24,35` | `synergy-core[.exe]` | `synergy-core-1.21.2[.exe]` |
| `src/lib/gui/core/CoreProcess.cpp:107-111` | `<exeDir>/synergy-core` | 同上 |
| `src/apps/deskflow-daemon/DaemonApp.cpp:92` | `<exeDir>/synergy-core` | 同上 |
| `src/lib/platform/win32/MSWindowsWatchdog.cpp:39` | 与 `kCoreBinNameW` 比对 | 同上 |
| `Constants.h.in:15` | `synergy-daemon` | `synergy-daemon-1.21.2` |
| `extra/deploy/linux/com.symless.synergy.desktop:9` | `Exec=synergy` | `synergy-1.21.2` |
| `.github/actions/test-package/action.yml:20,25,44` | 探测 `synergy-core` | 同上 |

全仓库没有任何地方生成无版本别名或软链（没有 `create_symlink`，唯一的 `OUTPUT_NAME` 设置点就是那个加版本号的函数）。`CoreProcess.cpp:108` 的存在性检查会失败并打印 `core server binary does not exist`。

**影响**：GUI 可能无法启动 core；Linux 启动器指向不存在的文件；CI 打包校验失败。

**已修复（2026-09-22）**：构建成功后被实证确认，且后果比预估更严重 —— 它还直接导致**打包失败**。`deploy/windows/wix-patch.xml.in` 以 CPack 依据安装后文件名推导的组件 ID 作为 WiX 片段键（`CM_CP_synergy_core.exe`、`CM_CP_synergy_daemon.exe`），因此 `package` 直接中止并报 *“Some XML patch fragments did not have matching IDs”*。

修法：去掉可执行文件名中的版本号。`set_output_name_with_version()` 改为 `set_output_name()`（`extra/cmake/Synergy.cmake`），产出 `synergy` / `synergy-core` / `synergy-daemon`，与上表全部引用方一致。版本信息未丢失：仍由 `--version`、二进制版本资源与安装包文件名承载。

#### U-02 — Linux 安装源路径指向不存在的身份

`deploy/linux/deploy.cmake:10` 与 `:30` 安装 `${CMAKE_SOURCE_DIR}/extra/deploy/linux/${CMAKE_PROJECT_REV_FQDN}.desktop` 与 `.metainfo.xml`。`CMAKE_PROJECT_REV_FQDN` 全仓库只在一处定义 —— `extra/cmake/Synergy.cmake:10` = `com.tupig.synergy`。而磁盘上的文件是 `com.symless.synergy.desktop` 与 `com.symless.synergy.metainfo.xml`。

同一文件（`:17`）把图标重命名为 `com.tupig.synergy.png`，但桌面项里声明的是 `Icon=com.symless.synergy`（`com.symless.synergy.desktop:10`）—— 即使装上图标也解析不到。

**影响**：Linux DEB/RPM 打包失败，或产出缺少桌面项的包。

#### U-03 — 三套产品身份并存

| 身份 | 出现位置 | 角色 |
|---|---|---|
| `org.deskflow.deskflow` | `deploy/linux/*.desktop`、`.metainfo.xml`、`.png`；`deploy/linux/flatpak/org.deskflow.deskflow.yml`；`src/apps/res/deskflow.qrc`（`icons/deskflow-{dark,light}/`、`apps/64/org.deskflow.deskflow.svg`） | 上游；CI 已不再使用 |
| `com.symless.synergy` | `extra/deploy/linux/*`；`extra/deploy/linux/flatpak/com.symless.synergy.yml`；`.github/workflows/ci.yml:678,681,687`；`extra/src/apps/res/synergy.qrc:7-10` | **上游商业厂商** —— 但 CI 实际构建的就是它 |
| `com.tupig.synergy` | `extra/cmake/Synergy.cmake:10` | 被 `deploy/linux/deploy.cmake` 的安装路径和 macOS `BUNDLE_GUI_IDENTIFIER`（`src/apps/deskflow-gui/CMakeLists.txt:24`）使用 |

**影响**：macOS 与 Linux 发布出不同的应用身份；Linux 安装路径要的身份与磁盘上存在的文件不是同一个。

**需要决策**：哪个身份是权威，再让另外两个对齐。

#### U-04 — macOS Bundle 图标文件名不存在

`APPLE` 时 `target = CMAKE_PROJECT_PROPER_NAME` = `"TuPig Synergy"`（`src/apps/deskflow-gui/CMakeLists.txt:6`），于是 `:25` 的 `BUNDLE_ICON_FILE` 与 `:35` 的图标源都是 `TuPig Synergy.icns`。该目录下只有 `Synergy.icns` 与 `Volume.icns`。该路径被加入 `add_executable`（`:39-44`），macOS 配置期会因找不到源文件直接失败。

#### U-05 — macOS 死模板

`extra/deploy/mac/bundle/Contents/Info.plist.in` 与 `PkgInfo.in` 引用 `@DESKFLOW_APP_NAME@`、`@DESKFLOW_APP_ID@`、`@DESKFLOW_MAC_BUNDLE_CODE@`、`@DESKFLOW_VERSION@`、`@DESKFLOW_BUILD_YEAR@`。全仓库没有任何地方定义这些变量（`deploy/linux/org.deskflow.deskflow.metainfo.xml:346-355` 记录的上游 changelog 说明它们已被删除）。也没有任何 `configure_file` 引用该模板 —— GUI 用的是 `src/apps/res/deskflow.plist.in`（`src/apps/deskflow-gui/CMakeLists.txt:30`）。`Info.plist.in:27` 还留着上游商业厂商的版权行。

#### U-06 — PKGBUILD 与自己冲突

`deploy/linux/arch/PKGBUILD.in:5-6` 由 `CMAKE_PROJECT_NAME` 得出 `pkgname=synergy-git`，但 `:13` 的 `conflicts` 数组第一个元素仍是 `'synergy-git'`，末尾还留着 `'deskflow'`。rebrand 改了 `_basename` 却没清理这个列表。

#### U-07 — 两套图标主题编进同一个二进制

GUI 同时链接 `../res/deskflow.qrc` 与 `extra/src/apps/res/synergy.qrc`（`src/apps/deskflow-gui/CMakeLists.txt:41-42`）。前者带 `icons/deskflow-{dark,light}/`，app 图标名为 `org.deskflow.deskflow.svg`；后者带 `icons/synergy-{dark,light}/`（主题名 `synergy-dark` / `synergy-light`），app 图标名为 `com.symless.synergy.svg`（`extra/src/apps/res/synergy.qrc:5-10`）。两套主题名与两种 app 图标标识并存。

#### U-08 — 与代码相反的注释

- `src/lib/common/UrlConstants.h:13-15` 说该域名会被转成反向域名 “e.g. org.deskflow”；实际 `kAppDomain` 是 `tupig.com`（`extra/cmake/Synergy.cmake:11`）。功能没问题，注释过时。
- `extra/cmake/Synergy.cmake:14-18` 称保留 `CMAKE_PROJECT_PROPER_NAME` 可让路径与 Windows 全局对象名 “space-free” —— 但 `"TuPig Synergy"` 本身带空格，且 `Constants.h.in:36-40` 正是用它拼 `Global\TuPig SynergyClose` 这类全局名，`MSWindowsScreen.cpp:112,785` 还把它当 Win32 窗口类名。
- `extra/cmake/Synergy.cmake:66` 说 “Base semver lives in `./VERSION` (read by the root CMakeLists.txt)”。仓库没有 `VERSION` 文件；根 `CMakeLists.txt:24` 包含的是 `extra/cmake/Version.cmake`。

#### U-09 — 目录名与文件名用不同标识

目录名来自 `kAppName`（`TuPig Synergy`），目录内文件名用 `kAppId`（`synergy`）—— 例如 `~/.config/TuPig Synergy/` 与 `~/synergy.log` 并存。见 `src/lib/common/Settings.h:22-33` 与 `Settings.cpp:178,184,193,210`。`kUpstreamId`（`deskflow`）也保留在 `Constants.h.in:12`。

**需要决策**：是否有意如此，还是应对齐。

#### U-10 — 供应商字样仍指上游商业厂商

`extra/deploy/linux/com.symless.synergy.metainfo.xml:9-11` 的 developer 是 `com.symless` / “Synergy App Ltd”，`:20` 的 homepage 指向上游商业站点。`extra/deploy/linux/com.symless.synergy.desktop:1`、`extra/deploy/PackageFileName.cmake:1`、`Info.plist.in:27` 也带同一版权行。

**需要决策**：算合法的上游归属声明，还是要替换。

#### U-11 — 三种双语格式并存

| 格式 | 文件 |
|---|---|
| 分节式（`## English` + `## 中文`） | `build.md`、`configuration.md`、`troubleshooting.md`、`architecture.md`、`protocol.md`、`contributing.md`、`delivery.md`、`consistency-audit.md`、`security.md`、`.github/CONTRIBUTING.md`、`.github/CODE_OF_CONDUCT.md` |
| 纯中文、无英文 | `HANDOFF.md`、`.github/ISSUE_TEMPLATE/security-quality-refactoring.md` |
| 纯英文、无中文 | `.github/pull_request_template.md` |

**已修复（2026-09-23）**：`security.md` 由「行内对照」改为分节式。`mcp-integration.md` 原为
「标题双语、正文纯中文」，现已**删除** —— 它描述的集成在代码库中没有任何实现。
`.github/CODE_OF_CONDUCT.md` 增补中文节，上游英文正文保持不改。`FORK.md` 已并入
`contributing.md` 的「上游同步」一节并删除；因此 `.github/CONTRIBUTING.md` 得以改为指向
`docs/contributing.md` 的短指针（其原 wiki 链接指向一个不存在的仓库，`REUSE.toml` 也带着
同一个错误 URL）。

仍未遵循双语约定的三份文件属操作类而非参考资料 —— `HANDOFF.md` 与 issue 模板是会话范围内
的文档，pull request 模板由 GitHub 的 PR 表单消费 —— 故保持原样。

`docs/build.md` 在中文段之后还有一个 `## Build Presets & Scripts / 构建预设与脚本` 段。它此前只有中文、
位于双语结构之外，现已改为双语标题，不再违反约定。

#### U-12 — 文档过时引用

- `docs/build.md:525` 提到 `CMakeUserPresets.json`，该文件已在 Phase 0 删除（`security-quality-refactoring.md:102`、`HANDOFF.md:48`）。
- `docs/build.md:527-548` 记录的预设名是 `release` / `debug`。`CMakePresets.json` 里实际是 `windows-msvc`、`windows-msvc-release`、`windows-msvc-debug`、`linux`、`linux-release`、`macos`、`macos-release` —— 敲 `cmake --preset=release` 会失败。
- `docs/HANDOFF.md:98,122,123` 引用 `docs/phase2-qt-network-migration.md` 与 `docs/optimization-plan.md`，两者现已在 `docs/archive/`。

**已修复（2026-09-23）**：三处均已更正。`build.md` 现记录真实预设（`windows-msvc`、
`windows-msvc-release`、`windows-msvc-debug`、`linux`、`linux-release`、`macos`、
`macos-release`，以及诊断类预设），并明确说明本仓库不含 `CMakeUserPresets.json`。
`HANDOFF.md` 现写明那两份归档文档是**被删除**而非迁移。

#### U-13 — 版本号硬编码

单一来源是 `extra/cmake/Version.cmake`，但 `1.21.2` 被硬编码在 `vcpkg.json`（`version-string`）、`docs/HANDOFF.md:16`、`docs/protocol.md:38,256`、`docs/troubleshooting.md:282,568`、`docs/archive/optimization-plan.md:247-251`。版本一升全部失效。

#### U-14 — `HANDOFF.md` 与实际脱节

`docs/HANDOFF.md:2` 称当前分支是 `main`；`:13` 给的仓库 URL 指向 `github.com/Tupig/TuPig_Product/tree/main/synergy`；`:71-79` 的“最新 6 个提交”最新一条是 `13d448213`，而 HEAD 已是 `88926ce09`；`:115-124` 里列 `HANDOFF.md` 时没带它现在所属的 `docs/` 前缀。

#### U-15 — 许可描述不一致

`README.md` 写 “GNU General Public License v2.0”；源码 SPDX 头与 `deploy/linux/arch/PKGBUILD.in:12` 用的是 `GPL-2.0-only WITH LicenseRef-OpenSSL-Exception`；`vcpkg.json` 的 `license` 字段是 `GPL-2.0-only`，漏了 exception。

#### U-16 — CI 强制的命名与身份

- `.github/actions/test-package/action.yml:20,25,44` 探测无版本名 `synergy-core` / `synergy-core.exe`（同 U-01）。
- `.github/workflows/ci.yml:678,681,687` 校验并构建 `extra/deploy/linux/com.symless.synergy.*`，等于在 CI 上主动强制上游商业厂商身份（见 U-03）。
- `.github/workflows/ci.yml:179` 的注释假定产物无版本名。
- `.github/ISSUE_TEMPLATE/security-quality-refactoring.md` 是状态追踪文档而非 issue 模板，位置不当。

#### U-17 — 四个产品显示名

| 名称 | 来源 | 用途 |
|---|---|---|
| `TuPig Synergy` | `CMAKE_PROJECT_PROPER_NAME` | 配置/数据目录名、Windows 全局对象名、`Constants.h.in:10` |
| `TuPig Synergy 1` | `SYNERGY_DISPLAY_NAME` 默认值 | 窗口标题、关于框（`extra/cmake/Synergy.cmake:14-24`） |
| `TuPig Synergy Core` | `SYNERGY_CORE_FLAVOR=ON` | 同上 |
| `Synergy` | 写死在 `extra/deploy/linux/*` | Linux 桌面项、AppStream |

#### U-18 — `deskflow` / `synergy` 边界与一个脆弱的 i18n 耦合

内部标识有意保留 `deskflow`，以便与上游合并（见 `docs/contributing.md` 的「上游同步」一节）：`deskflow::` 命名空间、`src/lib/deskflow/`、`src/apps/deskflow-*/`（含 `deskflow-*.cpp` 文件名）、`src/unittests/deskflow/`、`src/apps/res/deskflow.qrc|deskflow.plist.in`，以及 CMake 里的 `set(filename ...)`。

**警告**：翻译链路依赖这个名字。翻译文件是 `translations/deskflow_*.ts`（`translations/CMakeLists.txt:7`），运行时 `src/lib/common/I18N.cpp:41,178` 按 `kUpstreamId` 过滤文件。`kUpstreamId`（`Constants.h.in:12`）取值 `@UPSTREAM_PROJECT_NAME@`，它在根 `CMakeLists.txt:58` 赋值，**早于** `extra/cmake/Synergy.cmake:64` 把项目改名为 `synergy`，所以值是 `deskflow`。`src/unittests/common/I18NTests.cpp:34,107` 也硬编码了同样的名字。

该链路目前是自洽的，翻译能正常加载。但若把 `.ts` 文件或 `kUpstreamId` 改成 `synergy`，翻译会静默失效。要统一这三处必须一起改。

#### U-19 — overlay 结构与测试归属

- overlay 用了两种布局：`extra/src/apps/res/`（镜像 `apps/res`）与 `extra/src/lib/synergy/gui/`（带产品名目录）。
- 同一平台的测试归属不一致：`X11LayoutParserTests` 在 `src/unittests/deskflow/`，而 `XWindowsClipboardTests` 在 `src/unittests/platform/`。

#### U-20 — 文档中的 CLI 选项并不存在

**级别**: P2 · **验证**: 代码检索（无需构建）

**已修复。**

`docs/troubleshooting.md` 在「服务模式失败」时让用户执行 `synergy-core --install-service`，但该选项
并不存在：core 的选项只有 `CoreArgs.h` 中的 `help` / `version` / `new-instance` / `settings`，且
在 `src/` 下检索 `install-service` 无任何命中。

这比表面上的文档错误更严重 —— 它掩盖了一个事实：**本代码库无法注册 Windows 服务**。代码中没有
`CreateService` 调用，`IArchDaemon.h` 也只声明了 `daemonize()` 与 `commandLine()`（其类注释却仍称
支持安装/卸载）。照文档执行只会得到失败，用户因此拿不到 UAC 提示与登录界面支持。

**修法**：在 `docs/troubleshooting.md` 中把无效命令替换为**真正可行的两条途径** —— MSI（为
`synergy-daemon.exe` 声明了 `ServiceInstall`）与手工 `sc create`，中英双语；修正 `IArchDaemon` 的
过时注释；并把该约束记入 `AGENTS.md` 与 `docs/delivery.md`，以免后续 agent 为了让旧文档「成真」
而贸然实现自安装选项。

### 已核实一致（不是缺陷）

以下各项已检查且**不是**问题 —— 记录下来，避免后续审计重复开单：

- **翻译加载**：`.ts` 命名、`kUpstreamId` 过滤条件与 `I18NTests` 三者一致（原因见 U-18）。
- **CI 产物命名**：`PACKAGE_PREFIX` 默认 `synergy`（`ci.yml:48`），`synergy_<version>_<os>_<arch>` 方案与 `extra/deploy/PackageFileName.cmake:46` 一致。
- **`PACKAGE_VERSION_LABEL`**：在 `deploy/CMakeLists.txt:22-23` 由同一个版本函数派生，CI 未覆盖。

### 待验证项

U-01 无法实证确认：`build/` 为空，且本地 `cmake`/`ninja` 不在 `PATH`（`HANDOFF.md:158`）。确认方式是配置并构建一次，检查是否存在 `synergy-core-1.21.2[.exe]`、以及 GUI 能否启动 core。若能，则 U-01 成立。
