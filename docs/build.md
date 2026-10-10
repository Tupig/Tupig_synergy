# Build Guide / 编译指南

> **Language / 语言**: [English](#english) | [中文](#中文)

---

## English

### Prerequisites

| Component | Minimum Version | Notes |
|-----------|----------------|-------|
| **CMake** | 3.25+ | Presets use schema version 6, which needs 3.25 |
| **Qt** | 6.7.0+ | Core, Widgets, Network, DBus (Linux) |
| **OpenSSL** | 3.0+ | TLS/crypto support |
| **libportal** | 0.8.0+ | Linux/BSD only (Wayland portal) |
| **libei** | 1.0+ | Linux/BSD only (input emulation) |

### Architectures

Build artifacts are **64-bit only**. `vcpkg.json` sets `"supports": "x64 | arm64"`. There is no 32-bit (x86) target.

| Platform | Architectures |
|----------|----------------|
| Windows | x64 (`vcvarsall.bat x64`, triplet `x64-windows-static`) and arm64 in CI |
| Linux | x86_64 and aarch64 |
| macOS | arm64 locally (`arm64-osx`); Intel x86_64 in CI |

x86_64 and x64 name the same 64-bit Intel/AMD architecture. They are not 32-bit.

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
| `CMAKE_COMPILE_WARNING_AS_ERROR` | Fail the build on a compiler warning | `ON` |

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
then configures with the preset for that Visual Studio. Visual Studio 2022 uses
`windows-msvc-release`. Visual Studio 2026 uses `windows-msvc-2026-release`, because vcpkg builds
the dependencies with the same compiler and a VS 2022 link cannot resolve the newer static STL.
That preset sets `SYNERGY_VERSION_RELEASE=ON`, so the version string is `X.Y.Z` from
`cmake/Version.cmake` (currently 1.21.2), not `X.Y.Z-dev+<sha>`.

Output:

| File | Role |
|------|------|
| `build\bin\Release\synergy_<X.Y.Z>.exe` | GUI. `X.Y.Z` is `SYNERGY_VERSION_MAJOR.MINOR.PATCH` plus an optional stage. |
| `build\bin\Release\synergy-core.exe` | Core. The GUI looks this name up; do not version it. |
| `build\bin\Release\synergy-daemon.exe` | Windows service helper. Also unversioned. The portable 7Z omits it. |

Check the binaries. `X.Y.Z` below is the current `Version.cmake` value, 1.21.2:

```bat
build\bin\Release\synergy_1.21.2.exe --version
build\bin\Release\synergy-core.exe --version
build\bin\Release\synergy-daemon.exe --version
build\bin\Release\synergy-core.exe server
```

`synergy-core --version` prints the product version and `protocol v1.8`. The GUI prints `TuPig Synergy: 1.21.2`. The daemon prints `1.21.2` only. Flags are listed in [configuration.md](configuration.md#command-line).

#### Windows code signing

Release CI signs `synergy_<X.Y.Z>.exe`, `synergy-core.exe`, `synergy-daemon.exe`, and the
MSI through [SignPath Foundation](https://signpath.org/). The private key stays on
SignPath's HSM. This repository never stores a certificate, a password, or a token.
The portable 7Z is built from those signed executables; the archive itself is not an
Authenticode file. A local Release build stays unsigned, because the Foundation key
cannot be used on a developer machine.

Signing runs only when all three of these GitHub Actions values are set (Settings,
Secrets and variables, Actions):

| Name | Kind | Meaning |
|---|---|---|
| `SIGNPATH_API_TOKEN` | secret | API token for a user who may submit the signing policy. The only secret. |
| `SIGNPATH_ORGANIZATION_ID` | variable | Organization id assigned after the Foundation approves the project. |
| `SIGNPATH_PROJECT_SLUG` | variable | Project slug for this repository. |

`SIGNPATH_POLICY_SLUG` is an optional variable. When it is empty, the workflow uses
`release-signing`.

Apply at <https://signpath.org/> for the public repository
`https://github.com/Tupig/Tupig_synergy`. After approval:

1. Install the SignPath GitHub App on this repository and link the predefined
   Trusted Build System GitHub.com to the project.
2. Create signing policy `release-signing` for the `main` branch and release tags.
3. Add two artifact configurations, using these slugs and the XML in this repository:
   `windows-executables` from `.signpath/windows-executables.xml`, and `windows-msi`
   from `.signpath/windows-msi.xml`.
4. Store the API token as the secret above, and store the organization id and project
   slug as the two variables. Do not commit any of those values.

If any required value is missing, the job logs that signing was skipped and uploads unsigned packages. If they
are set and `signtool verify` fails after SignPath returns the files, the job fails.

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
> 5.0.2; CPack requests the WiX v4 schema).

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

### Source encoding

CI job `lint-encoding` runs `python3 scripts/ci/check-encoding.py` after the version plan and before compile or packaging. The same command works from a checkout. It walks the configured roots, skips excluded directory names, and reads each source file in chunks.

The default whitelist is `utf-8`, which means UTF-8 without a BOM. An ASCII file is reported as `ascii` and still passes, because ASCII is UTF-8. To allow a BOM, add `utf-8-bom` (aliases `utf-8-sig` and `utf-8-with-bom`). To allow only ASCII, set the whitelist to `ascii` alone.

`on_violation` is `fail` by default: each bad file is printed and the process exits 1, so the build jobs do not start. Set it to `warn`, or pass `--warn`, to print the same lines and exit 0. `--fail` forces the non-zero exit even when the file says `warn`.

| Key | Default | Meaning |
|-----|---------|---------|
| `roots` | `src`, `cmake`, `scripts`, `deploy`, `.github`, `translations` | Directories scanned from the repository root |
| `extensions` | C/C++, Qt, CMake, scripts, YAML, JSON, XML | Suffixes that count as source |
| `names` | `CMakeLists.txt` | Extra file names, regardless of suffix |
| `allowed` | `utf-8` | Whitelist. Also accepts `ascii` and `utf-8-bom` |
| `exclude_dirs` | `vendor`, `build`, `.git`, `dist`, `node_modules`, `downloads`, `vcpkg_installed`, `generated`, `autogen`, `CMakeFiles` | Directory names skipped at any depth |
| `on_violation` | `fail` | `fail` exits 1. `warn` prints and exits 0 |

A failing line looks like this. The path and the detected encoding are on the same line:

```text
encoding FAIL src/foo.cpp detected=utf-8-bom allowed=utf-8
encoding FAIL count=1 checked=669 mode=fail
```

A clean run is one line: `encoding OK checked=669 allowed=utf-8 mode=fail`.

---

### ❓ Troubleshooting Build Issues

| Issue | Solution |
|-------|----------|
| `Qt6 not found` | Re-run `scripts/build.bat` or `scripts/build.sh`. Do not set `Qt6_DIR` or `CMAKE_PREFIX_PATH` to a distro Qt. |
| `OpenSSL not found` | Same. OpenSSL is a vcpkg manifest dependency, not a system package you point CMake at. |
| `X11 libs missing` | Install `libx11-dev`, `libxi-dev`, `libxtst-dev`, and the rest of the Linux package list above. |
| `Wayland protocols` | Install `libwayland-dev` and `wayland-protocols` when the log asks for them. |
| vcpkg download stalls | Run the build script again. Keep `vendor/vcpkg/downloads/`; partial files are rejected by their SHA-512. |
| `encoding FAIL ... detected=` | The file is not in the whitelist. Save it as UTF-8 without a BOM, or add that encoding to `allowed`. `ascii` already passes under `utf-8`. |

---

## 中文

### 编译环境要求

| 组件 | 最低版本 | 说明 |
|------|----------|------|
| **CMake** | 3.25+ | 预设使用 schema 第 6 版，因此需要 3.25 |
| **Qt** | 6.7.0+ | Core, Widgets, Network, DBus (Linux) |
| **OpenSSL** | 3.0+ | TLS/加密支持 |
| **libportal** | 0.8.0+ | 仅 Linux/BSD (Wayland Portal) |
| **libei** | 1.0+ | 仅 Linux/BSD (输入仿真) |

### 架构

构建产物**只有 64 位**。`vcpkg.json` 的 `"supports"` 是 `x64 | arm64`。没有 32 位（x86）目标。

| 平台 | 架构 |
|------|------|
| Windows | x64（`vcvarsall.bat x64`，triplet `x64-windows-static`）；CI 另有 arm64 |
| Linux | x86_64 与 aarch64 |
| macOS | 本地 arm64（`arm64-osx`）；CI 另有 Intel x86_64 |

x86_64 与 x64 都是 64 位 Intel/AMD，不是 32 位。

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
| `CMAKE_COMPILE_WARNING_AS_ERROR` | 编译器警告使构建失败 | `ON` |

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

`scripts\build.bat` 会通过 `vswhere` 定位 Visual Studio，激活 MSVC x64 环境，然后选用与该
Visual Studio 对应的预设。Visual Studio 2022 使用 `windows-msvc-release`。Visual Studio 2026
使用 `windows-msvc-2026-release`，因为 vcpkg 用同一套编译器构建依赖，而 VS 2022 的链接器
解不开较新的静态 STL 符号。
该预设打开 `SYNERGY_VERSION_RELEASE`，版本字符串是 `cmake/Version.cmake` 里的 `X.Y.Z`
（当前 1.21.2），不是 `X.Y.Z-dev+<sha>`。

产物：

| 文件 | 作用 |
|------|------|
| `build\bin\Release\synergy_<X.Y.Z>.exe` | GUI。`X.Y.Z` 来自 `SYNERGY_VERSION_MAJOR.MINOR.PATCH`，若有 stage 再追加。 |
| `build\bin\Release\synergy-core.exe` | 核心。GUI 按这个固定名字查找，不要加版本号。 |
| `build\bin\Release\synergy-daemon.exe` | Windows 服务辅助进程，同样不带版本号。便携 7Z 故意不含它。 |

检查产物。下面的 `1.21.2` 是当前 `Version.cmake` 的值：

```bat
build\bin\Release\synergy_1.21.2.exe --version
build\bin\Release\synergy-core.exe --version
build\bin\Release\synergy-daemon.exe --version
build\bin\Release\synergy-core.exe server
```

`synergy-core --version` 同时打印产品版本和 `protocol v1.8`。GUI 打印 `TuPig Synergy: 1.21.2`。daemon 只打印 `1.21.2`。参数说明见 [configuration.md](configuration.md#命令行)。

#### Windows 代码签名

Release CI 通过 [SignPath Foundation](https://signpath.org/) 签名 `synergy_<X.Y.Z>.exe`、`synergy-core.exe`、`synergy-daemon.exe` 和 MSI。私钥留在 SignPath 的 HSM 上。本仓库不存放证书、密码或令牌。便携 7Z 由这些已签名的可执行文件打成，压缩包本身不是 Authenticode 文件。本地 Release 构建不签名，因为 Foundation 的私钥不能拿到开发机上用。

只有下面三项 GitHub Actions 值都设置了才签名（Settings、Secrets and variables、Actions）：

| 名称 | 种类 | 含义 |
|---|---|---|
| `SIGNPATH_API_TOKEN` | secret | 可提交该签名策略的用户 API 令牌。唯一的 secret。 |
| `SIGNPATH_ORGANIZATION_ID` | variable | Foundation 批准项目后给出的组织 id。 |
| `SIGNPATH_PROJECT_SLUG` | variable | 本仓库对应的项目 slug。 |

`SIGNPATH_POLICY_SLUG` 是可选变量。为空时工作流使用 `release-signing`。

到 <https://signpath.org/> 为公开仓库 `https://github.com/Tupig/Tupig_synergy` 申请。批准之后：

1. 在本仓库安装 SignPath GitHub App，并把预置的 Trusted Build System GitHub.com 关联到该项目。
2. 创建签名策略 `release-signing`，范围是 `main` 分支和 Release 标签。
3. 按仓库里的 XML 添加两份构件配置，slug 必须一致：`windows-executables` 对应 `.signpath/windows-executables.xml`，`windows-msi` 对应 `.signpath/windows-msi.xml`。
4. 把 API 令牌存成上面的 secret，把组织 id 和项目 slug 存成那两个变量。这三项都不要提交进仓库。

缺任何一项必填值时，作业记下签名已跳过，并上传未签名包。三项都在、SignPath 返回文件之后 `signtool verify` 失败，则作业失败。

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

> **说明**：AppImage **未实现** —— `deploy/`、`.github/` 中不存在任何 `appimage` 引用。Flatpak 清单位于 `deploy/linux/flatpak/`，由 CI 的 `build-flatpak` job 消费；本地 `package` 目标不产出 Flatpak 包。MSI 使用机器上已安装的 WiX（CI 固定 5.0.2；CPack 请求 WiX v4 schema）。

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

### 源码编码

CI 作业 `lint-encoding` 在版本计划之后、编译和打包之前运行 `python3 scripts/ci/check-encoding.py`。在检出的仓库里执行同一条命令即可。它按配置的根目录往下走，跳过指定的目录名，并分块读取每个源码文件。

默认白名单是 `utf-8`，即不带 BOM 的 UTF-8。ASCII 文件会被标成 `ascii`，仍然通过，因为 ASCII 属于 UTF-8。要允许 BOM，把 `utf-8-bom` 加进白名单（别名是 `utf-8-sig` 和 `utf-8-with-bom`）。若只允许 ASCII，白名单里就只留 `ascii`。

`on_violation` 默认是 `fail`：每个不合规文件打一行，进程以 1 退出，编译作业不会开始。设成 `warn`，或加上 `--warn`，会打出同样的行并以 0 退出。配置写成 `warn` 时，`--fail` 仍强制以非零退出。

| 键 | 默认 | 含义 |
|----|------|------|
| `roots` | `src`、`cmake`、`scripts`、`deploy`、`.github`、`translations` | 从仓库根开始扫描的目录 |
| `extensions` | C/C++、Qt、CMake、脚本、YAML、JSON、XML | 算作源码的后缀 |
| `names` | `CMakeLists.txt` | 不论后缀都纳入的文件名 |
| `allowed` | `utf-8` | 白名单。也可写 `ascii` 和 `utf-8-bom` |
| `exclude_dirs` | `vendor`、`build`、`.git`、`dist`、`node_modules`、`downloads`、`vcpkg_installed`、`generated`、`autogen`、`CMakeFiles` | 任意层级都跳过的目录名 |
| `on_violation` | `fail` | `fail` 以 1 退出。`warn` 只打印并以 0 退出 |

失败时路径和检测到的编码在同一行：

```text
encoding FAIL src/foo.cpp detected=utf-8-bom allowed=utf-8
encoding FAIL count=1 checked=669 mode=fail
```

全部通过时只有一行：`encoding OK checked=669 allowed=utf-8 mode=fail`。

---

### ❓ 常见编译问题排查

| 问题 | 解决方案 |
|------|----------|
| `Qt6 not found` | 重新运行 `scripts\build.bat` 或 `scripts/build.sh`。不要把 `Qt6_DIR` 或 `CMAKE_PREFIX_PATH` 指到发行版 Qt。 |
| `OpenSSL not found` | 同上。OpenSSL 是 vcpkg 清单依赖，不是要另外指给 CMake 的系统包。 |
| `X11 libs missing`（X11 库缺失） | 安装 `libx11-dev`、`libxi-dev`、`libxtst-dev` 以及上面 Linux 软件包列表里的其余项。 |
| `Wayland protocols`（Wayland 协议） | 日志点名时再安装 `libwayland-dev` 与 `wayland-protocols`。 |
| vcpkg 下载中断 | 重新运行构建脚本。保留 `vendor/vcpkg/downloads/`；不完整文件会因 SHA-512 校验失败而被拒绝。 |
| `encoding FAIL ... detected=` | 该文件不在白名单里。另存为不带 BOM 的 UTF-8，或把检测到的编码加进 `allowed`。`utf-8` 已经接受 `ascii`。 |

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

The `*-release` presets set `SYNERGY_VERSION_RELEASE=ON`, so the version string is `X.Y.Z` from `cmake/Version.cmake`. Configuring without that preset (or without `-DSYNERGY_VERSION_RELEASE=ON`) produces `X.Y.Z-dev+<sha>`.

这些 `*-release` 预设都会设置 `SYNERGY_VERSION_RELEASE=ON`，版本字符串是 `cmake/Version.cmake` 里的 `X.Y.Z`。不用该预设、也不传 `-DSYNERGY_VERSION_RELEASE=ON` 时，版本是 `X.Y.Z-dev+<sha>`。

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

> **Windows prerequisite / Windows 前置条件**: a C++ toolchain (Visual Studio 2022 or 2026 Build Tools with
> the "Desktop development with C++" workload), CMake 3.25+ and Git. Run `setup.bat` once to install
> them. `scripts\build.bat` locates Visual Studio through `vswhere` and activates the MSVC environment
> itself — do not hardcode install paths or pre-set `VCPKG_ROOT`.
>
> **Windows 前置条件**：C++ 工具链（含 "Desktop development with C++" 工作负载的 Visual Studio 2022
> 或 2026 Build Tools）、CMake 3.25+ 与 Git。首次运行 `setup.bat` 安装。`scripts\build.bat` 会通过 `vswhere`
> 自行定位 Visual Studio 并激活 MSVC 环境 —— 不要硬编码安装路径，也不要预设 `VCPKG_ROOT`。

### Presets in `CMakePresets.json` / 预设清单

| Preset | Purpose / 用途 |
|---|---|
| `windows-msvc`, `windows-msvc-release`, `windows-msvc-debug` | Windows x64, Visual Studio 2022, static triplet |
| `windows-msvc-2026`, `windows-msvc-2026-release` | Windows x64, Visual Studio 2026, static triplet. `scripts\build.bat` selects this when VS 2026 is the newest install. |
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
