@echo off
REM setup.bat
REM One-click environment setup for TuPig Synergy.
REM Installs CMake, MSVC Build Tools, Git via winget, then bootstraps vcpkg.
REM Must be run as Administrator.
REM Pure cmd batch — no PowerShell.

setlocal enabledelayedexpansion

echo.
echo ========================================
echo   TuPig Synergy — Environment Setup
echo ========================================
echo.

REM --- Check Administrator Privileges ---
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] Administrator privileges required.
    echo.
    echo Please right-click this file and select "Run as administrator".
    echo Or open an elevated CMD and run: setup.bat
    echo.
    pause
    exit /b 1
)
echo [OK] Running as Administrator
echo.

REM --- Step 1: Check / Install CMake ---
echo [Step 1/4] Checking CMake...
where cmake >nul 2>&1
if %errorlevel% equ 0 (
    cmake --version | findstr /R "^cmake" >nul
    echo [OK] CMake found
    goto :check_msvc
)
echo CMake not found. Installing via winget...
winget install Kitware.CMake --accept-package-agreements --accept-source-agreements
if %errorlevel% neq 0 (
    echo [ERROR] CMake installation failed (error %errorlevel%)
    echo Please install manually: https://cmake.org/download/
    pause
    exit /b 1
)
REM Refresh PATH
REM Derive from %ProgramFiles% rather than a literal "C:\..." so this still
REM works when the system drive is not C:.
set "PATH=%PATH%;%ProgramFiles%\CMake\bin"
where cmake >nul 2>&1
if %errorlevel% neq 0 (
    echo [WARNING] CMake installed but not in PATH. Please restart terminal.
    echo Manual path: %ProgramFiles%\CMake\bin
)
echo [OK] CMake installed
echo.

:check_msvc
REM --- Step 2: Check / Install MSVC Build Tools ---
echo [Step 2/4] Checking MSVC Build Tools...
where cl >nul 2>&1
if %errorlevel% equ 0 (
    echo [OK] MSVC compiler found
    goto :check_git
)
echo MSVC Build Tools not found. Installing via winget...
echo.
echo IMPORTANT: After winget finishes, the Visual Studio Installer will open.
echo You MUST select the "Desktop development with C++" workload.
echo.
winget install Microsoft.VisualStudio.2022.BuildTools --accept-package-agreements --accept-source-agreements
if %errorlevel% neq 0 (
    echo [ERROR] MSVC Build Tools installation failed (error %errorlevel%)
    echo Please install manually: https://visualstudio.microsoft.com/visual-cpp-build-tools/
    echo Select: "Desktop development with C++" workload
    pause
    exit /b 1
)
echo [OK] MSVC Build Tools installed
echo.
echo NOTE: If the Visual Studio Installer opened, please:
echo   1. Select "Desktop development with C++" workload
echo   2. Click Install
echo   3. Re-run this script after installation completes
echo.
echo.

:check_git
REM --- Step 3: Check / Install Git ---
echo [Step 3/4] Checking Git...
where git >nul 2>&1
if %errorlevel% equ 0 (
    echo [OK] Git found
    goto :bootstrap_vcpkg
)
echo Git not found. Installing via winget...
winget install Git.Git --accept-package-agreements --accept-source-agreements
if %errorlevel% neq 0 (
    echo [ERROR] Git installation failed (error %errorlevel%)
    echo Please install manually: https://git-scm.com/download/win
    pause
    exit /b 1
)
REM Refresh PATH (see the CMake step for why %ProgramFiles% and not a literal path)
set "PATH=%PATH%;%ProgramFiles%\Git\cmd"
echo [OK] Git installed
echo.

:bootstrap_vcpkg
REM --- Step 4: Bootstrap vcpkg ---
echo [Step 4/4] Bootstrapping vcpkg...
call "%~dp0scripts\bootstrap-vcpkg.bat"
if %errorlevel% neq 0 (
    echo [ERROR] vcpkg bootstrap failed
    pause
    exit /b 1
)
echo.

REM --- Done ---
echo ========================================
echo   Environment Setup Complete
echo ========================================
echo.
echo Next steps:
echo   1. cmake --preset windows-msvc-release
echo   2. cmake --build build --config Release
echo.
echo Output: build\bin\Release\synergy.exe        (GUI)
echo         build\bin\Release\synergy-core.exe   (core)
echo         build\bin\Release\synergy-daemon.exe (daemon)
echo.

endlocal
