@echo off
REM ============================================================================
REM  scripts\ci-windows-signing.bat
REM
REM  Windows-only CI helpers for signing the release artifacts.
REM
REM  Why this is a .bat rather than inline PowerShell in the workflow: AGENTS.md
REM  R1 keeps Windows automation to a single scripting runtime, and the rest of
REM  this repository is cmd. Putting the logic in a file also means it can be run
REM  and reviewed locally, which inline YAML cannot.
REM
REM  Usage:  scripts\ci-windows-signing.bat <mode>
REM
REM    stage        Copy build\bin\*.exe (excluding tests) into the staging dir
REM                 so they can be batch-signed with a single OTP.
REM    restore      Copy the signed binaries back over build\bin and verify each
REM                 signature is valid.
REM    locate-msi   Find the built .msi and append "path=<file>" to GITHUB_OUTPUT.
REM    verify-msi   Verify the MSI signature given by %MSI_PATH%.
REM
REM  ASCII-only on purpose, so console code pages cannot corrupt parsing.
REM ============================================================================

setlocal enabledelayedexpansion

REM --- directories -------------------------------------------------------------
REM STAGE_ROOT is overridable so this can be exercised without a CI runner.
if "%STAGE_ROOT%"=="" set "STAGE_ROOT=%RUNNER_TEMP%\sign-stage"
set "STAGE_IN=%STAGE_ROOT%\in"
set "STAGE_OUT=%STAGE_ROOT%\out"
set "BIN_DIR=build\bin"

if "%~1"=="" goto :usage

if /i "%~1"=="stage"       goto :stage
if /i "%~1"=="restore"     goto :restore
if /i "%~1"=="locate-msi"  goto :locate_msi
if /i "%~1"=="verify-msi"  goto :verify_msi

echo [ERROR] Unknown mode: %~1
goto :usage

REM ============================================================================
:stage
REM Excludes test executables: they are not shipped and must not consume signing
REM quota. A missing or empty BIN_DIR is an error, not a silent no-op, because
REM the release would otherwise ship unsigned binaries.
if not exist "%BIN_DIR%" (
    echo [ERROR] %BIN_DIR% does not exist; nothing to sign.
    exit /b 1
)

if not exist "%STAGE_IN%"  mkdir "%STAGE_IN%"
if not exist "%STAGE_OUT%" mkdir "%STAGE_OUT%"

set "STAGED=0"
for %%F in ("%BIN_DIR%\*.exe") do (
    set "NAME=%%~nxF"
    echo !NAME! | findstr /i "test" >nul
    if errorlevel 1 (
        copy /y "%%~fF" "%STAGE_IN%\!NAME!" >nul
        if errorlevel 1 (
            echo [ERROR] could not stage %%F
            exit /b 1
        )
        set /a STAGED+=1
    )
)

if !STAGED! EQU 0 (
    echo [ERROR] no non-test executables found in %BIN_DIR% to sign.
    exit /b 1
)

echo [OK] staged !STAGED! executable^(s^) in "%STAGE_IN%"
exit /b 0

REM ============================================================================
:restore
if not exist "%STAGE_OUT%" (
    echo [ERROR] %STAGE_OUT% does not exist; the signing step produced nothing.
    exit /b 1
)

REM Check for signed output before demanding signtool: "nothing was signed" is the
REM more actionable failure, and it is also the one that can happen on a runner
REM without the Windows SDK.
set "PENDING=0"
for %%F in ("%STAGE_OUT%\*.exe") do set /a PENDING+=1
if !PENDING! EQU 0 (
    echo [ERROR] no signed executables found in "%STAGE_OUT%".
    exit /b 1
)

call :find_signtool
if errorlevel 1 exit /b 1

set "RESTORED=0"
for %%F in ("%STAGE_OUT%\*.exe") do (
    set "NAME=%%~nxF"
    copy /y "%%~fF" "%BIN_DIR%\!NAME!" >nul
    if errorlevel 1 (
        echo [ERROR] could not restore !NAME! into %BIN_DIR%
        exit /b 1
    )

    REM /pa = use the default authentication verification policy, /q = quiet.
    "!SIGNTOOL!" verify /pa /q "%BIN_DIR%\!NAME!"
    if errorlevel 1 (
        echo [ERROR] signature check failed on %BIN_DIR%\!NAME!
        exit /b 1
    )
    echo [OK] !NAME! is validly signed
    set /a RESTORED+=1
)

echo [OK] restored and verified !RESTORED! executable^(s^)
exit /b 0

REM ============================================================================
:locate_msi
set "MSI="
for %%F in ("build\*.msi") do (
    if not defined MSI set "MSI=%%~fF"
)

if not defined MSI (
    echo [ERROR] no MSI found in build\
    exit /b 1
)

if not defined GITHUB_OUTPUT (
    echo [ERROR] GITHUB_OUTPUT is not set; run this from a workflow step.
    exit /b 1
)

echo path=!MSI!>>"%GITHUB_OUTPUT%"
echo [OK] located !MSI!
exit /b 0

REM ============================================================================
:verify_msi
if not defined MSI_PATH (
    echo [ERROR] MSI_PATH is not set.
    exit /b 1
)

call :find_signtool
if errorlevel 1 exit /b 1

"!SIGNTOOL!" verify /pa /q "%MSI_PATH%"
if errorlevel 1 (
    echo [ERROR] MSI is not validly signed: %MSI_PATH%
    exit /b 1
)

echo [OK] MSI is validly signed: %MSI_PATH%
exit /b 0

REM ============================================================================
:find_signtool
REM Replaces PowerShell's Get-AuthenticodeSignature, which needs no external tool.
REM signtool ships with the Windows SDK; it is on PATH in a developer prompt but
REM not necessarily on a runner, so fall back to the standard Kits location, the
REM same "probe, then literal last resort" shape build.bat uses for vswhere.
set "SIGNTOOL="
for %%S in (signtool.exe) do if not defined SIGNTOOL if not "%%~$PATH:S"=="" set "SIGNTOOL=%%~$PATH:S"

if not defined SIGNTOOL (
    for /d %%D in ("%ProgramFiles(x86)%\Windows Kits\10\bin\*") do (
        if not defined SIGNTOOL if exist "%%~fD\x64\signtool.exe" set "SIGNTOOL=%%~fD\x64\signtool.exe"
    )
)

if not defined SIGNTOOL (
    echo [ERROR] signtool.exe not found.
    echo         It ships with the Windows SDK; install it or add it to PATH.
    exit /b 1
)

exit /b 0

REM ============================================================================
:usage
echo Usage: scripts\ci-windows-signing.bat ^<stage^|restore^|locate-msi^|verify-msi^>
exit /b 1
