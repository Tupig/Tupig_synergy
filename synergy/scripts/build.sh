#!/usr/bin/env bash
# TuPig Synergy — Linux/macOS 一键构建脚本
# 用法: ./scripts/build.sh [debug|release]
# 前置条件: cmake >= 3.24, ninja, vcpkg

set -euo pipefail

BUILD_TYPE="${1:-release}"
BUILD_TYPE_LOWER="$(echo "$BUILD_TYPE" | tr '[:upper:]' '[:lower:]')"

# 确保 VCPKG_ROOT 已设置
if [ -z "${VCPKG_ROOT:-}" ]; then
    if [ -d "$HOME/vcpkg" ]; then
        export VCPKG_ROOT="$HOME/vcpkg"
    elif [ -d "/usr/local/share/vcpkg" ]; then
        export VCPKG_ROOT="/usr/local/share/vcpkg"
    else
        echo "[ERROR] VCPKG_ROOT not set and vcpkg not found."
        echo "        Install: git clone https://github.com/microsoft/vcpkg.git && ./vcpkg/bootstrap-vcpkg.sh"
        exit 1
    fi
fi

echo "=== TuPig Synergy Build ==="
echo "VCPKG_ROOT: $VCPKG_ROOT"
echo "Build type: $BUILD_TYPE_LOWER"

# 检查工具
for cmd in cmake ninja; do
    if ! command -v "$cmd" &>/dev/null; then
        echo "[ERROR] $cmd not found. Install it first."
        exit 1
    fi
done

# 配置
echo ""
echo "=== Configuring (cmake --preset $BUILD_TYPE_LOWER) ==="
cmake --preset "$BUILD_TYPE_LOWER"

# 编译
echo ""
echo "=== Building ==="
cmake --build --preset "$BUILD_TYPE_LOWER"

echo ""
echo "=== Build complete ==="
echo "Output: build/bin/"
ls -la build/bin/ 2>/dev/null || true
