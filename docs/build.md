# Build Guide / 编译指南

> **Language / 语言**: [English](#english) | [中文](#中文)

---

## English

### Prerequisites

| Component | Minimum Version | Notes |
|-----------|----------------|-------|
| **CMake** | 3.25+ | Modern CMake required |
| **Qt** | 6.7.0+ | Core, Widgets, Network, DBus (Linux) |
| **OpenSSL** | 3.0+ | TLS/crypto support |
| **libportal** | 0.8.0+ | Linux/BSD only (Wayland portal) |
| **libei** | 1.0+ | Linux/BSD only (input emulation) |

### Default Build Options

The following components are enabled by default:

- ✅ TuPig Synergy GUI (`synergy_<X.Y.Z>.exe` on Windows, `synergy` on Linux, `TuPig Synergy.app` on macOS)
- ✅ TuPig Synergy Core (`synergy-core`, unversioned)
- ✅ Daemon for Windows UAC handling (`synergy-daemon`, Windows only, unversioned)
- ✅ Build-time Unit Tests (Qt Test + CTest)

### CMake Configuration Options

| Option | Description | Default |
|--------|-------------|---------|
| `BUILD_GUI` | Build the Qt GUI | `ON` |
| `BUILD_TESTS` | Build unit tests | `ON` |
| `BUILD_INSTALLER` | Build platform installers | `ON` |
| `BUILD_X11_SUPPORT` | Build X11 backend (Linux/BSD) | `ON` |
| `BUILD_OSX_BUNDLE` | Build macOS `.app` bundle | `ON` |
| `SKIP_BUILD_TESTS` | Skip tests during build | `ON` |
| `ENABLE_COVERAGE` | Enable code coverage reports | `OFF` |
| `CLEAN_TRS` | Remove obsolete translation strings | `OFF` |
| `SYNERGY_CORE_FLAVOR` | Build as "TuPig Synergy Core"; seeds headless defaults (GUI/tests/installer off) | `OFF` |
| `APPLE_CODESIGN_DEV` | Apple Developer code-sign identity (cache variable, not an `option()`) | unset |

**Build:**

```bash
# Windows: scripts\build.bat release
# Linux or macOS (Apple Silicon):
./scripts/build.sh release
```

A bare `cmake -S` without the vcpkg toolchain from `CMakePresets.json` will not find Qt.

---

### 🪟 Windows (MSVC + vcpkg)

Dependencies are resolved by vcpkg in manifest mode. The repository-local vcpkg is bootstrapped for
you, so there is no `VCPKG_ROOT` to configure.

```bat
REM One-time: install the host toolchain (run as Administrator)
setup.bat

REM Build (bootstraps vcpkg, configures, compiles)
scripts\build.bat release
```

`scripts\build.bat` locates Visual Studio through `vswhere`, activates the MSVC x64 environment and
then runs `cmake --preset windows-msvc-release` / `cmake --build --preset windows-msvc-release`.
That preset sets `SYNERGY_VERSION_RELEASE=ON`, so the version string is `X.Y.Z` from
`cmake/Version.cmake` (currently 1.21.2), not `X.Y.Z-dev+<sha>`.

Output:

| File | Role |
|------|------|
| `build\bin\Release\synergy_<X.Y.Z>.exe` | GUI. `X.Y.Z` is `SYNERGY_VERSION_MAJOR.MINOR.PATCH` plus an optional stage. |
| `build\bin\Release\synergy-core.exe` | Core. The GUI looks this name up; do not version it. |
| `build\bin\Release\synergy-daemon.exe` | Windows service helper. Also unversioned. The portable 7Z omits it. |

#### Windows code signing

Release CI signs the inner executables and the MSI only when `WINDOWS_SSL_USERNAME`,
`WINDOWS_SSL_PASSWORD`, `WINDOWS_SSL_CREDENTIAL_ID`, and `WINDOWS_SSL_TOTP_SECRET` are
all set. If any one is missing, the job warns and uploads unsigned packages. There is
no local signing step in `scripts\build.bat`.

---

### 🍎 macOS (Apple Silicon locally, Intel in CI)

Host tools are Xcode Command Line Tools, CMake 3.25+, and Ninja. Qt and OpenSSL come from
the repository vcpkg. Do not point CMake at a Homebrew Qt.

```bash
xcode-select --install
brew install cmake ninja
./scripts/build.sh release
```

`./scripts/build.sh` selects preset `macos-release` (Ninja, triplet `arm64-osx`). That
preset is Apple Silicon. Intel x86_64 is the Actions job `macos-x64`
(`CMAKE_OSX_ARCHITECTURES=x86_64` on `macos-15-intel`), which produces a separate DMG
(`mac_x64`), not a universal binary. The deployment target is macOS 12 and is set only
in `cmake/Synergy.cmake`. Do not pass `-DCMAKE_OSX_DEPLOYMENT_TARGET`.

Output: `build/bin/TuPig Synergy.app` and `build/bin/synergy-core`.

#### macOS code signing (development)

```bash
security find-identity -v -p codesigning login.keychain-db
cmake --preset macos-release -DAPPLE_CODESIGN_DEV="Apple Development: Name (TEAMID)"
codesign -d -r- "build/bin/TuPig Synergy.app"
```

Local development uses an Apple Development certificate. Distribution signing uses the
CI secret `APPLE_CODESIGN_ID` when it is set.

---

### 🐧 Linux (Ubuntu / Debian / Fedora / Arch)

Install a C++ compiler, CMake 3.25+, Ninja, and the system libraries vcpkg does not
build (X11 and the Wayland portal stack). Do not install a distro Qt or OpenSSL for
this build, and do not set `CMAKE_PREFIX_PATH` to one: `./scripts/build.sh` configures
with the vcpkg toolchain, which builds Qt and OpenSSL from `vcpkg.json`.

#### Ubuntu / Debian

```bash
sudo apt update && sudo apt install -y \
  cmake ninja-build g++ pkg-config \
  libx11-dev libxi-dev libxtst-dev \
  libxinerama-dev libxrandr-dev \
  libxkbcommon-dev libglib2.0-dev \
  libportal-dev libei-dev
```

#### Fedora / RHEL

```bash
sudo dnf install -y \
  cmake ninja-build gcc-c++ pkgconf-pkg-config \
  libX11-devel libXi-devel libXtst-devel \
  libXinerama-devel libXrandr-devel \
  libxkbcommon-devel glib2-devel \
  libportal-devel libei-devel
```

#### Arch Linux

```bash
sudo pacman -S \
  cmake ninja gcc pkgconf \
  libx11 libxi libxtst libxinerama \
  libxrandr libxkbcommon glib2 \
  libportal libei
```

#### Build

```bash
./scripts/build.sh release
```

Output: `build/bin/synergy` (GUI, unversioned, because the `.desktop` file launches that
name) and `build/bin/synergy-core`. If vcpkg stops on a missing system library, install
the package it names and run the script again. Downloads already in
`vendor/vcpkg/downloads/` are reused.

---

### 📦 Packaging & Installation

| Target | Command | Output |
|--------|---------|--------|
| **Install locally** | `cmake --install build --prefix /usr/local` | System-wide |
| **Staged install** | `DESTDIR=/tmp/pkg cmake --install build` | Package staging |
| **Binary package** | `cmake --build build --target package` | Platform native |
| **Source package** | `cmake --build build --target package_source` | Source tarball |

**Supported Package Formats:**

| Platform | Formats |
|----------|---------|
| **All** | TGZ, TBZ2, TXZ, TZST |
| **Linux** | DEB or RPM (auto-selected from `/etc/os-release`); Flatpak via CI `build-flatpak`; Arch via `deploy/linux/arch/PKGBUILD.in` |
| **macOS** | DMG (DragNDrop) |
| **Windows** | 7Z (portable), MSI (WiX, see note) |

> **Note**: AppImage is not implemented — no `appimage` reference exists in
> `deploy/` or `.github/`. A Flatpak manifest exists under
> `deploy/linux/flatpak/` and is consumed by the CI `build-flatpak`
> job; the local `package` target does not produce a Flatpak bundle.
> MSI generation uses the WiX toolset installed on the machine (CI pins
> 5.0.2; CPack requests the WiX v4 schema). See `docs/HANDOFF.md` for
> verification status.

---

### 🔧 Advanced Configuration

#### Sanitizer builds

```bash
cmake --preset linux-asan-build && cmake --build --preset linux-asan-build
cmake --preset linux-tsan-build && cmake --build --preset linux-tsan-build
cmake --preset linux-coverage-build && cmake --build --preset linux-coverage-build
```

Windows ASan is preset `windows-msvc-asan`. It uses the dynamic-CRT triplet `x64-windows` because MSVC ASan requires `/MD`. There is no MemorySanitizer preset.

#### ccache Acceleration

```bash
cmake -S . -B build \
  -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
```

---

### 🧪 Testing

Tests are registered under `src/unittests`, so `ctest --test-dir build` reports that no tests were found. `BUILD_TESTS` defaults to `ON` (the test binaries are compiled). `SKIP_BUILD_TESTS` defaults to `ON`, which only skips running them at the end of the build.

```bash
# Windows (Visual Studio preset)
ctest --test-dir build/src/unittests -C Release --output-on-failure

# Linux / macOS (Ninja)
ctest --test-dir build/src/unittests --output-on-failure

ctest --test-dir build/src/unittests -C Release -R "ClipboardTests" --output-on-failure
```

Pass `-DSKIP_BUILD_TESTS=OFF` to run that ctest invocation automatically after the build. Coverage uses preset `linux-coverage-build`.

---

### ❓ Troubleshooting Build Issues

| Issue | Solution |
|-------|----------|
| `Qt6 not found` | Re-run `scripts/build.bat` or `scripts/build.sh`. Do not set `Qt6_DIR` or `CMAKE_PREFIX_PATH` to a distro Qt. |
| `OpenSSL not found` | Same. OpenSSL is a vcpkg manifest dependency, not a system package you point CMake at. |
| `X11 libs missing` | Install `libx11-dev`, `libxi-dev`, `libxtst-dev`, and the rest of the Linux package list above. |
| `Wayland protocols` | Install `libwayland-dev` and `wayland-protocols` when the log asks for them. |
| vcpkg download stalls | Run the build script again. Keep `vendor/vcpkg/downloads/`; partial files are rejected by their SHA-512. |

---

## 中文

### 编译环境要求

| 组件 | 最低版本 | 说明 |
|------|----------|------|
| **CMake** | 3.25+ | 需要现代 CMake 特性 |
| **Qt** | 6.7.0+ | Core, Widgets, Network, DBus (Linux) |
| **OpenSSL** | 3.0+ | TLS/加密支持 |
| **libportal** | 0.8.0+ | 仅 Linux/BSD (Wayland Portal) |
| **libei** | 1.0+ | 仅 Linux/BSD (输入仿真) |

### 默认启用组件

- ✅ TuPig Synergy GUI（Windows 为 `synergy_<X.Y.Z>.exe`，Linux 为 `synergy`，macOS 为 `TuPig Synergy.app`）
- ✅ TuPig Synergy 核心（`synergy-core`，不带版本号）
- ✅ Windows UAC 守护进程（`synergy-daemon`，仅 Windows，不带版本号）
- ✅ 编译时单元测试 (Qt Test + CTest)

### CMake 配置选项

| 选项 | 说明 | 默认值 |
|------|------|--------|
| `BUILD_GUI` | 编译 Qt GUI | `ON` |
| `BUILD_TESTS` | 编译单元测试 | `ON` |
| `BUILD_INSTALLER` | 编译平台安装包 | `ON` |
| `BUILD_X11_SUPPORT` | 编译 X11 后端 (Linux/BSD) | `ON` |
| `BUILD_OSX_BUNDLE` | 编译 macOS `.app` 包 | `ON` |
| `SKIP_BUILD_TESTS` | 跳过编译时测试 | `ON` |
| `ENABLE_COVERAGE` | 启用代码覆盖率报告 | `OFF` |
| `CLEAN_TRS` | 清理翻译文件中过时字符串 | `OFF` |
| `SYNERGY_CORE_FLAVOR` | 以 “TuPig Synergy Core” 构建；同时将 GUI/测试/安装包默认置为关闭（无界面构建） | `OFF` |
| `APPLE_CODESIGN_DEV` | Apple 开发者代码签名身份（缓存变量，非 `option()`） | 未设置 |

**构建：**

```bash
# Windows：scripts\build.bat release
# Linux 或 macOS（Apple Silicon）：
./scripts/build.sh release
```

不带 `CMakePresets.json` 里 vcpkg 工具链的 `cmake -S` 找不到 Qt。

---

### 🪟 Windows (MSVC + vcpkg)

依赖由 vcpkg 清单模式解析，仓库内的 vcpkg 会自动引导，因此无需配置 `VCPKG_ROOT`。

```bat
REM 一次性：安装宿主工具链（以管理员身份运行）
setup.bat

REM 构建（自动引导 vcpkg、配置、编译）
scripts\build.bat release
```

`scripts\build.bat` 会通过 `vswhere` 定位 Visual Studio，激活 MSVC x64 环境，然后执行
`cmake --preset windows-msvc-release` / `cmake --build --preset windows-msvc-release`。
该预设打开 `SYNERGY_VERSION_RELEASE`，版本字符串是 `cmake/Version.cmake` 里的 `X.Y.Z`
（当前 1.21.2），不是 `X.Y.Z-dev+<sha>`。

产物：

| 文件 | 作用 |
|------|------|
| `build\bin\Release\synergy_<X.Y.Z>.exe` | GUI。`X.Y.Z` 来自 `SYNERGY_VERSION_MAJOR.MINOR.PATCH`，若有 stage 再追加。 |
| `build\bin\Release\synergy-core.exe` | 核心。GUI 按这个固定名字查找，不要加版本号。 |
| `build\bin\Release\synergy-daemon.exe` | Windows 服务辅助进程，同样不带版本号。便携 7Z 故意不含它。 |

#### Windows 代码签名

Release CI 只有在 `WINDOWS_SSL_USERNAME`、`WINDOWS_SSL_PASSWORD`、`WINDOWS_SSL_CREDENTIAL_ID`、`WINDOWS_SSL_TOTP_SECRET` 四个 secret 全部存在时才签名内部 exe 和 MSI。缺任何一个时，作业给出警告并上传未签名包。`scripts\build.bat` 本身不签名。

---

### 🍎 macOS（本地 Apple Silicon，Intel 由 CI 构建）

宿主工具是 Xcode Command Line Tools、CMake 3.25+ 和 Ninja。Qt 与 OpenSSL 来自仓库内 vcpkg，不要把 CMake 指到 Homebrew 的 Qt。

```bash
xcode-select --install
brew install cmake ninja
./scripts/build.sh release
```

`./scripts/build.sh` 选择预设 `macos-release`（Ninja，triplet `arm64-osx`），这是 Apple Silicon。Intel x86_64 是 Actions 作业 `macos-x64`（`macos-15-intel` 上 `CMAKE_OSX_ARCHITECTURES=x86_64`），单独产出 `mac_x64` 的 DMG，不是一个通用二进制。部署目标是 macOS 12，只写在 `cmake/Synergy.cmake`，不要传 `-DCMAKE_OSX_DEPLOYMENT_TARGET`。

产物：`build/bin/TuPig Synergy.app` 与 `build/bin/synergy-core`。

#### macOS 代码签名（开发用）

```bash
security find-identity -v -p codesigning login.keychain-db
cmake --preset macos-release -DAPPLE_CODESIGN_DEV="Apple Development: Name (TEAMID)"
codesign -d -r- "build/bin/TuPig Synergy.app"
```

本地开发用 Apple Development 证书。分发签名在设置了 CI secret `APPLE_CODESIGN_ID` 时由 CI 完成。

---

### 🐧 Linux (Ubuntu / Debian / Fedora / Arch)

安装 C++ 编译器、CMake 3.25+、Ninja，以及 vcpkg 不负责构建的系统库（X11 与 Wayland portal）。不要为这次构建安装发行版 Qt 或 OpenSSL，也不要把 `CMAKE_PREFIX_PATH` 指过去：`./scripts/build.sh` 使用 vcpkg 工具链，按 `vcpkg.json` 从源码构建 Qt 与 OpenSSL。

#### Ubuntu / Debian

```bash
sudo apt update && sudo apt install -y \
  cmake ninja-build g++ pkg-config \
  libx11-dev libxi-dev libxtst-dev \
  libxinerama-dev libxrandr-dev \
  libxkbcommon-dev libglib2.0-dev \
  libportal-dev libei-dev
```

#### Fedora / RHEL

```bash
sudo dnf install -y \
  cmake ninja-build gcc-c++ pkgconf-pkg-config \
  libX11-devel libXi-devel libXtst-devel \
  libXinerama-devel libXrandr-devel \
  libxkbcommon-devel glib2-devel \
  libportal-devel libei-devel
```

#### Arch Linux

```bash
sudo pacman -S \
  cmake ninja gcc pkgconf \
  libx11 libxi libxtst libxinerama \
  libxrandr libxkbcommon glib2 \
  libportal libei
```

#### 编译

```bash
./scripts/build.sh release
```

产物：`build/bin/synergy`（GUI，不带版本号，因为 `.desktop` 的 `Exec=` 按这个名字启动）和 `build/bin/synergy-core`。vcpkg 若因缺少系统库停下，安装它点名的包后再运行脚本。`vendor/vcpkg/downloads/` 里已有的下载会被复用。

---

### 📦 打包与安装

| 目标 | 命令 | 输出 |
|------|------|------|
| **本地安装** | `cmake --install build --prefix /usr/local` | 系统级 |
| **暂存安装** | `DESTDIR=/tmp/pkg cmake --install build` | 打包暂存 |
| **二进制包** | `cmake --build build --target package` | 平台原生包 |
| **源码包** | `cmake --build build --target package_source` | 源码压缩包 |

**支持的包格式：**

| 平台 | 格式 |
|------|------|
| **所有平台** | TGZ, TBZ2, TXZ, TZST |
| **Linux** | DEB 或 RPM（依 `/etc/os-release` 自动二选一）；Flatpak 经 CI `build-flatpak`；Arch 经 `deploy/linux/arch/PKGBUILD.in` |
| **macOS** | DMG (DragNDrop) |
| **Windows** | 7Z（便携）、MSI (WiX，见说明) |

> **说明**：AppImage **未实现** —— `deploy/`、`.github/` 中不存在任何 `appimage` 引用。Flatpak 清单位于 `deploy/linux/flatpak/`，由 CI 的 `build-flatpak` job 消费；本地 `package` 目标不产出 Flatpak 包。MSI 使用机器上已安装的 WiX（CI 固定 5.0.2；CPack 请求 WiX v4 schema）。验证状态见 `docs/HANDOFF.md`。

---

### 🔧 进阶配置

#### Sanitizer 构建

```bash
cmake --preset linux-asan-build && cmake --build --preset linux-asan-build
cmake --preset linux-tsan-build && cmake --build --preset linux-tsan-build
cmake --preset linux-coverage-build && cmake --build --preset linux-coverage-build
```

Windows ASan 使用预设 `windows-msvc-asan`。它改用动态 CRT triplet `x64-windows`，因为 MSVC ASan 需要 `/MD`。没有 MemorySanitizer 预设。

#### ccache 加速编译

```bash
cmake -S . -B build \
  -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
```

---

### 🧪 测试

测试注册在 `src/unittests`，所以 `ctest --test-dir build` 会报找不到测试。`BUILD_TESTS` 默认 `ON`（会编译测试程序）。`SKIP_BUILD_TESTS` 默认 `ON`，只是不在构建结束时自动跑它们。

```bash
# Windows（Visual Studio 预设）
ctest --test-dir build/src/unittests -C Release --output-on-failure

# Linux / macOS（Ninja）
ctest --test-dir build/src/unittests --output-on-failure

ctest --test-dir build/src/unittests -C Release -R "ClipboardTests" --output-on-failure
```

加上 `-DSKIP_BUILD_TESTS=OFF` 会在构建结束后自动跑上述 ctest。覆盖率使用预设 `linux-coverage-build`。

---

### ❓ 常见编译问题排查

| 问题 | 解决方案 |
|------|----------|
| `Qt6 not found` | 重新运行 `scripts\build.bat` 或 `scripts/build.sh`。不要把 `Qt6_DIR` 或 `CMAKE_PREFIX_PATH` 指到发行版 Qt。 |
| `OpenSSL not found` | 同上。OpenSSL 是 vcpkg 清单依赖，不是要另外指给 CMake 的系统包。 |
| `X11 库缺失` | 安装 `libx11-dev`、`libxi-dev`、`libxtst-dev` 以及上面 Linux 软件包列表里的其余项。 |
| `Wayland 协议缺失` | 日志点名时再安装 `libwayland-dev` 与 `wayland-protocols`。 |
| vcpkg 下载中断 | 重新运行构建脚本。保留 `vendor/vcpkg/downloads/`；不完整文件会因 SHA-512 校验失败而被拒绝。 |

---

## Build Presets & Scripts / 构建预设与脚本

### One-command build / 一键构建

The repository is self-contained. Third-party dependencies (Qt, OpenSSL and their transitive
dependencies) are managed by vcpkg in manifest mode, and the repository-local vcpkg is bootstrapped
automatically — no `VCPKG_ROOT`, no global install, no manual environment variables.

本仓库自包含。第三方依赖（Qt、OpenSSL 及其传递依赖）由 vcpkg 清单模式管理，仓库内的 vcpkg 自动引导
—— 无需 `VCPKG_ROOT`、无需全局安装、无需手工设置环境变量。

| Platform / 平台 | Command / 命令 |
|---|---|
| Windows | `scripts\build.bat release` |
| Linux / macOS | `./scripts/build.sh release` |

The three `*-release` presets set `SYNERGY_VERSION_RELEASE=ON`, so the version string is `X.Y.Z` from `cmake/Version.cmake`. Configuring without that preset (or without `-DSYNERGY_VERSION_RELEASE=ON`) produces `X.Y.Z-dev+<sha>`.

这三个 `*-release` 预设都会设置 `SYNERGY_VERSION_RELEASE=ON`，版本字符串是 `cmake/Version.cmake` 里的 `X.Y.Z`。不用该预设、也不传 `-DSYNERGY_VERSION_RELEASE=ON` 时，版本是 `X.Y.Z-dev+<sha>`。

Both scripts accept only `release`: every vcpkg overlay triplet in this repository sets
`VCPKG_BUILD_TYPE release`, so a Debug configuration cannot link the dependencies. For
instrumented builds use the diagnostics presets (`linux-asan-build` / `linux-tsan-build` /
`linux-coverage-build`).

Both scripts call `scripts/bootstrap-vcpkg.{bat,sh}`, which clones the repository-local vcpkg into
`vendor/vcpkg`, checks out the baseline pinned by `builtin-baseline` in `vcpkg.json`, and bootstraps
the vcpkg tool. The baseline is the single source of truth and is read from `vcpkg.json`, not
duplicated in the scripts.

两个脚本都会调用 `scripts/bootstrap-vcpkg.{bat,sh}`：克隆仓库内 vcpkg 到 `vendor/vcpkg`，检出
`vcpkg.json` 中 `builtin-baseline` 固定的版本，并引导 vcpkg 工具。baseline 是唯一真源，由脚本从
`vcpkg.json` 读取，不在脚本里复制一份。

两个脚本都只接受 `release`：本仓库所有 vcpkg overlay triplet 均设置
`VCPKG_BUILD_TYPE release`，Debug 配置无法链接依赖。需要插桩构建请使用诊断预设
（`linux-asan-build` / `linux-tsan-build` / `linux-coverage-build`）。

> **Windows prerequisite / Windows 前置条件**: a C++ toolchain (Visual Studio 2022 Build Tools with
> the "Desktop development with C++" workload), CMake 3.25+ and Git. Run `setup.bat` once to install
> them. `scripts\build.bat` locates Visual Studio through `vswhere` and activates the MSVC environment
> itself — do not hardcode install paths or pre-set `VCPKG_ROOT`.
>
> **Windows 前置条件**：C++ 工具链（含 "Desktop development with C++" 工作负载的 Visual Studio 2022
> Build Tools）、CMake 3.25+ 与 Git。首次运行 `setup.bat` 安装。`scripts\build.bat` 会通过 `vswhere`
> 自行定位 Visual Studio 并激活 MSVC 环境 —— 不要硬编码安装路径，也不要预设 `VCPKG_ROOT`。

### Presets in `CMakePresets.json` / 预设清单

| Preset | Purpose / 用途 |
|---|---|
| `windows-msvc`, `windows-msvc-release`, `windows-msvc-debug` | Windows x64, Visual Studio 2022, static triplet |
| `linux`, `linux-release` | Linux x64, Ninja, static triplet |
| `macos`, `macos-release` | macOS arm64, Ninja, static triplet |
| `linux-asan-build`, `linux-tsan-build`, `linux-coverage-build` | Linux diagnostics (`RelWithDebInfo` + `-O1`) / Linux 诊断构建 |

```bash
# List presets / 列出预设
cmake --list-presets

# Configure and build directly (the scripts do both for you)
# 直接配置与构建（脚本已代为完成这两步）
cmake --preset windows-msvc-release
cmake --build --preset windows-msvc-release
```

> There is no `CMakeUserPresets.json` in this repository — it is git-ignored and was removed.
>
> 本仓库不包含 `CMakeUserPresets.json` —— 该文件已被忽略并移除。
