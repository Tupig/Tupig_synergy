# TuPig Synergy - 无序列号版本

基于 [Synergy](https://github.com/symless/synergy) 的修改版本，移除了序列号验证和许可证激活系统，可直接使用。

## 功能特性

- 跨平台键鼠共享（Windows、macOS、Linux）
- 支持多台电脑之间的无缝切换
- **无需序列号/许可证密钥**
- 基于 Qt6 的图形用户界面
- TLS 加密通信支持

## 项目结构

```
synergy/
├── src/                          # 核心源代码
│   ├── apps/                     # 应用程序
│   │   ├── deskflow-core/        # 核心服务
│   │   ├── deskflow-daemon/      # 守护进程
│   │   ├── deskflow-gui/         # GUI界面
│   │   └── res/                  # 资源文件
│   └── lib/                      # 核心库
│       ├── arch/                 # 架构抽象层（跨平台接口）
│       ├── base/                 # 基础工具（事件、日志、字符串）
│       ├── client/               # 客户端实现
│       ├── common/               # 公共定义（常量、枚举、设置）
│       ├── deskflow/             # 核心逻辑库
│       │   ├── core/             # 应用核心（App、Client、Server）
│       │   ├── clipboard/        # 剪贴板功能
│       │   ├── input/            # 输入处理（键盘、鼠标）
│       │   ├── protocol/         # 网络协议
│       │   ├── screen/           # 屏幕管理
│       │   ├── ipc/              # 进程间通信
│       │   ├── unix/             # Unix/Linux特定代码
│       │   └── win32/            # Windows特定代码
│       ├── gui/                  # GUI组件
│       │   ├── config/           # 配置管理
│       │   ├── core/             # GUI核心
│       │   ├── dialogs/          # 对话框
│       │   ├── ipc/              # GUI IPC
│       │   ├── validators/       # 输入验证
│       │   └── widgets/          # 自定义控件
│       ├── io/                   # IO流抽象
│       ├── mt/                   # 多线程（互斥锁、条件变量）
│       ├── net/                  # 网络（TCP、SSL、Socket）
│       ├── platform/             # 平台特定实现
│       │   ├── win32/            # Windows实现
│       │   ├── macos/            # macOS实现
│       │   └── linux/            # Linux实现（X11、Wayland、Portal）
│       └── server/               # 服务端实现
├── extra/                        # 扩展功能（GUI hooks）
├── cmake/                        # CMake配置
├── deploy/                       # 部署脚本
│   ├── linux/
│   ├── mac/
│   └── windows/
├── docs/                         # 文档
├── translations/                 # 翻译文件
├── .github/                      # GitHub配置
└── .vscode/                      # VS Code配置
```

## 编译说明

### 环境要求

- **Windows**: Visual Studio 2022 + CMake 3.24+ + Qt 6.7+ + vcpkg
- **macOS**: Xcode + CMake 3.24+ + Qt 6.7+ (Homebrew)
- **Linux**: GCC 12+ / Clang 15+ + CMake 3.24+ + Qt 6.7+

### 编译步骤

#### Windows

```powershell
# 安装依赖后
cmake -Bbuild -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

#### macOS (Apple Silicon)

```bash
cmake -Bbuild -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64"
cmake --build build --config Release
```

#### Linux

```bash
cmake -Bbuild -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j$(nproc)
```

## 模块说明

### 核心库 (src/lib/)

| 模块 | 说明 | 文件数 |
|------|------|--------|
| `arch` | 架构抽象层，提供跨平台的系统调用接口 | ~10 |
| `base` | 基础工具库，事件队列、日志、字符串处理 | ~32 |
| `client` | 客户端实现，处理与服务端的连接 | ~5 |
| `common` | 公共定义，常量、枚举、配置 | ~16 |
| `deskflow` | 核心逻辑，键鼠共享的主要实现 | ~59 |
| `gui` | Qt GUI组件，用户界面 | ~33 |
| `io` | IO流抽象 | ~8 |
| `mt` | 多线程原语 | ~12 |
| `net` | 网络通信，TCP/SSL | ~33 |
| `platform` | 平台特定实现 | ~121 |
| `server` | 服务端实现 | ~35 |

### 应用程序 (src/apps/)

| 应用 | 说明 |
|------|------|
| `deskflow-core` | 核心服务进程 |
| `deskflow-daemon` | 后台守护进程 |
| `deskflow-gui` | 图形用户界面 |

## 修改说明

本版本对原版 Synergy 进行了以下修改：

1. **移除序列号验证** - `gui_hook.h` 中所有许可证检查钩子已被绕过
2. **禁用许可证激活** - `LicenseHandler.cpp` 中所有强制执行方法已被禁用
3. **保留核心功能** - 所有键鼠共享功能保持完整

## 技术栈

- C++20
- Qt 6.7+
- CMake 3.24+
- OpenSSL 3.0+

## 致谢

本项目基于以下开源项目：

- [Synergy](https://github.com/symless/synergy) - 原始项目
- [Deskflow](https://deskflow.org) - 上游社区项目

## 许可证

本项目遵循原项目的 GNU General Public License v2.0 许可证。
