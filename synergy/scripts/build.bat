@echo off
REM ============================================================================
REM  scripts\build.bat
REM
REM  One-command Windows build for TuPig Synergy.
REM
REM  Usage:  scripts\build.bat [release|debug]        (default: release)
REM
REM  Self-contained by design:
REM    * bootstraps the repository-local vcpkg (no VCPKG_ROOT, no global vcpkg)
REM    * locates Visual Studio through vswhere (no hardcoded install paths)
REM    * presets come from CMakePresets.json
REM
REM  Compatible with both `cmd /c` and PowerShell invocation. ASCII-only on
REM  purpose so console code pages cannot corrupt output or parsing.
REM ============================================================================

setlocal enabledelayedexpansion

set "SCRIPT_DIR=%~dp0"
set "REPO_ROOT=%SCRIPT_DIR%.."

REM --- build type -------------------------------------------------------------
set "BUILD_TYPE=%~1"
if "%BUILD_TYPE%"=="" set "BUILD_TYPE=release"
if /i "%BUILD_TYPE%"=="release" set "BUILD_TYPE=release"
if /i "%BUILD_TYPE%"=="debug"   set "BUILD_TYPE=debug"
if /i not "%BUILD_TYPE%"=="release" if /i not "%BUILD_TYPE%"=="debug" (
    echo [ERROR] Unknown build type: %~1
    echo         Usage: scripts\build.bat [release^|debug]
    exit /b 1
)

set "PRESET=windows-msvc-%BUILD_TYPE%"

echo.
echo ============================================
echo   TuPig Synergy - Windows build
echo   Preset: %PRESET%
echo ============================================
echo.

REM --- required host tools ----------------------------------------------------
where git >nul 2>&1
if errorlevel 1 (
    echo [ERROR] git not found in PATH.
    echo         Install Git for Windows, then re-run.
    exit /b 1
)

where cmake >nul 2>&1
if errorlevel 1 (
    echo [ERROR] cmake not found in PATH.
    echo         Run setup.bat once to install the toolchain, or install CMake 3.24+.
    exit /b 1
)

REM --- locate Visual Studio ---------------------------------------------------
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo [ERROR] vswhere.exe not found.
    echo         Visual Studio 2022 Build Tools are required.
    echo         Run setup.bat, or install "Desktop development with C++".
    exit /b 1
)

set "VS_PATH="
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VS_PATH=%%i"
if not defined VS_PATH (
    echo [ERROR] Visual Studio C++ toolset not found.
    echo         Install the "Desktop development with C++" workload, then re-run.
    exit /b 1
)
echo Visual Studio: %VS_PATH%

REM --- activate the MSVC environment ------------------------------------------
REM Required: vcpkg resolves its Visual Studio instance from the VC environment
REM variables (VCINSTALLDIR / VCToolsInstallDir). Without them vcpkg fails with
REM "Could not locate a complete Visual Studio instance" on Build Tools
REM installations whose instance metadata lacks an isComplete flag.
set "VCVARSALL=%VS_PATH%\VC\Auxiliary\Build\vcvarsall.bat"
if not exist "%VCVARSALL%" (
    echo [ERROR] vcvarsall.bat not found under %VS_PATH%
    exit /b 1
)
echo Activating MSVC x64 environment ...
call "%VCVARSALL%" x64 >nul
if errorlevel 1 (
    echo [ERROR] Failed to activate the MSVC environment.
    exit /b 1
)

REM --- bootstrap vcpkg (repository-local) -------------------------------------
call "%SCRIPT_DIR%bootstrap-vcpkg.bat"
if errorlevel 1 (
    echo [ERROR] vcpkg bootstrap failed.
    exit /b 1
)

pushd "%REPO_ROOT%"

REM --- configure (drives vcpkg manifest install) ------------------------------
echo.
echo === Configure ===
cmake --preset "%PRESET%"
if errorlevel 1 (
    echo [ERROR] CMake configure failed.
    popd
    exit /b 1
)

REM --- build ------------------------------------------------------------------
echo.
echo === Build ===
cmake --build --preset "%PRESET%"
if errorlevel 1 (
    echo [ERROR] Build failed.
    popd
    exit /b 1
)

popd

echo.
echo === Build complete ===
if /i "%BUILD_TYPE%"=="release" (
    echo Output directory: build\bin\Release
) else (
    echo Output directory: build\bin\Debug
)
echo.

endlocal
exit /b 0
