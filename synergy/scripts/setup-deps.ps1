<#
.SYNOPSIS
    TuPig Synergy 依赖自动安装脚本
.DESCRIPTION
    自动安装并缓存 Qt6 + OpenSSL 依赖到仓库本地目录，
    实现克隆即构建，无需额外配置。
.NOTES
    以管理员权限运行 PowerShell，执行此脚本即可完成所有依赖准备。
#>

param(
    [string]$VcpkgRoot = "",
    [string]$Triplet = "x64-windows",
    [switch]$Force = $false
)

$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path -Parent $PSScriptRoot
$CacheDir = Join-Path $RepoRoot "deps\vcpkg-cache"

# 自动检测 vcpkg
if (-not $VcpkgRoot) {
    $VcpkgRoot = $env:VCPKG_ROOT
    if (-not $VcpkgRoot) {
        # 尝试常见路径
        $candidates = @(
            "$env:USERPROFILE\vcpkg",
            "$env:LOCALAPPDATA\vcpkg",
            "C:\vcpkg",
            "D:\vcpkg"
        )
        foreach ($c in $candidates) {
            if (Test-Path "$c\vcpkg.exe") {
                $VcpkgRoot = $c
                break
            }
        }
        if (-not $VcpkgRoot) {
            Write-Host "❌ 未找到 vcpkg，请设置 VCPKG_ROOT 环境变量" -ForegroundColor Red
            Write-Host "   或安装 vcpkg: git clone https://github.com/microsoft/vcpkg.git" -ForegroundColor Yellow
            exit 1
        }
    }
}

Write-Host "🔧 TuPig Synergy 依赖安装" -ForegroundColor Cyan
Write-Host "   vcpkg 路径: $VcpkgRoot" -ForegroundColor Gray
Write-Host "   缓存目录:  $CacheDir" -ForegroundColor Gray
Write-Host "   目标架构:  $Triplet" -ForegroundColor Gray

# 创建缓存目录
if (-not (Test-Path $CacheDir)) {
    New-Item -ItemType Directory -Path $CacheDir -Force | Out-Null
    Write-Host "📁 创建缓存目录: $CacheDir" -ForegroundColor Green
}

# 设置环境变量
$env:VCPKG_DEFAULT_BINARY_CACHE = $CacheDir

# 引导 vcpkg
$vcpkgExe = Join-Path $VcpkgRoot "vcpkg.exe"
if (-not (Test-Path $vcpkgExe)) {
    Write-Host "📦 引导 vcpkg..." -ForegroundColor Yellow
    Push-Location $VcpkgRoot
    & .\bootstrap-vcpkg.bat
    Pop-Location
}

# 安装依赖
Write-Host "📦 安装 Qt6 + OpenSSL 依赖 (这可能需要几分钟)..." -ForegroundColor Yellow
& $vcpkgExe install qtbase openssl --triplet $Triplet

if ($LASTEXITCODE -ne 0) {
    Write-Host "❌ 依赖安装失败" -ForegroundColor Red
    exit 1
}

# 集成到 Visual Studio
Write-Host "🔗 集成 vcpkg 到 Visual Studio..." -ForegroundColor Yellow
& $vcpkgExe integrate install

Write-Host ""
Write-Host "✅ 依赖安装完成！" -ForegroundColor Green
Write-Host ""
Write-Host "现在可以构建项目:" -ForegroundColor Cyan
Write-Host "   cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release" -ForegroundColor White
Write-Host "   cmake --build build" -ForegroundColor White
