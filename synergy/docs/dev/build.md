# TuPig Synergy 编译指南

## 编译环境要求

| 组件 | 版本要求 |
|------|----------|
| CMake | 3.24+ |
| Qt | 6.7.0+ |
| OpenSSL | 3.0+ |
| libportal | 0.9.1+（仅 Linux/BSD）|
| libei | 1.3+（仅 Linux/BSD）|

## 默认编译选项

启用以下组件：
- TuPig Synergy GUI 应用程序
- TuPig Synergy Core 应用程序
- Doxygen 文档（若已安装）
- 编译时测试

## CMake 配置选项

| 选项 | 说明 | 默认值 | 额外依赖 |
|------|------|--------|----------|
| BUILD_USER_DOCS | 编译用户文档 | DOXYGEN_FOUND | Doxygen |
| BUILD_DEV_DOCS | 编译开发文档 | OFF | Doxygen |
| BUILD_INSTALLER | 编译安装包 | ON | - |
| BUILD_TESTS | 编译单元测试 | ON | Qt Test |
| BUILD_X11_SUPPORT | 编译 X11 后端（仅 Linux/BSD）| ON | x11 库 |
| BUILD_OSX_BUNDLE | 编译 macOS 应用包 | ON | - |
| ENABLE_COVERAGE | 启用测试覆盖率 | OFF | gcov |
| SKIP_BUILD_TESTS | 跳过编译时测试 | OFF | - |
| VCPKG_QT | 使用 vcpkg 编译 Qt（仅 Windows）| OFF | - |
| CLEAN_TRS | 删除 tr 文件中过时的字符串 | OFF | - |
| APPLE_CODESIGN_DEV | Apple 开发者代码签名 ID | 未设置 | - |

配置示例：
```bash
cmake -S. -Bbuild -DCMAKE_INSTALL_PREFIX=<安装路径>
```

### Windows 配置

推荐使用 vcpkg 安装依赖。首次配置 TuPig Synergy 时，除 Qt 外的所有依赖将被编译。若不使用 vcpkg，需手动设置依赖（本文档不涵盖此方式）。

#### Windows 与 Qt

有两种方式安装 Qt（vcpkg 或 Qt 在线安装器）。默认配置使用 Qt 在线安装器。不应同时使用两种方式安装 Qt，否则可能导致库文件冲突。切换安装方式时，请先删除之前的安装。

##### 系统 Qt

1. 从 Qt 官网下载并安装 [Qt] 在线安装器。
2. 将 Qt 的 cmake 文件路径添加到系统环境变量（若不添加，可在配置时通过 `Qt6_DIR` 指定）。
   - 通常为：`C:\Qt\<版本>\<msvc信息>\lib\cmake`
3. 将 Qt 的二进制工具路径添加到系统环境变量。
   - 通常为：`C:\Qt\<版本>\<msvc信息>\bin`

##### vcpkg 管理的 Qt

1. 在 cmake 配置命令中添加 `-DVCPKG_QT=ON`（如 `cmake -S. -Bbuild -DVCPKG_QT=ON ...`），或在 IDE 中找到相关配置选项。
2. 配置开始后，vcpkg 将编译更多包。编译 Qt 需要较长时间（可能数小时）。
3. 若需切换回系统 Qt，需删除项目根目录下的 `vcpkg.json` 和 `build` 文件夹，然后重新配置。

### macOS 代码签名

`APPLE_CODESIGN_DEV` 选项仅用于本地开发，不适用于分发包。

本地开发的签名与分发包的签名必须不同，因为开发用的授权条款可能不适合生产环境。使用分发包进行本地开发速度慢且操作繁琐。本地开发时，应用包为部分构建，不包含依赖项，使用外部库（如通过 Homebrew 安装），授权条款允许加载这些外部库。

本地开发签名步骤：

1. 安装 Xcode
2. 进入设置 -> 账户
3. 添加账户（需要免费的 Apple Developer ID）
4. 管理证书 -> 添加 -> Apple Development
5. 获取 ID：`security find-identity -v -p codesigning login.keychain-db`
6. 传递 ID 给 CMake：`-DAPPLE_CODESIGN_DEV=Apple Development: bob@example.com (KLGSJHLFXY)`
7. 配置并编译
8. 验证：`codesign -d -r- build/bin/TuPig Synergy.app`

## 编译

配置完成后，运行 make 编译所有目标：

```bash
cmake --build build
```

## 安装

测试安装：
```bash
DESTDIR=<安装目录> cmake --install build
```

正式安装：
```bash
cmake --install build
```

## 打包 TuPig Synergy

TuPig Synergy 可使用 `cpack` 生成多种安装包。

编译 `package` 或 `package_source` 目标即可生成安装包：

```bash
cmake --build build --target package package_source
```

支持的包类型取决于操作系统：
- **归档包**：所有平台通用
- **Linux**：支持 deb、rpm，Flatpak 可从 deploy/linux 生成，Arch Linux 的 PKGBUILD 在 build 目录中生成
- **macOS**：生成并签名 DMG 文件
- **Windows**：可使用 WiX 生成安装程序

[Qt]:https://www.qt.io
[doxygen]:http://www.stack.nl/~dimitri/doxygen/
[cmake]:https://cmake.org/
[openssl]:https://www.openssl.org/
[libei]:https://gitlab.freedesktop.org/libinput/libei
[libportal]:https://github.com/flatpak/libportal
