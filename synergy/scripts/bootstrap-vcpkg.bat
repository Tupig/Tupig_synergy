@echo off
REM bootstrap-vcpkg.bat
REM Clones vcpkg into vendor/vcpkg and bootstraps it.
REM Idempotent: skips clone if vendor/vcpkg already exists.

setlocal enabledelayedexpansion

set "VCPKG_DIR=%~dp0..\vendor\vcpkg"
set "VCPKG_REPO=https://github.com/microsoft/vcpkg.git"
set "BASELINE=5f96cd15fd745122cf27e0524606d6c1efc5fd07"

echo.
echo === vcpkg Bootstrap ===

REM Check if vcpkg already exists
if exist "%VCPKG_DIR%\bootstrap-vcpkg.bat" (
    echo vcpkg already present at vendor\vcpkg
    goto :checkout
)

REM Clone vcpkg
echo Cloning vcpkg...
git clone --depth 1 %VCPKG_REPO% "%VCPKG_DIR%"
if errorlevel 1 (
    echo [ERROR] Failed to clone vcpkg
    exit /b 1
)

REM Fetch full history for baseline checkout
echo Fetching full history for baseline checkout...
cd /d "%VCPKG_DIR%"
git fetch --depth 1 origin %BASELINE%
if errorlevel 1 (
    echo [ERROR] Failed to fetch baseline %BASELINE%
    exit /b 1
)
git checkout %BASELINE%
if errorlevel 1 (
    echo [ERROR] Failed to checkout baseline %BASELINE%
    exit /b 1
)

:checkout
REM Bootstrap vcpkg
echo Bootstrapping vcpkg...
cd /d "%VCPKG_DIR%"
call bootstrap-vcpkg.bat -disableMetrics
if errorlevel 1 (
    echo [ERROR] vcpkg bootstrap failed
    exit /b 1
)

echo.
echo === vcpkg Ready ===
echo Location: %VCPKG_DIR%
echo Baseline: %BASELINE%

endlocal
