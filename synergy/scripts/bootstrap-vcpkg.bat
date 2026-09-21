@echo off
REM ============================================================================
REM  scripts\bootstrap-vcpkg.bat
REM
REM  Clones and bootstraps the repository-local vcpkg into vendor\vcpkg.
REM
REM  Self-contained by design:
REM    * no VCPKG_ROOT / no global vcpkg required
REM    * the pinned baseline is read from ..\vcpkg.json (single source of truth)
REM    * idempotent: safe to re-run
REM
REM  Compatible with both `cmd /c` and PowerShell invocation. ASCII-only on
REM  purpose so console code pages cannot corrupt output or parsing.
REM ============================================================================

setlocal enabledelayedexpansion

set "SCRIPT_DIR=%~dp0"
set "REPO_ROOT=%SCRIPT_DIR%.."
set "VCPKG_DIR=%REPO_ROOT%\vendor\vcpkg"
set "MANIFEST=%REPO_ROOT%\vcpkg.json"
set "VCPKG_REPO=https://github.com/microsoft/vcpkg"

echo.
echo === vcpkg bootstrap ===

REM --- locate git -------------------------------------------------------------
where git >nul 2>&1
if errorlevel 1 (
    echo [ERROR] git not found in PATH.
    echo         Install Git for Windows: https://git-scm.com/download/win
    exit /b 1
)

REM --- read pinned baseline from vcpkg.json -----------------------------------
set "BASELINE="
for /f "tokens=2 delims=:," %%a in ('findstr /c:"builtin-baseline" "%MANIFEST%" 2^>nul') do set "BASELINE=%%a"
if defined BASELINE set "BASELINE=%BASELINE: =%"
if defined BASELINE set "BASELINE=%BASELINE:"=%"
if not defined BASELINE (
    echo [ERROR] "builtin-baseline" not found in vcpkg.json
    exit /b 1
)
echo Baseline (from vcpkg.json): %BASELINE%

REM --- clone if absent --------------------------------------------------------
if not exist "%VCPKG_DIR%\.git" (
    echo Cloning vcpkg ^(shallow^) into vendor\vcpkg ...
    git clone --depth 1 "%VCPKG_REPO%" "%VCPKG_DIR%"
    if errorlevel 1 (
        echo [ERROR] git clone failed.
        exit /b 1
    )
)

REM --- make sure the pinned baseline commit is present ------------------------
git -C "%VCPKG_DIR%" rev-parse --verify --quiet "%BASELINE%" >nul 2>&1
if errorlevel 1 (
    echo Fetching pinned baseline commit ...
    git -C "%VCPKG_DIR%" fetch --depth 1 origin "%BASELINE%"
    if errorlevel 1 (
        echo [ERROR] Failed to fetch baseline %BASELINE%
        exit /b 1
    )
)
git -C "%VCPKG_DIR%" checkout --quiet "%BASELINE%"
if errorlevel 1 (
    echo [ERROR] Failed to checkout baseline %BASELINE%
    exit /b 1
)

REM --- bootstrap the vcpkg tool ----------------------------------------------
if not exist "%VCPKG_DIR%\vcpkg.exe" (
    echo Bootstrapping vcpkg tool ...
    pushd "%VCPKG_DIR%"
    call bootstrap-vcpkg.bat -disableMetrics
    set "BOOTSTRAP_RC=!errorlevel!"
    popd
    if not "!BOOTSTRAP_RC!"=="0" (
        echo [ERROR] vcpkg bootstrap failed.
        exit /b 1
    )
)

echo vcpkg ready: %VCPKG_DIR%
echo.

endlocal
exit /b 0
