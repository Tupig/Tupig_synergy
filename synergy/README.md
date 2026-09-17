# Synergy - 无序列号版本

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
├── src/                 # 核心源代码
│   ├── lib/             # 核心库
│   ├── apps/            # 应用程序
│   └── unittests/       # 单元测试
├── extra/               # 扩展功能（许可证系统已移除）
├── cmake/               # CMake 配置
├── subprojects/         # 第三方依赖
└── docs/                # 文档
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
