#!/usr/bin/env bash
# ============================================================================
#  scripts/bootstrap-vcpkg.sh
#
#  Clones and bootstraps the repository-local vcpkg into vendor/vcpkg.
#
#  Self-contained by design:
#    * no VCPKG_ROOT / no global vcpkg required
#    * the pinned baseline is read from ../vcpkg.json (single source of truth)
#    * idempotent: safe to re-run
#
#  Intended for Linux and macOS. ASCII-only messages on purpose.
# ============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
VCPKG_DIR="${REPO_ROOT}/vendor/vcpkg"
MANIFEST="${REPO_ROOT}/vcpkg.json"
VCPKG_REPO="https://github.com/microsoft/vcpkg"

echo
echo "=== vcpkg bootstrap ==="

command -v git >/dev/null 2>&1 || {
    echo "[ERROR] git not found in PATH."
    exit 1
}

# --- read pinned baseline from vcpkg.json ----------------------------------
BASELINE="$(sed -n 's/.*"builtin-baseline"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' "${MANIFEST}" | head -n 1)"
if [ -z "${BASELINE}" ]; then
    echo "[ERROR] \"builtin-baseline\" not found in vcpkg.json"
    exit 1
fi
echo "Baseline (from vcpkg.json): ${BASELINE}"

# --- clone if absent -------------------------------------------------------
if [ ! -d "${VCPKG_DIR}/.git" ]; then
    echo "Cloning vcpkg (shallow) into vendor/vcpkg ..."
    git clone --depth 1 "${VCPKG_REPO}" "${VCPKG_DIR}"
fi

# --- make sure the pinned baseline commit is present ----------------------
if ! git -C "${VCPKG_DIR}" rev-parse --verify --quiet "${BASELINE}" >/dev/null; then
    echo "Fetching pinned baseline commit ..."
    git -C "${VCPKG_DIR}" fetch --depth 1 origin "${BASELINE}"
fi
git -C "${VCPKG_DIR}" checkout --quiet "${BASELINE}"

# --- bootstrap the vcpkg tool ---------------------------------------------
if [ ! -x "${VCPKG_DIR}/vcpkg" ]; then
    echo "Bootstrapping vcpkg tool ..."
    ( cd "${VCPKG_DIR}" && ./bootstrap-vcpkg.sh -disableMetrics )
fi

echo "vcpkg ready: ${VCPKG_DIR}"
echo
