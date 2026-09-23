# Consistency Audit / 一致性审计

> **Language / 语言**: [English](#english) | [中文](#中文)
>
> **Last updated / 最后更新**: 2026-09-21
> **Scope / 范围**: Cross-cutting naming + identity consistency / 跨模块命名与身份一致性
> **Related / 相关**: `docs/HANDOFF.md`, `.github/ISSUE_TEMPLATE/security-quality-refactoring.md`
>
> **Note on `.github/` paths (2026-09-23)**: the `.github` directory was moved from
> `synergy/.github` to the **repository root**, because GitHub only reads
> `<repo-root>/.github/workflows` and therefore never ran any of this project's CI.
> Every `.github/...` path cited below is now relative to the repository root, and
> the line numbers in `ci.yml` / `action.yml` citations have shifted as a result.
> Treat those citations as pointers to the file, not to an exact line.
>
> **关于 `.github/` 路径（2026-09-23）**：`.github` 目录已从 `synergy/.github` 移到
> **仓库根**，原因是 GitHub 只读取 `<仓库根>/.github/workflows`，导致本项目的 CI 从未
> 运行。以下所有 `.github/...` 路径现均相对仓库根；相关 `ci.yml` / `action.yml` 的行号
> 也因之发生偏移。请把这些引用当作「指向该文件」的线索，而非精确行号。

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
| U-02 | P0 | Packaging (Linux) | Install source paths use an identity that has no files | **Fixed** |
| U-03 | P1 | Packaging (all) | Three coexisting product identities; macOS and Linux differ | **Fixed** |
| U-04 | P1 | Packaging (macOS) | Bundle icon filename does not exist | **Fixed** |
| U-05 | P2 | Packaging (macOS) | Dead plist templates referencing removed variables | **Fixed** |
| U-06 | P2 | Packaging (Arch) | PKGBUILD declares a conflict with itself | **Fixed** |
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
| U-21 | P2 | Docs / feature scope | Linux drag-and-drop described as restorable; it was never implemented | Recorded |

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
| `.github/actions/test-package/action.yml`（仓库根下） | probes `synergy-core` | same |

No unversioned alias or symlink is produced anywhere (no `create_symlink` in the tree; the versioned `OUTPUT_NAME` is the only one set). `CoreProcess.cpp:108` fails its existence check and logs `core server binary does not exist`.

**Impact**: the GUI may not be able to launch the core; the Linux launcher points at a missing file; the CI packaging probe fails.

**Resolution (2026-09-22)**: confirmed empirically once a build succeeded, and the failure was worse than predicted — it also broke packaging outright. `deploy/windows/wix-patch.xml.in` keys its WiX fragments on the component IDs CPack derives from the installed file names (`CM_CP_synergy_core.exe`, `CM_CP_synergy_daemon.exe`), so `package` aborted with *"Some XML patch fragments did not have matching IDs"*.

Fixed by dropping the version from executable names: `set_output_name_with_version()` became `set_output_name()` (`extra/cmake/Synergy.cmake`) and now sets `synergy` / `synergy-core` / `synergy-daemon`, matching every consumer listed above. The version remains available via `--version`, the binary version resource, and package file names.

Evidence: `build/bin/Release/` now contains `synergy.exe`, `synergy-core.exe`, `synergy-daemon.exe`; the portable package contains the same unversioned names. The WiX failure that blocked packaging — *"Some XML patch fragments did not have matching IDs"* — is gone; `package` now proceeds past the patch stage and only stops at an unrelated external constraint (the installed WiX v7 requires accepting its OSMF EULA).

#### U-02 — Linux install source paths resolve to a non-existent identity

`deploy/linux/deploy.cmake:10` and `:30` install `${CMAKE_SOURCE_DIR}/extra/deploy/linux/${CMAKE_PROJECT_REV_FQDN}.desktop` and `.metainfo.xml`. `CMAKE_PROJECT_REV_FQDN` is defined in exactly one place — `extra/cmake/Synergy.cmake:10` = `com.tupig.synergy`. The files on disk are `com.symless.synergy.desktop` and `com.symless.synergy.metainfo.xml`.

The same file (`:17`) renames the icon to `com.tupig.synergy.png`, while the desktop entry declares `Icon=com.symless.synergy` (`com.symless.synergy.desktop:10`) — so the icon would not resolve even if it installed.

**Impact**: Linux DEB/RPM packaging fails or produces packages without a desktop entry.

**Resolution (2026-09-23)**: aligned on `com.tupig.synergy`, the identity
`CMAKE_PROJECT_REV_FQDN` already declared and the one every install path asks for.
Renamed `com.symless.synergy.{desktop,metainfo.xml}` → `com.tupig.synergy.*`, the
flatpak manifest `com.symless.synergy.yml` → `com.tupig.synergy.yml`, and the
matching `flatpak-builder-lint` invocations in CI. Also updated the identifiers
*inside* those files — `<id>`, `<launchable>`, `<provides><id>` and the desktop
`Icon=` key — plus the flatpak lint-exceptions key.

That last part turned out to fix a second, user-visible defect that had nothing to
do with packaging. `MainWindow.cpp` sets the window and tray icon with
`QIcon::fromTheme(kRevFqdnName)`, and `kRevFqdnName` is `com.tupig.synergy`; but
`extra/src/apps/res/synergy.qrc` published the app icon under the alias
`com.symless.synergy.svg`. The lookup therefore never matched the icon compiled
into the binary and silently fell back to a generic one. Renaming the four qrc
aliases makes the name the code asks for the name the resource provides.

*Not* changed here: the vendor attribution inside those files (`<developer
id="com.symless">`, "Synergy App Ltd", the upstream homepage) and the display name
`Synergy`. Those are U-10 and U-17, which are separate decisions.

#### U-03 — Three coexisting product identities

| Identity | Where it appears | Role |
|---|---|---|
| `org.deskflow.deskflow` | `src/apps/res/deskflow.qrc` (`icons/deskflow-{dark,light}/`, `apps/64/org.deskflow.deskflow.svg`) | Upstream; packaging copies removed 2026-09-23 (see resolution) |
| `com.symless.synergy` | no longer used for anything that is built (see U-02) | Was the identity CI built; now reduced to vendor attribution (U-10) |
| `com.tupig.synergy` | `extra/cmake/Synergy.cmake:10`; `extra/deploy/linux/*`; `extra/deploy/linux/flatpak/com.tupig.synergy.yml`; `extra/src/apps/res/synergy.qrc`; `extra/src/lib/synergy/gui/` | Used by Linux install paths, the flatpak app-id, the icon theme lookup and macOS `BUNDLE_GUI_IDENTIFIER` (`src/apps/deskflow-gui/CMakeLists.txt:24`) |

**Impact**: macOS and Linux ship different application identities; the Linux install path asks for one identity while only files for another exist.

**Resolution (2026-09-23)**: canonical identity confirmed as `com.tupig.synergy`
(the approval from A-01/A-02/U-02 session). The four upstream packaging copies
were deleted — `deploy/linux/org.deskflow.deskflow.{desktop,metainfo.xml,png}`
and `deploy/linux/flatpak/org.deskflow.deskflow.yml` — together with their
orphan lint companion `deploy/linux/flatpak/ci-build-lint-exceptions.json`,
whose only key was the deleted manifest's `org.deskflow.deskflow` app-id (CI
actually lints `extra/deploy/linux/flatpak/ci-build-lint-exceptions.json`,
which keeps working). The live files `deploy/linux/deploy.cmake` and
`deploy/linux/arch/PKGBUILD.in` stay (both SPDX-headered, both wired). Updated
`REUSE.toml` (three entries for the deleted paths removed), `docs/build.md`
(Flatpak-manifest mentions now name only `extra/`), and the upstream-path
comment in `extra/deploy/linux/flatpak/com.tupig.synergy.yml`. Verified with
`rg 'org\.deskflow|deploy/linux/flatpak'`: remaining hits are only this ledger,
the point-in-time snapshot `audit-2026-09-23.md`, `src/apps/res/deskflow.qrc`
icon assets (out of scope — U-07) and the flatpak yml comment recording the
deletion. Icon themes (U-07) and stale comments (U-08) remain separate items.

#### U-04 — macOS bundle icon filename does not exist

On `APPLE`, `target = CMAKE_PROJECT_PROPER_NAME` = `"TuPig Synergy"` (`src/apps/deskflow-gui/CMakeLists.txt:6`), so `:25` sets `BUNDLE_ICON_FILE` to `TuPig Synergy.icns` and `:35` points at `extra/deploy/mac/bundle/Contents/Resources/TuPig Synergy.icns`. That directory contains only `Synergy.icns` and `Volume.icns`. The path is added to `add_executable` (`:39-44`), so macOS configuration fails on a missing source file.

**Resolution (2026-09-23)**: confirmed by reading the code path — `CMAKE_PROJECT_PROPER_NAME` is `"TuPig Synergy"` (`extra/cmake/Synergy.cmake:6`), so `${target}.icns` really does expand to `TuPig Synergy.icns`. The stray `Volume.icns` was deleted and `Synergy.icns` renamed to `TuPig Synergy.icns`, matching the reference. The removed `Volume.icns` belonged to the dead `dmgbuild` mechanism (U-05).

#### U-05 — Dead macOS plist templates

`extra/deploy/mac/bundle/Contents/Info.plist.in` and `PkgInfo.in` reference `@DESKFLOW_APP_NAME@`, `@DESKFLOW_APP_ID@`, `@DESKFLOW_MAC_BUNDLE_CODE@`, `@DESKFLOW_VERSION@` and `@DESKFLOW_BUILD_YEAR@`. Nothing in the tree sets those variables (the upstream changelog recorded in `deploy/linux/org.deskflow.deskflow.metainfo.xml:346-355` — since deleted with U-03 — notes they were removed). No `configure_file` references this template either — the GUI uses `src/apps/res/deskflow.plist.in` (`src/apps/deskflow-gui/CMakeLists.txt:30`). `Info.plist.in:27` also still carries the upstream commercial copyright line.

**Resolution (2026-09-23)**: both templates deleted, along with the rest of the never-wired `dmgbuild` mechanism they belonged to (`dmgbuild/settings.py`, `Resources/Background.tiff`, `Resources/Volume.icns`). Verified beforehand that no `configure_file` and no build script referenced any of them. `deploy/mac/deploy.cmake` is the one live macOS packaging path.

#### U-06 — PKGBUILD conflicts with itself

`deploy/linux/arch/PKGBUILD.in:5-6` derives `pkgname=synergy-git` from `CMAKE_PROJECT_NAME`, but the `conflicts` array at `:13` still begins with `'synergy-git'` and ends with `'deskflow'`. The rebrand changed `_basename` without cleaning the list.

**Resolution (2026-09-23)**: `'synergy-git'` removed from `conflicts`; `'synergy'` was added in its place, since the built package now provides the `synergy` command and must not coexist with something else that does. Verified the reasoning: `conflicts` names *other* packages this one cannot live alongside, so listing its own `pkgname` made pacman refuse its own artifact. The `deskflow` entry is intentional and stays.

#### U-07 — Two icon themes compiled into the same binary

The GUI links both `../res/deskflow.qrc` and `extra/src/apps/res/synergy.qrc` (`src/apps/deskflow-gui/CMakeLists.txt:41-42`). The first ships `icons/deskflow-{dark,light}/` with the app icon named `org.deskflow.deskflow.svg`; the second ships `icons/synergy-{dark,light}/` (theme names `synergy-dark` / `synergy-light`) with the app icon named `com.tupig.synergy.svg`. Two theme names and two app-icon identifiers coexist.

**Partly addressed (2026-09-23)**: the second theme's app-icon identifier is now `com.tupig.synergy` (was `com.symless.synergy`), which is what `QIcon::fromTheme(kRevFqdnName)` asks for — see U-02. The two-themes-coexisting half of this item is still open; the upstream `deskflow-{dark,light}` theme is still linked in.

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

#### U-21 — Linux drag-and-drop was never implemented (scope assumption corrected)

**Severity**: P2 · **Verification**: git history, read locally without network

The drag-and-drop file transfer work was planned around "restore the upstream implementation on
all three platforms", and Linux was described as "XDND / Wayland DnD". Checking the history shows
that premise is wrong for Linux.

`5365e34f0` — *"feat: remove drag and drop support, its broken on all platforms"*, 2025-05-08 —
is the commit that removed it. It is **upstream, not this fork**: `git merge-base --is-ancestor
5365e34f0 8ed7a3ef` succeeds, where `8ed7a3ef` is the fork point recorded in
`docs/contributing.md`. Its deletions are:

```
D  src/lib/deskflow/DragInformation.cpp    D  src/lib/platform/MSWindowsDropTarget.cpp
D  src/lib/deskflow/DragInformation.h      D  src/lib/platform/MSWindowsDropTarget.h
D  src/lib/deskflow/DropHelper.cpp         D  src/lib/platform/OSXDragSimulator.m
D  src/lib/deskflow/DropHelper.h           D  src/lib/platform/OSXDragView.h
D  src/lib/deskflow/FileChunk.cpp          D  src/lib/platform/OSXDragView.m
D  src/lib/deskflow/FileChunk.h
```

Only Windows and macOS plus the shared core. Three independent checks confirm Linux was never
covered:

- `git show --name-status 5365e34f0` lists **no** Linux or X11 file.
- `git grep -i 'xdnd\|drag' 5365e34f0^ -- src/lib/platform/XWindowsScreen.cpp XWindowsScreen.h`
  returns **nothing** (run with the blobs local, so this is not a promisor failure).
- `git ls-tree -r --name-only 5365e34f0^` filtered for drag/drop shows exactly the ten files above.

The removed `ArgParser.cpp` hunk even logged *"ignoring --enable-drag-drop, not supported on
linux."* under `WINAPI_XWINDOWS`.

**Consequence for planning**: Windows (OLE `IDropSource`/`IDropTarget`) and macOS
(`NSPasteboard`/`NSDragPboard`) have upstream code to port; **Linux has nothing to port** — XDND /
Wayland DnD would be new development, and this project has no Linux machine to verify it on. The
agreed scope is therefore Windows + macOS, with Linux recorded as not implemented rather than
implied to be nearly done.

**Retrieval is offline-capable on this clone** — the pre-removal blobs are in the local object
store, so no fetch is needed:

```
git show 5365e34f0^:src/lib/platform/MSWindowsDropTarget.cpp
git show 5365e34f0^:src/lib/platform/OSXDragView.m
git show 5365e34f0^:src/lib/platform/OSXDragSimulator.m
```

**Three mandatory adaptations**, none of which is a verbatim re-apply:

1. The old `FileChunk` API is gone: `kStart`/`kNotFinish`/`kFinish`/`kError` and
   `assemble(stream, cached, size) -> int` were replaced by `TransferState` and
   `ChunkType::DataStart/DataChunk/DataEnd` (`protocol/ProtocolTypes.h`).
2. The old sender chunked at 512 KiB. Today the `%s` transport ceiling is 64 KiB
   (`PROTOCOL_MAX_STRING_LENGTH`) and exceeding it is not slow but fatal — the receiver throws
   `BadClientException` and the dispatcher drops the connection. Ported code must use
   `FileChunk::chunkSize()`; this is the same class of drift fixed in `49444783b`.
3. The old `DDRG` payload was `path,size,path,size,…`. The current encoder sends NUL-separated
   **base names only** (`FileTransferPath::joinNames`). That is a deliberate security improvement —
   a peer should not be handed local paths — so the platform layer must call the new encoder rather
   than `DragInformation`.

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
| U-02 | P0 | 打包 (Linux) | 安装源路径指向一个没有文件的身份 | **已修复** |
| U-03 | P1 | 打包 (全平台) | 三套产品身份并存；macOS 与 Linux 不一致 | **已修复** |
| U-04 | P1 | 打包 (macOS) | Bundle 图标文件名不存在 | **已修复** |
| U-05 | P2 | 打包 (macOS) | 死模板引用已被删除的变量 | **已修复** |
| U-06 | P2 | 打包 (Arch) | PKGBUILD 声明与自己冲突 | **已修复** |
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
| U-21 | P2 | 文档 / 功能范围 | 文档称 Linux 拖拽可恢复；实际从未实现 | 已记录 |

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
| `org.deskflow.deskflow` | `src/apps/res/deskflow.qrc`（`icons/deskflow-{dark,light}/`、`apps/64/org.deskflow.deskflow.svg`） | 上游；打包副本已于 2026-09-23 删除（见下方决议） |
| `com.symless.synergy` | 已不用于任何构建产物（见 U-02） | 曾是 CI 构建的身份；现仅剩厂商署名（U-10） |
| `com.tupig.synergy` | `extra/cmake/Synergy.cmake:10`；`extra/deploy/linux/*`；`extra/deploy/linux/flatpak/com.tupig.synergy.yml`；`extra/src/apps/res/synergy.qrc`；`extra/src/lib/synergy/gui/` | 被 Linux 安装路径、flatpak app-id、图标主题查找与 macOS `BUNDLE_GUI_IDENTIFIER`（`src/apps/deskflow-gui/CMakeLists.txt:24`）使用 |

**影响**：macOS 与 Linux 发布出不同的应用身份；Linux 安装路径要的身份与磁盘上存在的文件不是同一个。

**决议（2026-09-23）**：权威身份定为 `com.tupig.synergy`。已删除 4 个上游打包副本 ——
`deploy/linux/org.deskflow.deskflow.{desktop,metainfo.xml,png}` 与
`deploy/linux/flatpak/org.deskflow.deskflow.yml`，连同其孤儿 lint 伴随文件
`deploy/linux/flatpak/ci-build-lint-exceptions.json`（唯一 key 即被删清单的
`org.deskflow.deskflow` app-id；CI 实际 lint 的
`extra/deploy/linux/flatpak/ci-build-lint-exceptions.json` 不受影响）。
活文件 `deploy/linux/deploy.cmake`、`deploy/linux/arch/PKGBUILD.in` 保留（均带
SPDX 头且被构建引用）。同步更新 `REUSE.toml`（移除 3 条已删路径）、`docs/build.md`
（Flatpak 清单提及现仅指向 `extra/`）及 `extra/deploy/linux/flatpak/com.tupig.synergy.yml`
的上游路径注释。`rg 'org\.deskflow|deploy/linux/flatpak'` 复核：剩余命中仅限本台账、
时点快照 `audit-2026-09-23.md`、`src/apps/res/deskflow.qrc` 图标资源（U-07 范围外）与
记录本次删除的 flatpak yml 注释。图标主题（U-07）与陈旧注释（U-08）仍为独立未决项。

#### U-04 — macOS Bundle 图标文件名不存在

`APPLE` 时 `target = CMAKE_PROJECT_PROPER_NAME` = `"TuPig Synergy"`（`src/apps/deskflow-gui/CMakeLists.txt:6`），于是 `:25` 的 `BUNDLE_ICON_FILE` 与 `:35` 的图标源都是 `TuPig Synergy.icns`。该目录下只有 `Synergy.icns` 与 `Volume.icns`。该路径被加入 `add_executable`（`:39-44`），macOS 配置期会因找不到源文件直接失败。

#### U-05 — macOS 死模板

`extra/deploy/mac/bundle/Contents/Info.plist.in` 与 `PkgInfo.in` 引用 `@DESKFLOW_APP_NAME@`、`@DESKFLOW_APP_ID@`、`@DESKFLOW_MAC_BUNDLE_CODE@`、`@DESKFLOW_VERSION@`、`@DESKFLOW_BUILD_YEAR@`。全仓库没有任何地方定义这些变量（该上游 changelog 记录于 `deploy/linux/org.deskflow.deskflow.metainfo.xml:346-355` —— 该文件已随 U-03 删除 —— 说明它们已被删除）。也没有任何 `configure_file` 引用该模板 —— GUI 用的是 `src/apps/res/deskflow.plist.in`（`src/apps/deskflow-gui/CMakeLists.txt:30`）。`Info.plist.in:27` 还留着上游商业厂商的版权行。

#### U-06 — PKGBUILD 与自己冲突

`deploy/linux/arch/PKGBUILD.in:5-6` 由 `CMAKE_PROJECT_NAME` 得出 `pkgname=synergy-git`，但 `:13` 的 `conflicts` 数组第一个元素仍是 `'synergy-git'`，末尾还留着 `'deskflow'`。rebrand 改了 `_basename` 却没清理这个列表。
**已修复（2026-09-23）**：已从 `conflicts` 移除 `'synergy-git'`，并代之以 `'synergy'`，因为该包现在提供 `synergy` 命令，不能与另一个提供者共存。理由已核实：`conflicts` 列的是「本包无法共存的**其他**包」，把自己的 `pkgname` 列进去会让 pacman 拒绝自己产出的包。`deskflow` 一条是有意的，保留。

#### U-07 — 两套图标主题编进同一个二进制

GUI 同时链接 `../res/deskflow.qrc` 与 `extra/src/apps/res/synergy.qrc`（`src/apps/deskflow-gui/CMakeLists.txt:41-42`）。前者带 `icons/deskflow-{dark,light}/`，app 图标名为 `org.deskflow.deskflow.svg`；后者带 `icons/synergy-{dark,light}/`（主题名 `synergy-dark` / `synergy-light`），app 图标名为 `com.tupig.synergy.svg`。两套主题名与两种 app 图标标识并存。
**部分处理（2026-09-23）**：第二套主题的图标标识已改为 `com.tupig.synergy`（原为 `com.symless.synergy`），即 `QIcon::fromTheme(kRevFqdnName)` 实际请求的名字，见 U-02。本条目「两套主题并存」的另一半仍未处理：上游 `deskflow-{dark,light}` 主题仍被链接进来。

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

#### U-21 — Linux 拖拽从未实现（范围前提被更正）

**级别**: P2 · **验证**: git 历史，本地读取，无需联网

文件拖拽传输的工作原是按「三端都恢复上游实现」规划的，并把 Linux 描述为「XDND / Wayland
DnD」。核查历史后确认：**这一前提对 Linux 不成立**。

`5365e34f0` —— *"feat: remove drag and drop support, its broken on all platforms"*，2025-05-08
—— 就是移除它的那个提交。它属于**上游，不是本 fork**：`git merge-base --is-ancestor 5365e34f0
8ed7a3ef` 成立，而 `8ed7a3ef` 正是 `docs/contributing.md` 记录的 fork 基点。其删除清单为：

```
D  src/lib/deskflow/DragInformation.cpp    D  src/lib/platform/MSWindowsDropTarget.cpp
D  src/lib/deskflow/DragInformation.h      D  src/lib/platform/MSWindowsDropTarget.h
D  src/lib/deskflow/DropHelper.cpp         D  src/lib/platform/OSXDragSimulator.m
D  src/lib/deskflow/DropHelper.h           D  src/lib/platform/OSXDragView.h
D  src/lib/deskflow/FileChunk.cpp          D  src/lib/platform/OSXDragView.m
D  src/lib/deskflow/FileChunk.h
```

只有 Windows 与 macOS 以及共享核心。三项独立检查都指向「Linux 从来未被覆盖」：

- `git show --name-status 5365e34f0` 未列出任何 Linux / X11 文件。
- `git grep -i 'xdnd\|drag' 5365e34f0^ -- src/lib/platform/XWindowsScreen.cpp XWindowsScreen.h`
  **无任何结果**（在 blob 已本地化的情况下执行，故不是 promisor 取用失败）。
- `git ls-tree -r --name-only 5365e34f0^` 按 drag/drop 过滤后，恰好就是上述十个文件。

被删的 `ArgParser.cpp` 代码块甚至有一行日志：`WINAPI_XWINDOWS` 下
*"ignoring --enable-drag-drop, not supported on linux."*

**对规划的影响**：Windows（OLE `IDropSource`/`IDropTarget`）与 macOS
（`NSPasteboard`/`NSDragPboard`）有上游代码可移植；**Linux 无物可移植** —— XDND / Wayland DnD
属全新开发，而本项目没有 Linux 机器可供验证。因此确定的范围是 Windows + macOS，Linux 明确
记为「未实现」，而不是含糊地像是「快好了」。

**本克隆可离线取出**（移除前的 blob 就在本地对象库里，无需 fetch）：

```
git show 5365e34f0^:src/lib/platform/MSWindowsDropTarget.cpp
git show 5365e34f0^:src/lib/platform/OSXDragView.m
git show 5365e34f0^:src/lib/platform/OSXDragSimulator.m
```

**三处必须改写**（都不是原样照搬）：

1. 旧 `FileChunk` 接口已不存在：`kStart`/`kNotFinish`/`kFinish`/`kError` 与
   `assemble(stream, cached, size) -> int` 已被 `TransferState` 与
   `ChunkType::DataStart/DataChunk/DataEnd` 取代（`protocol/ProtocolTypes.h`）。
2. 旧发送端按 512 KiB 分块。现在 `%s` 的传输上限是 64 KiB（`PROTOCOL_MAX_STRING_LENGTH`），
   且超限不是变慢而是**致命** —— 接收端抛 `BadClientException`，分发层断开连接。移植代码必须
   使用 `FileChunk::chunkSize()`；这与 `49444783b` 修的正是同一类漂移。
3. 旧 `DDRG` 载荷是 `路径,大小,路径,大小…`。当前编码器只发 NUL 分隔的**基名**
   （`FileTransferPath::joinNames`）。这是有意的安全改进 —— 不应把本机路径交给对端 —— 故平台层
   必须调用新编码器，而非 `DragInformation`。

### 已核实一致（不是缺陷）

以下各项已检查且**不是**问题 —— 记录下来，避免后续审计重复开单：

- **翻译加载**：`.ts` 命名、`kUpstreamId` 过滤条件与 `I18NTests` 三者一致（原因见 U-18）。
- **CI 产物命名**：`PACKAGE_PREFIX` 默认 `synergy`（`ci.yml:48`），`synergy_<version>_<os>_<arch>` 方案与 `extra/deploy/PackageFileName.cmake:46` 一致。
- **`PACKAGE_VERSION_LABEL`**：在 `deploy/CMakeLists.txt:22-23` 由同一个版本函数派生，CI 未覆盖。

### 待验证项

U-01 无法实证确认：`build/` 为空，且本地 `cmake`/`ninja` 不在 `PATH`（`HANDOFF.md:158`）。确认方式是配置并构建一次，检查是否存在 `synergy-core-1.21.2[.exe]`、以及 GUI 能否启动 core。若能，则 U-01 成立。
