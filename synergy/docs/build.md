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
| **libportal** | 0.9.1+ | Linux/BSD only (Wayland portal) |
| **libei** | 1.3+ | Linux/BSD only (input emulation) |

### Default Build Options

The following components are enabled by default:

- ✅ TuPig Synergy GUI Application (`synergy`)
- ✅ TuPig Synergy Core Service (`synergy-core`)
- ✅ Daemon for Windows UAC handling (`synergy-daemon`, Windows only)
- ✅ Doxygen Documentation (if Doxygen installed)
- ✅ Build-time Unit Tests (GoogleTest)

### CMake Configuration Options

| Option | Description | Default |
|--------|-------------|---------|
| `BUILD_GUI` | Build the Qt GUI | `ON` |
| `BUILD_TESTS` | Build unit tests | `ON` |
| `BUILD_INSTALLER` | Build platform installers | `ON` |
| `BUILD_X11_SUPPORT` | Build X11 backend (Linux/BSD) | `ON` |
| `BUILD_OSX_BUNDLE` | Build macOS `.app` bundle | `ON` |
| `SKIP_BUILD_TESTS` | Skip tests during build | `OFF` |
| `ENABLE_COVERAGE` | Enable code coverage reports | `OFF` |
| `CLEAN_TRS` | Remove obsolete translation strings | `OFF` |
| `SYNERGY_CORE_FLAVOR` | Build as "TuPig Synergy Core"; seeds headless defaults (GUI/tests/installer off) | `OFF` |
| `APPLE_CODESIGN_DEV` | Apple Developer code-sign identity (cache variable, not an `option()`) | unset |

**Basic Configuration Example:**

```bash
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build
```

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

Output: `build\bin\Release\`.

#### Windows Code Signing (Optional)

For distribution builds, configure Authenticode signing in `deploy/windows/`.

---

### 🍎 macOS (Apple Silicon / Intel)

```bash
# 1. Install dependencies via Homebrew
brew install cmake ninja qt@6 openssl@3

# 2. Configure (Apple Silicon native)
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_PREFIX_PATH="$(brew --prefix qt@6)"

cmake --build build --config Release

# 3. Run
./build/bin/synergy-core    # Core service
./build/bin/synergy         # GUI
```

#### macOS Code Signing (Development)

```bash
# 1. Get Developer ID certificate
security find-identity -v -p codesigning login.keychain-db

# 2. Pass to CMake
cmake -S . -B build -DAPPLE_CODESIGN_DEV="Apple Development: Name (TEAMID)"

# 3. Verify
codesign -d -r- build/bin/TuPig\ Synergy.app
```

> **Development vs Distribution**: Local dev uses `Apple Development` cert with hardened runtime. Distribution builds use `Developer ID Application` cert with notarization via CI.

---

### 🐧 Linux (Ubuntu / Debian / Fedora / Arch)

#### Ubuntu / Debian

```bash
sudo apt update && sudo apt install -y \
  cmake ninja-build g++ \
  qt6-base-dev libssl-dev \
  libx11-dev libxi-dev libxtst-dev \
  libxinerama-dev libxrandr-dev \
  libxkbcommon-dev libglib2.0-dev \
  libportal-dev libei-dev \
  doxygen graphviz  # optional: for docs
```

#### Fedora / RHEL

```bash
sudo dnf install -y \
  cmake ninja-build gcc-c++ \
  qt6-qtbase-devel openssl-devel \
  libX11-devel libXi-devel libXtst-devel \
  libXinerama-devel libXrandr-devel \
  libxkbcommon-devel glib2-devel \
  libportal-devel libei-devel
```

#### Arch Linux

```bash
sudo pacman -S \
  cmake ninja gcc \
  qt6-base openssl \
  libx11 libxi libxtst libxinerama \
  libxrandr libxkbcommon glib2 \
  libportal libei
```

#### Build

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j$(nproc)

# Run
./build/bin/synergy-core    # Core service
./build/bin/synergy         # GUI
```

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
| **Linux** | DEB or RPM (auto-selected from `/etc/os-release`) |
| **macOS** | DMG (DragNDrop) |
| **Windows** | 7Z (portable), MSI (WiX, see note) |

> **Note**: AppImage is not implemented — no `appimage` reference exists in
> `deploy/`, `extra/` or `.github/`. A Flatpak manifest is present
> (`extra/deploy/linux/flatpak/`) but nothing in the build
> consumes it, so no Flatpak bundle is produced either. MSI generation
> additionally requires accepting the WiX v7 OSMF EULA. See
> [delivery.md](delivery.md) for the full per-platform matrix and constraints.

---

### 🔧 Advanced Configuration

#### Cross-Compilation (Linux → Windows)

```bash
# Requires mingw-w64 and Qt for MinGW
cmake -S . -B build-mingw \
  -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-toolchain.cmake \
  -DCMAKE_BUILD_TYPE=Release
```

#### Sanitizer Builds

```bash
# AddressSanitizer
cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer"

# ThreadSanitizer
cmake -S . -B build-tsan -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=thread -fPIC"

# MemorySanitizer (Clang only)
cmake -S . -B build-msan -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=memory -fPIE -fno-omit-frame-pointer"
```

#### ccache Acceleration

```bash
cmake -S . -B build \
  -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
```

---

### 🧪 Testing

```bash
# Run all tests
ctest --test-dir build --output-on-failure

# Run specific test suite
ctest --test-dir build -R "ClipboardTests" --output-on-failure

# With coverage (requires ENABLE_COVERAGE=ON)
cmake --build build --target coverage
```

---

### ❓ Troubleshooting Build Issues

| Issue | Solution |
|-------|----------|
| `Qt6 not found` | Set `Qt6_DIR` or `CMAKE_PREFIX_PATH` to Qt's lib/cmake dir |
| `OpenSSL not found` | Install `libssl-dev` (Linux) / `openssl@3` (macOS) / vcpkg (Windows) |
| `X11 libs missing` | Install `libx11-dev`, `libxi-dev`, `libxtst-dev` etc. |
| `Wayland protocols` | Install `libwayland-dev`, `wayland-protocols` |
| `vcpkg Qt timeout` | Increase timeout: `set(VCPKG_BUILD_TIMEOUT 3600)` in CMake |

---

## 中文

### 编译环境要求

| 组件 | 最低版本 | 说明 |
|------|----------|------|
| **CMake** | 3.25+ | 需要现代 CMake 特性 |
| **Qt** | 6.7.0+ | Core, Widgets, Network, DBus (Linux) |
| **OpenSSL** | 3.0+ | TLS/加密支持 |
| **libportal** | 0.9.1+ | 仅 Linux/BSD (Wayland Portal) |
| **libei** | 1.3+ | 仅 Linux/BSD (输入仿真) |

### 默认启用组件

- ✅ TuPig Synergy GUI 程序 (`synergy`)
- ✅ TuPig Synergy 核心服务 (`synergy-core`)
- ✅ Windows UAC 守护进程 (`synergy-daemon`，仅 Windows)
- ✅ Doxygen 文档 (检测到 Doxygen 时)
- ✅ 编译时单元测试 (GoogleTest)

### CMake 配置选项

| 选项 | 说明 | 默认值 |
|------|------|--------|
| `BUILD_GUI` | 编译 Qt GUI | `ON` |
| `BUILD_TESTS` | 编译单元测试 | `ON` |
| `BUILD_INSTALLER` | 编译平台安装包 | `ON` |
| `BUILD_X11_SUPPORT` | 编译 X11 后端 (Linux/BSD) | `ON` |
| `BUILD_OSX_BUNDLE` | 编译 macOS `.app` 包 | `ON` |
| `SKIP_BUILD_TESTS` | 跳过编译时测试 | `OFF` |
| `ENABLE_COVERAGE` | 启用代码覆盖率报告 | `OFF` |
| `CLEAN_TRS` | 清理翻译文件中过时字符串 | `OFF` |
| `SYNERGY_CORE_FLAVOR` | 以 “TuPig Synergy Core” 构建；同时将 GUI/测试/安装包默认置为关闭（无界面构建） | `OFF` |
| `APPLE_CODESIGN_DEV` | Apple 开发者代码签名身份（缓存变量，非 `option()`） | 未设置 |

**基础配置示例：**

```bash
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build
```

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

产物目录：`build\bin\Release\`。

#### Windows 代码签名（可选）

分发构建需在 `deploy/windows/` 配置 Authenticode 签名。

---

### 🍎 macOS (Apple Silicon / Intel)

```bash
# 1. 通过 Homebrew 安装依赖
brew install cmake ninja qt@6 openssl@3

# 2. 配置 (Apple Silicon 原生)
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_PREFIX_PATH="$(brew --prefix qt@6)"

cmake --build build --config Release

# 3. 运行
./build/bin/synergy-core    # 核心服务
./build/bin/synergy         # GUI
```

#### macOS 代码签名（开发用）

```bash
# 1. 获取开发者证书
security find-identity -v -p codesigning login.keychain-db

# 2. 传递给 CMake
cmake -S . -B build -DAPPLE_CODESIGN_DEV="Apple Development: Name (TEAMID)"

# 3. 验证
codesign -d -r- build/bin/TuPig\ Synergy.app
```

> **开发 vs 分发**：本地开发使用 `Apple Development` 证书 + Hardened Runtime。分发构建使用 `Developer ID Application` 证书并通过 CI 公证。

---

### 🐧 Linux (Ubuntu / Debian / Fedora / Arch)

#### Ubuntu / Debian

```bash
sudo apt update && sudo apt install -y \
  cmake ninja-build g++ \
  qt6-base-dev libssl-dev \
  libx11-dev libxi-dev libxtst-dev \
  libxinerama-dev libxrandr-dev \
  libxkbcommon-dev libglib2.0-dev \
  libportal-dev libei-dev \
  doxygen graphviz  # 可选：生成文档用
```

#### Fedora / RHEL

```bash
sudo dnf install -y \
  cmake ninja-build gcc-c++ \
  qt6-qtbase-devel openssl-devel \
  libX11-devel libXi-devel libXtst-devel \
  libXinerama-devel libXrandr-devel \
  libxkbcommon-devel glib2-devel \
  libportal-devel libei-devel
```

#### Arch Linux

```bash
sudo pacman -S \
  cmake ninja gcc \
  qt6-base openssl \
  libx11 libxi libxtst libxinerama \
  libxrandr libxkbcommon glib2 \
  libportal libei
```

#### 编译

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j$(nproc)

# 运行
./build/bin/synergy-core    # 核心服务
./build/bin/synergy         # GUI
```

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
| **Linux** | DEB 或 RPM（依 `/etc/os-release` 自动二选一） |
| **macOS** | DMG (DragNDrop) |
| **Windows** | 7Z（便携）、MSI (WiX，见说明) |

> **说明**：AppImage **未实现** —— `deploy/`、`extra/`、`.github/` 中不存在任何 `appimage` 引用。Flatpak 清单虽存在（`extra/deploy/linux/flatpak/`），但构建流程中无任何环节消费它，因此同样不产出 Flatpak 包。生成 MSI 另需接受 WiX v7 的 OSMF 许可协议。完整的各平台产物矩阵与限制见 [delivery.md](delivery.md)。

---

### 🔧 进阶配置

#### 交叉编译 (Linux → Windows)

```bash
# 需要 mingw-w64 和 MinGW 版 Qt
cmake -S . -B build-mingw \
  -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-toolchain.cmake \
  -DCMAKE_BUILD_TYPE=Release
```

#### Sanitizer 构建

```bash
# AddressSanitizer
cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer"

# ThreadSanitizer
cmake -S . -B build-tsan -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=thread -fPIC"

# MemorySanitizer (仅 Clang)
cmake -S . -B build-msan -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=memory -fPIE -fno-omit-frame-pointer"
```

#### ccache 加速编译

```bash
cmake -S . -B build \
  -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
```

---

### 🧪 测试

```bash
# 运行所有测试
ctest --test-dir build --output-on-failure

# 运行特定测试套件
ctest --test-dir build -R "ClipboardTests" --output-on-failure

# 生成覆盖率报告 (需 ENABLE_COVERAGE=ON)
cmake --build build --target coverage
```

---

### ❓ 常见编译问题排查

| 问题 | 解决方案 |
|------|----------|
| `Qt6 not found` | 设置 `Qt6_DIR` 或 `CMAKE_PREFIX_PATH` 为 Qt 的 lib/cmake 目录 |
| `OpenSSL not found` | 安装 `libssl-dev` (Linux) / `openssl@3` (macOS) / vcpkg (Windows) |
| `X11 库缺失` | 安装 `libx11-dev`, `libxi-dev`, `libxtst-dev` 等 |
| `Wayland 协议缺失` | 安装 `libwayland-dev`, `wayland-protocols` |
| `vcpkg Qt 编译超时` | 增加超时：CMake 中 `set(VCPKG_BUILD_TIMEOUT 3600)` |

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
| Windows | `scripts\build.bat [release\|debug]` |
| Linux / macOS | `./scripts/build.sh [release\|debug]` |

Both scripts call `scripts/bootstrap-vcpkg.{bat,sh}`, which clones the repository-local vcpkg into
`vendor/vcpkg`, checks out the baseline pinned by `builtin-baseline` in `vcpkg.json`, and bootstraps
the vcpkg tool. The baseline is the single source of truth and is read from `vcpkg.json`, not
duplicated in the scripts.

两个脚本都会调用 `scripts/bootstrap-vcpkg.{bat,sh}`：克隆仓库内 vcpkg 到 `vendor/vcpkg`，检出
`vcpkg.json` 中 `builtin-baseline` 固定的版本，并引导 vcpkg 工具。baseline 是唯一真源，由脚本从
`vcpkg.json` 读取，不在脚本里复制一份。

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
