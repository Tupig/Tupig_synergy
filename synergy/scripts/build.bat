@echo off
REM TuPig Synergy — Windows 一键构建脚本
REM 用法: scripts\build.bat [debug|release]
REM 前置条件: Visual Studio Build Tools + Ninja + vcpkg

setlocal enabledelayedexpansion

set BUILD_TYPE=%1
if "%BUILD_TYPE%"=="" set BUILD_TYPE=release

REM 确保 VCPKG_ROOT 已设置
if "%VCPKG_ROOT%"=="" (
    if exist "C:\Users\%USERNAME%\vcpkg" (
        set VCPKG_ROOT=C:\Users\%USERNAME%\vcpkg
    ) else (
        echo [ERROR] VCPKG_ROOT not set and vcpkg not found at C:\Users\%USERNAME%\vcpkg
        echo         Install vcpkg: git clone https://github.com/microsoft/vcpkg.git
        exit /b 1
    )
)

echo === TuPig Synergy Build ===
echo VCPKG_ROOT: %VCPKG_ROOT%
echo Build type: %BUILD_TYPE%

REM 激活 MSVC 环境
set VCVARSALL="C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat"
if not exist %VCVARSALL% (
    set VCVARSALL="C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"
)
if not exist %VCVARSALL% (
    set VCVARSALL="C:\Program Files (x86)\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"
)

if not exist %VCVARSALL% (
    echo [ERROR] vcvarsall.bat not found. Install Visual Studio Build Tools.
    exit /b 1
)

echo Activating MSVC environment...
call %VCVARSALL% x64 >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Failed to activate MSVC environment
    exit /b 1
)

REM 配置
echo.
echo === Configuring (cmake --preset %BUILD_TYPE%) ===
cmake --preset %BUILD_TYPE%
if errorlevel 1 (
    echo [ERROR] CMake configure failed
    exit /b 1
)

REM 编译
echo.
echo === Building ===
cmake --build --preset %BUILD_TYPE%
if errorlevel 1 (
    echo [ERROR] Build failed
    exit /b 1
)

echo.
echo === Build complete ===
echo Output: build\bin\
dir /b build\bin\*.exe 2>nul
