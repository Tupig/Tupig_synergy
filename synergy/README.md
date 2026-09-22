# TuPig Synergy / TuPig Synergy

<div align="center">

![TuPig Synergy Banner](https://capsule-render.vercel.app/api?type=waving&color=gradient&customColorList=0,2,15,30&height=180&section=header&text=TuPig%20Synergy&fontSize=45&fontAlignY=35&desc=跨平台键鼠共享工具%20%7C%20Cross-platform%20Keyboard%20Mouse%20Sharing&descAlignY=55&descAlign=50)

**基于 Synergy 的免许可证现代分支 — 一套键鼠，无缝掌控多台电脑。**  
**A modern, license-free fork of Synergy — Share one keyboard and mouse across multiple computers seamlessly.**

</div>

---

## 📊 Project Status / 项目状态

<div align="center">

![License](https://img.shields.io/github/license/Tupig/TuPig_Product?style=flat-square&color=2D8CFF)
![Release](https://img.shields.io/github/v/release/Tupig/TuPig_Product?style=flat-square&color=00C853&include_prereleases)
![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20macOS%20%7C%20Linux-0EA5E9?style=flat-square)
![C++](https://img.shields.io/badge/C%2B%2B-20-00599C?style=flat-square&logo=c%2B%2B)
![Qt](https://img.shields.io/badge/Qt-6.7%2B-41CD52?style=flat-square&logo=qt)
![CMake](https://img.shields.io/badge/CMake-3.24%2B-064F8C?style=flat-square&logo=cmake)

</div>

**本项目基于 Synergy/Deskflow，移除了序列号验证与许可证激活，开箱即用。**  
**This project is based on Synergy/Deskflow, with serial key verification and license activation removed — ready to use out of the box.**

---

## ✨ Key Features / 核心特性

<div align="center">

| Feature / 特性 | Description / 说明 | Status / 状态 |
|---|---|---|
| 🖱️ **Seamless Sharing / 无缝共享** | One keyboard & mouse across multiple computers / 一套键鼠控制多台电脑 | ✅ Stable / 稳定 |
| 🌐 **Cross-Platform / 跨平台** | Windows, macOS, Linux (X11/Wayland) | ✅ Native / 原生 |
| 🔒 **TLS Encryption / TLS 加密** | Secure communication with OpenSSL 3.0+ / OpenSSL 3.0+ 安全通信 | ✅ Enabled / 已启用 |
| 📋 **Clipboard Sync / 剪贴板同步** | Shared clipboard across all hosts / 所有主机共享剪贴板 | ✅ Full / 完全 |
| 📁 **File Drag-Drop / 文件拖拽** | Drag files between computers / 电脑间拖拽文件 | ✅ Supported / 支持 |
| ⌨️ **Hotkey Switching / 热键切屏** | Instant screen switching via custom hotkeys / 自定义热键瞬间切换 | ✅ Configurable / 可配置 |
| 🚫 **No License Required / 无需许可证** | Completely free, no serial keys or activation / 完全免费，无序列号/激活 | ✅ Forever / 永久 |
| 🎨 **Modern Qt6 UI / 现代 Qt6 界面** | Beautiful, responsive graphical interface / 美观、响应式图形界面 | ✅ Polished / 打磨完成 |

</div>

---

## 🏗️ Architecture Overview / 架构概览

<div align="center">

```mermaid
graph TB
    subgraph "User Interface / 用户界面"
        GUI[Qt6 GUI<br/>synergy]
        CLI[CLI Tools<br/>synergy-core / synergy-daemon]
    end

    subgraph "Core Libraries / 核心库"
        APP[Application Layer<br/>app, common]
        DESKFLOW[Deskflow Core<br/>core, clipboard, input, protocol, screen]
        NET[Network Stack<br/>net, io, mt]
        PLATFORM[Platform Abstraction<br/>arch, platform/*]
        SERVER[Server Implementation<br/>server]
        CLIENT[Client Implementation<br/>client]
    end

    subgraph "External Dependencies / 外部依赖"
        QT[Qt 6.7+]
        SSL[OpenSSL 3.0+]
        CMAKE[CMake 3.24+]
    end

    GUI --> APP
    CLI --> APP
    APP --> DESKFLOW
    DESKFLOW --> NET
    DESKFLOW --> PLATFORM
    DESKFLOW --> SERVER
    DESKFLOW --> CLIENT
    NET --> SSL
    PLATFORM --> QT
    APP --> QT
```

</div>

---

## 🚀 Quick Start / 快速开始

### One-Click Setup / 一键环境搭建

```bat
REM 1. Clone
git clone https://github.com/Tupig/TuPig_Product.git
cd TuPig_Product

REM 2. Right-click setup.bat → Run as Administrator
REM    (installs CMake, MSVC Build Tools, Git, bootstraps vcpkg)

REM 3. Build
cmake --preset windows-msvc-release
cmake --build build --config Release
```

Output (VS multi-config): `build\bin\Release\synergy.exe` (GUI),
`build\bin\Release\synergy-core.exe` (core),
`build\bin\Release\synergy-daemon.exe` (Windows daemon)

### Prerequisites (auto-installed by setup.bat)

| Requirement | Windows | macOS | Linux |
|---|---|---|---|
| **CMake 3.24+** | `setup.bat` installs | `brew install cmake` | `sudo apt install cmake` |
| **Ninja** | Included with VS Build Tools | `brew install ninja` | `sudo apt install ninja-build` |
| **vcpkg** | `setup.bat` bootstraps | `brew install vcpkg` | manual bootstrap |
| **MSVC Build Tools** | `setup.bat` installs | — | — |
| **"Desktop development with C++"** | Select in VS Installer | — | — |

> **Note for MSVC**: When `setup.bat` installs Build Tools, the Visual Studio Installer will open.  
> You **must** select the **"Desktop development with C++"** workload before clicking Install.

### Switching Machines

Just re-run `setup.bat` — it's idempotent and only installs what's missing.

---

## 📁 Project Structure / 项目结构

```
synergy/
├── cmake/                      # CMake modules
├── config/                     # Tool configs (SonarQube)
├── deploy/                     # Platform packaging (AppImage, DMG, MSI)
├── docs/                       # Documentation
├── extra/                      # Branding, deploy resources
├── scripts/                    # Build scripts (build.bat, build.sh)
├── src/                        # Source code
│   ├── apps/                   # Entry points (core, daemon, gui)
│   ├── lib/                    # 12 core libraries
│   └── unittests/              # GoogleTest unit tests
├── translations/               # Qt .ts translation files
├── triplets/                   # vcpkg overlay triplets (static linking)
├── vcpkg.json                  # vcpkg dependency manifest
├── CMakeLists.txt              # Root build config
├── CMakePresets.json           # Build presets (per-platform)
└── README.md                   # This file
```

---

## 🔧 Configuration / 配置

### Server (Primary / 主控端)

```ini
[core]
computerName = my-desktop
port = 24800
interface = 0.0.0.0

[server]
screens = main, laptop
main.position = 0,0
laptop.position = right,main
```

### Client (Secondary / 被控端)

```ini
[client]
serverHost = 192.168.1.100
serverPort = 24800
reconnectInterval = 5
autoConnect = true
```

---

## 📚 Documentation / 文档

| Document | Description |
|---|---|
| [build.md](docs/build.md) | Detailed Build Guide / 编译指南 |
| [contributing.md](docs/contributing.md) | Contributing Guide / 贡献指南 |
| [protocol.md](docs/protocol.md) | Protocol Reference / 协议参考 (v1.8) |
| [configuration.md](docs/configuration.md) | Configuration Reference / 配置参考 |
| [architecture.md](docs/architecture.md) | Architecture Decision Records / 架构决策记录 |
| [troubleshooting.md](docs/troubleshooting.md) | Troubleshooting Guide / 故障排查 |
| [security.md](docs/security.md) | Security Policy / 安全策略 |

---

## 📜 License / 许可证

<div align="center">

本项目遵循 **GNU General Public License v2.0** — 详见 [LICENSE](LICENSE)  
This project is licensed under the **GNU General Public License v2.0** — see [LICENSE](LICENSE)

</div>

---

## 🙏 Acknowledgments / 致谢

| Project | Role | License |
|---|---|---|
| [Synergy](https://github.com/symless/synergy) | Original upstream | GPL-2.0 |
| [Deskflow](https://deskflow.org) | Community upstream | GPL-2.0 |
| [Qt](https://www.qt.io/) | GUI Framework | LGPL-3.0 / Commercial |
| [OpenSSL](https://www.openssl.org/) | TLS/Crypto | Apache-2.0 |
| [CMake](https://cmake.org/) | Build System | BSD-3-Clause |
| [vcpkg](https://github.com/microsoft/vcpkg) | Dependency Manager | MIT |

</div>

---

<div align="center">

### ⭐ If you find this project useful, please consider giving it a star!

[![Star History Chart](https://api.star-history.com/svg?repos=Tupig/TuPig_Product&type=Date&theme=dark)](https://star-history.com/#Tupig/TuPig_Product&Date)

</div>
