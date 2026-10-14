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

# --- fetch the pinned baseline commit ---------------------------------------
if [ ! -d "${VCPKG_DIR}/.git" ]; then
    echo "Fetching vcpkg at the pinned baseline into vendor/vcpkg ..."
    # A shallow clone of the default branch failed deterministically on
    # GitHub-hosted runners (git exited 0 while the pack's tree objects were
    # missing, so the later checkout died with "unable to read tree").
    # Fetching the baseline commit directly is deterministic, skips unrelated
    # master history, and resumes naturally when a partial transfer left the
    # repository behind (re-running goes straight to the repair below).
    rm -rf "${VCPKG_DIR}"
    mkdir -p "${VCPKG_DIR}"
    git -c init.defaultBranch=master -C "${VCPKG_DIR}" init -q
    git -C "${VCPKG_DIR}" remote add origin "${VCPKG_REPO}"
    git -C "${VCPKG_DIR}" fetch --depth 1 origin "${BASELINE}"
    git -C "${VCPKG_DIR}" checkout -q FETCH_HEAD
fi

# --- make sure the pinned baseline commit and its tree are present ----------
if ! git -C "${VCPKG_DIR}" cat-file -e "${BASELINE}^{tree}" 2>/dev/null; then
    echo "Fetching pinned baseline commit ..."
    git -C "${VCPKG_DIR}" fetch --depth 1 origin "${BASELINE}"
    git -C "${VCPKG_DIR}" checkout --quiet "${BASELINE}"
fi

# --- bootstrap the vcpkg tool ---------------------------------------------
if [ ! -x "${VCPKG_DIR}/vcpkg" ]; then
    echo "Bootstrapping vcpkg tool ..."
    ( cd "${VCPKG_DIR}" && ./bootstrap-vcpkg.sh -disableMetrics )
fi

echo "vcpkg ready: ${VCPKG_DIR}"
echo
