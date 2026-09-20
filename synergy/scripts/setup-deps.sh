#!/usr/bin/env bash
# TuPig Synergy 依赖自动安装脚本 (Linux/macOS)
# 自动安装并缓存 Qt6 + OpenSSL 依赖到仓库本地目录

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
CACHE_DIR="$REPO_ROOT/deps/vcpkg-cache"

# 检测平台和架构
detect_platform() {
    local os arch
    
    case "$(uname -s)" in
        Linux*)     os="linux" ;;
        Darwin*)    os="macos" ;;
        MINGW*|MSYS*|CYGWIN*)  os="windows" ;;
        *)          echo "❌ 不支持的操作系统: $(uname -s)"; exit 1 ;;
    esac
    
    case "$(uname -m)" in
        x86_64|amd64)   arch="x64" ;;
        aarch64|arm64)   arch="arm64" ;;
        *)              echo "❌ 不支持的架构: $(uname -m)"; exit 1 ;;
    esac
    
    echo "${arch}-${os}"
}

# 安装系统依赖 (Linux)
install_linux_deps() {
    if command -v apt-get &> /dev/null; then
        echo "📦 使用 apt 安装系统依赖..."
        sudo apt-get update
        sudo apt-get install -y cmake ninja-build g++ \
            qt6-base-dev libssl-dev \
            libx11-dev libxi-dev libxtst-dev libxinerama-dev \
            libxrandr-dev libxkbcommon-dev libglib2.0-dev
    elif command -v dnf &> /dev/null; then
        echo "📦 使用 dnf 安装系统依赖..."
        sudo dnf install -y cmake ninja-build gcc-c++ \
            qt6-qtbase-devel openssl-devel \
            libX11-devel libXi-devel libXtst-devel \
            libXinerama-devel libXrandr-devel libxkbcommon-devel glib2-devel
    elif command -v pacman &> /dev/null; then
        echo "📦 使用 pacman 安装系统依赖..."
        sudo pacman -S --needed cmake ninja gcc qt6-base openssl \
            libx11 libxi libxtst libxinerama libxrandr libxkbcommon glib2
    else
        echo "⚠️  未识别的包管理器，请手动安装依赖"
        exit 1
    fi
}

# 安装系统依赖 (macOS)
install_macos_deps() {
    if command -v brew &> /dev/null; then
        echo "📦 使用 Homebrew 安装系统依赖..."
        brew install cmake ninja qt@6 openssl@3
    else
        echo "❌ 请先安装 Homebrew: https://brew.sh"
        exit 1
    fi
}

# 主流程
main() {
    echo "🔧 TuPig Synergy 依赖安装"
    
    PLATFORM=$(detect_platform)
    echo "   平台: $PLATFORM"
    echo "   缓存: $CACHE_DIR"
    
    # 创建缓存目录
    mkdir -p "$CACHE_DIR"
    export VCPKG_DEFAULT_BINARY_CACHE="$CACHE_DIR"
    
    # 检查是否已安装
    if [ -d "$CACHE_DIR/$PLATFORM" ] && [ "$(ls -A "$CACHE_DIR/$PLATFORM" 2>/dev/null)" ]; then
        echo "✅ 依赖已存在，跳过安装"
    else
        case "$PLATFORM" in
            x64-linux|arm64-linux)
                install_linux_deps
                ;;
            x64-macos|arm64-macos)
                install_macos_deps
                ;;
        esac
    fi
    
    echo ""
    echo "✅ 依赖准备完成！"
    echo ""
    echo "现在可以构建项目:"
    echo "   cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release"
    echo "   cmake --build build"
}

main "$@"
