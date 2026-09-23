#!/usr/bin/env bash
# ============================================================================
#  scripts/build.sh
#
#  One-command build for Linux and macOS.
#
#  Usage:  ./scripts/build.sh [release]              (default: release)
#
#  Only Release is offered. Every vcpkg overlay triplet in this repository sets
#  VCPKG_BUILD_TYPE release (see triplets/overlay), so a Debug build configures
#  and compiles but cannot link the dependencies - offering it here would only
#  produce a confusing link failure. For an instrumented or non-optimised build
#  use the diagnostics presets (linux-asan / linux-tsan / linux-coverage) via
#  cmake --preset directly.
#
#  Self-contained by design:
#    * bootstraps the repository-local vcpkg (no VCPKG_ROOT, no global vcpkg)
#    * picks the platform preset from CMakePresets.json automatically
#    * no environment variables need to be set by hand
#
#  ASCII-only messages on purpose.
# ============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# --- build type -------------------------------------------------------------
BUILD_TYPE="$(printf '%s' "${1:-release}" | tr '[:upper:]' '[:lower:]')"
case "${BUILD_TYPE}" in
    release) ;;
    debug)
        echo "[ERROR] A Debug build cannot link this project's dependencies."
        echo "        Every overlay triplet sets VCPKG_BUILD_TYPE release, so only"
        echo "        Release dependencies are available."
        echo "        Use 'release', or a diagnostics preset such as linux-asan."
        exit 1
        ;;
    *)
        echo "[ERROR] Unknown build type: ${1:-}"
        echo "        Usage: scripts/build.sh [release]"
        exit 1
        ;;
esac

# --- platform preset --------------------------------------------------------
case "$(uname -s)" in
    Linux*)
        PLATFORM="linux"
        ;;
    Darwin*)
        PLATFORM="macos"
        ;;
    *)
        echo "[ERROR] Unsupported platform: $(uname -s)"
        echo "        Use scripts\\build.bat on Windows."
        exit 1
        ;;
esac

PRESET="${PLATFORM}-${BUILD_TYPE}"

echo
echo "============================================"
echo "  TuPig Synergy - build"
echo "  Preset: ${PRESET}"
echo "============================================"
echo

# --- required host tools ----------------------------------------------------
for tool in git cmake ninja; do
    if ! command -v "${tool}" >/dev/null 2>&1; then
        echo "[ERROR] ${tool} not found in PATH."
        case "${tool}" in
            ninja)
                echo "        Debian/Ubuntu: sudo apt install ninja-build"
                echo "        macOS:         brew install ninja"
                ;;
            cmake)
                echo "        Install CMake 3.25+ and re-run."
                ;;
        esac
        exit 1
    fi
done

if [ "${PLATFORM}" = "macos" ] && ! xcode-select -p >/dev/null 2>&1; then
    echo "[ERROR] Xcode Command Line Tools not found. Run: xcode-select --install"
    exit 1
fi

# --- bootstrap vcpkg (repository-local) -------------------------------------
"${SCRIPT_DIR}/bootstrap-vcpkg.sh"

cd "${REPO_ROOT}"

# --- configure (drives vcpkg manifest install) ------------------------------
echo
echo "=== Configure ==="
cmake --preset "${PRESET}"

# --- build ------------------------------------------------------------------
echo
echo "=== Build ==="
cmake --build --preset "${PRESET}"

echo
echo "=== Build complete ==="
echo "Output directory: build/bin"
echo
