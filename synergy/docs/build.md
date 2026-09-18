# Build Guide / 编译指南

> **Language / 语言**: [English](#english) | [中文](#中文)

---

## English

### Prerequisites

| Component | Minimum Version | Notes |
|-----------|----------------|-------|
| **CMake** | 3.24+ | Modern CMake required |
| **Qt** | 6.7.0+ | Core, Widgets, Network, DBus (Linux) |
| **OpenSSL** | 3.0+ | TLS/crypto support |
| **libportal** | 0.9.1+ | Linux/BSD only (Wayland portal) |
| **libei** | 1.3+ | Linux/BSD only (input emulation) |

### Default Build Options

The following components are enabled by default:

- ✅ TuPig Synergy GUI Application (`deskflow-gui`)
- ✅ TuPig Synergy Core Service (`deskflow-core`)
- ✅ Daemon for Windows UAC handling (`deskflow-daemon`)
- ✅ Doxygen Documentation (if Doxygen installed)
- ✅ Build-time Unit Tests (GoogleTest)

### CMake Configuration Options

| Option | Description | Default | Dependencies |
|--------|-------------|---------|--------------|
| `BUILD_USER_DOCS` | Build user-facing documentation | `DOXYGEN_FOUND` | Doxygen |
| `BUILD_DEV_DOCS` | Build developer/API documentation | `OFF` | Doxygen |
| `BUILD_INSTALLER` | Build platform installers | `ON` | Platform tools |
| `BUILD_TESTS` | Build unit tests | `ON` | Qt Test |
| `BUILD_X11_SUPPORT` | Build X11 backend (Linux/BSD) | `ON` | X11 libraries |
| `BUILD_OSX_BUNDLE` | Build macOS .app bundle | `ON` | — |
| `ENABLE_COVERAGE` | Enable code coverage reports | `OFF` | gcov/lcov |
| `SKIP_BUILD_TESTS` | Skip tests during build | `OFF` | — |
| `VCPKG_QT` | Use vcpkg for Qt (Windows only) | `OFF` | vcpkg |
| `CLEAN_TRS` | Remove obsolete translation strings | `OFF` | — |
| `APPLE_CODESIGN_DEV` | Apple Developer code-sign identity | unset | Xcode |

**Basic Configuration Example:**

```bash
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build
```

---

### 🪟 Windows (MSVC + vcpkg)

#### Option 1: System Qt (Recommended for Development)

```powershell
# 1. Install Qt 6.7+ via Qt Online Installer
#    Select: MSVC 2022 64-bit, Qt 6.7+

# 2. Add to System PATH:
#    C:\Qt\6.7.x\msvc2022_64\bin
#    C:\Qt\6.7.x\msvc2022_64\lib\cmake

# 3. Configure & Build
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

#### Option 2: vcpkg-managed Qt (CI / Reproducible Builds)

```powershell
# 1. Install vcpkg
git clone https://github.com/microsoft/vcpkg.git
.\vcpkg\bootstrap-vcpkg.bat
.\vcpkg\vcpkg integrate install

# 2. Configure with vcpkg toolchain
cmake -S . -B build -G Ninja `
  -DCMAKE_BUILD_TYPE=Release `
  -DVCPKG_QT=ON `
  -DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake

cmake --build build --config Release
```

> ⚠️ **Note**: Do not mix system Qt and vcpkg Qt. Switching requires deleting `build/` and `vcpkg.json`.

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
./build/bin/synergy-core-1.21.2    # Core service
./build/bin/synergy-1.21.2         # GUI
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
./build/bin/synergy-core-1.21.2    # Core service
./build/bin/synergy-1.21.2         # GUI
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
| **Linux** | DEB, RPM, AppImage, Flatpak |
| **macOS** | DMG (signed + notarized) |
| **Windows** | MSI (WiX), NSIS, ZIP |

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
| **CMake** | 3.24+ | 需要现代 CMake 特性 |
| **Qt** | 6.7.0+ | Core, Widgets, Network, DBus (Linux) |
| **OpenSSL** | 3.0+ | TLS/加密支持 |
| **libportal** | 0.9.1+ | 仅 Linux/BSD (Wayland Portal) |
| **libei** | 1.3+ | 仅 Linux/BSD (输入仿真) |

### 默认启用组件

- ✅ TuPig Synergy GUI 程序 (`deskflow-gui`)
- ✅ TuPig Synergy 核心服务 (`deskflow-core`)
- ✅ Windows UAC 守护进程 (`deskflow-daemon`)
- ✅ Doxygen 文档 (检测到 Doxygen 时)
- ✅ 编译时单元测试 (GoogleTest)

### CMake 配置选项

| 选项 | 说明 | 默认值 | 依赖 |
|------|------|--------|------|
| `BUILD_USER_DOCS` | 编译用户文档 | `DOXYGEN_FOUND` | Doxygen |
| `BUILD_DEV_DOCS` | 编译开发者/API 文档 | `OFF` | Doxygen |
| `BUILD_INSTALLER` | 编译平台安装包 | `ON` | 平台工具 |
| `BUILD_TESTS` | 编译单元测试 | `ON` | Qt Test |
| `BUILD_X11_SUPPORT` | 编译 X11 后端 (Linux/BSD) | `ON` | X11 库 |
| `BUILD_OSX_BUNDLE` | 编译 macOS .app 包 | `ON` | — |
| `ENABLE_COVERAGE` | 启用代码覆盖率报告 | `OFF` | gcov/lcov |
| `SKIP_BUILD_TESTS` | 跳过编译时测试 | `OFF` | — |
| `VCPKG_QT` | 使用 vcpkg 管理 Qt (仅 Windows) | `OFF` | vcpkg |
| `CLEAN_TRS` | 清理翻译文件中过时字符串 | `OFF` | — |
| `APPLE_CODESIGN_DEV` | Apple 开发者代码签名身份 | 未设置 | Xcode |

**基础配置示例：**

```bash
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build
```

---

### 🪟 Windows (MSVC + vcpkg)

#### 方案一：系统 Qt（推荐用于开发）

```powershell
# 1. 通过 Qt 在线安装器安装 Qt 6.7+
#    勾选：MSVC 2022 64-bit, Qt 6.7+

# 2. 添加到系统 PATH:
#    C:\Qt\6.7.x\msvc2022_64\bin
#    C:\Qt\6.7.x\msvc2022_64\lib\cmake

# 3. 配置与编译
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

#### 方案二：vcpkg 管理 Qt（CI / 可复现构建）

```powershell
# 1. 安装 vcpkg
git clone https://github.com/microsoft/vcpkg.git
.\vcpkg\bootstrap-vcpkg.bat
.\vcpkg\vcpkg integrate install

# 2. 使用 vcpkg 工具链配置
cmake -S . -B build -G Ninja `
  -DCMAKE_BUILD_TYPE=Release `
  -DVCPKG_QT=ON `
  -DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake

cmake --build build --config Release
```

> ⚠️ **注意**：不要混用系统 Qt 和 vcpkg Qt。切换时需删除 `build/` 与 `vcpkg.json`。

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
./build/bin/synergy-core-1.21.2    # 核心服务
./build/bin/synergy-1.21.2         # GUI
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
./build/bin/synergy-core-1.21.2    # 核心服务
./build/bin/synergy-1.21.2         # GUI
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
| **Linux** | DEB, RPM, AppImage, Flatpak |
| **macOS** | DMG (签名 + 公证) |
| **Windows** | MSI (WiX), NSIS, ZIP |

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

## CMake Presets / CMake 预设配置

项目提供 `CMakePresets.json` 与 `CMakeUserPresets.json`：

```json
{
  "version": 6,
  "configurePresets": [
    {
      "name": "release",
      "displayName": "Release Build",
      "binaryDir": "${sourceDir}/build",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Release"
      }
    },
    {
      "name": "debug",
      "displayName": "Debug Build",
      "binaryDir": "${sourceDir}/build-debug",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug"
      }
    }
  ]
}
```

**Usage / 使用：**

```bash
# List presets / 列出预设
cmake --list-presets

# Use preset / 使用预设
cmake --preset=release
cmake --build --preset=release
```