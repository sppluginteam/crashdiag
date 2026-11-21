@echo off
REM =============================================================================
REM crashdiag - one-shot Windows build.
REM
REM   build.bat                  stub SDK build (compiles against sdk\stub\)
REM   build.bat --sdk-path DIR   build against a real HL2SDK (e.g. hl2sdk-dods)
REM   build.bat --clone-dods     clone hl2sdk-dods into deps\ before building
REM
REM Run from a normal shell; CMake locates MSVC automatically.
REM Output lands in build-win64\.
REM =============================================================================
setlocal enabledelayedexpansion

REM %~dp0 always ends with a backslash. A path ending in \" is parsed as an
REM *escaped* quote by cmd, which truncates the argument and produces the very
REM unhelpful error:  ". was unexpected at this time."
REM Strip the trailing backslash before it is ever quoted.
set "SCRIPT_DIR=%~dp0"
if "%SCRIPT_DIR:~-1%"=="\" set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"

pushd "%SCRIPT_DIR%"

set "BUILD_DIR=%SCRIPT_DIR%\build-win64"
set "DEPS_DIR=%SCRIPT_DIR%\deps"
set "SDK_PATH=%HL2SDK_PATH%"
set "CLONE_DODS=0"
set "SKIP_CLONE=0"

:parse
if "%~1"=="" goto :afterparse
if /i "%~1"=="--sdk-path" (set "SDK_PATH=%~2"& shift& shift& goto :parse)
if /i "%~1"=="--clone-dods" (set "CLONE_DODS=1"& shift& goto :parse)
if /i "%~1"=="--no-clone" (set "SKIP_CLONE=1"& shift& goto :parse)
if /i "%~1"=="-h" (echo Usage: build.bat [--sdk-path DIR^|--clone-dods^|--no-clone]& exit /b 0)
echo unknown option: %~1
exit /b 1
:afterparse

if defined SDK_PATH if "%SDK_PATH:~-1%"=="\" set "SDK_PATH=%SDK_PATH:~0,-1%"

where cmake >nul 2>nul || (echo error: cmake is required but not installed. & exit /b 1)
where git   >nul 2>nul || (echo error: git is required but not installed. & exit /b 1)

if not defined SDK_PATH if "%CLONE_DODS%"=="1" call :clone_dods

if defined SDK_PATH goto :realbuild
echo ==^> stub SDK build ^(sdk\stub\^)
set "STUB=ON"
set "EXTRA=-DCRASHDIAG_STUB_SDK=ON"
goto :configure

:realbuild
if not exist "%SDK_PATH%\public\eiface.h" (
    echo error: %SDK_PATH% does not look like an HL2SDK ^(missing public\eiface.h^).
    exit /b 1
)
echo ==^> real HL2SDK build: %SDK_PATH%
set "STUB=OFF"
set "EXTRA=-DCRASHDIAG_STUB_SDK=OFF -DHL2SDK_PATH=%SDK_PATH%"
goto :configure

REM --- Real SDK resolution (only reached with --clone-dods) ---------------------
:clone_dods
set "SDK_PATH=%DEPS_DIR%\hl2sdk-dods"
if exist "%SDK_PATH%\.git" (
    echo ==^> reusing existing %SDK_PATH%
    exit /b 0
)
if "%SKIP_CLONE%"=="1" (
    echo error: SDK not found at %SDK_PATH% and --no-clone was given.
    exit /b 1
)
echo ==^> cloning alliedmodders/hl2sdk ^(branch dods^) -^> %SDK_PATH%
git clone --depth 1 --branch dods https://github.com/alliedmodders/hl2sdk.git "%SDK_PATH%"
exit /b %errorlevel%

REM --- Configure & build --------------------------------------------------------
:configure
echo ==^> configuring -^> %BUILD_DIR%
cmake -S "%SCRIPT_DIR%" -B "%BUILD_DIR%" -A x64 -DCMAKE_BUILD_TYPE=Release %EXTRA%
if errorlevel 1 exit /b 1

echo ==^> building
cmake --build "%BUILD_DIR%" --config Release
if errorlevel 1 exit /b 1

echo.
echo ==^> build complete. Artifacts:
dir /s /b "%BUILD_DIR%\*.dll" 2>nul || verify >nul

popd
endlocal
exit /b 0
