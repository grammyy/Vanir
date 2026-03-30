@echo off
setlocal enabledelayedexpansion

set "SCRIPT_DIR=%~dp0"
set "TEST_DIR=%SCRIPT_DIR%tests"
set "DLL=%SCRIPT_DIR%Vanir.dll"

:: ↓ usage ↓
set "SHOW_HELP=0"
set "FORCE_LUAJIT=0"
set "FORCE_LUA54=0"

for %%A in (%*) do (
    if "%%A"=="--luajit" set "FORCE_LUAJIT=1"
    if "%%A"=="--lua54"  set "FORCE_LUA54=1"
    if "%%A"=="--help"   set "SHOW_HELP=1"
    if "%%A"=="-h"       set "SHOW_HELP=1"
)

if !SHOW_HELP!==1 (
    echo Usage: run-tests.bat [--luajit ^| --lua54]
    echo.
    echo Options:
    echo   --luajit    Force LuaJIT ^(luajit.exe^)
    echo   --lua54     Force Lua 5.4 ^(lua54.exe / lua5.4.exe^)
    echo   ^(default^)   Auto-detect: prefers lua54.exe, then luajit.exe, then lua.exe
    
    exit /b 0
)

if !FORCE_LUAJIT!==1 if !FORCE_LUA54!==1 (
    echo [ERROR] Cannot specify both --luajit and --lua54.
    
    exit /b 1
)

set "LUA_EXE="

:: ↓ get lua executable ↓
if !FORCE_LUAJIT!==1 (
    where luajit.exe >nul 2>&1
    
    if !errorlevel!==0 (
        set "LUA_EXE=luajit.exe"
    ) else (
        echo [ERROR] --luajit specified but luajit.exe not found in PATH.
        
        exit /b 1
    )
    
    goto :exe_found
)

if !FORCE_LUA54!==1 (
    for %%C in (lua54.exe lua5.4.exe) do (
        if "!LUA_EXE!"=="" (
            where %%C >nul 2>&1
            
            if !errorlevel!==0 set "LUA_EXE=%%C"
        )
    )
    
    if "!LUA_EXE!"=="" (
        echo [ERROR] --lua54 specified but lua54.exe / lua5.4.exe not found in PATH.
        
        exit /b 1
    )
    
    goto :exe_found
)

:: ↓ prefer lua54, then luajit, then default lua ↓
for %%C in (lua54.exe lua5.4.exe luajit.exe lua.exe) do (
    if "!LUA_EXE!"=="" (
        where %%C >nul 2>&1
        
        if !errorlevel!==0 set "LUA_EXE=%%C"
    )
)

if "!LUA_EXE!"=="" (
    echo [ERROR] No Lua executable found in PATH.
    echo   Tried: lua54.exe, lua5.4.exe, luajit.exe, lua.exe
    echo   Pass --luajit or --lua54, or add a Lua runtime to PATH.
    
    exit /b 1
)

:exe_found

echo Lua executable : !LUA_EXE!

:: ↓ checks ↓
if not exist "%TEST_DIR%\" (
    echo [WARN] No tests\ folder found, skipping.
    
    exit /b 0
)

if not exist "%DLL%" (
    echo [ERROR] Vanir.dll not found at %DLL%
    echo   Run: build.bat --platform windows
    
    exit /b 1
)

:: ↓ escape backslashes in SCRIPT_DIR for use inside Lua strings ↓
set "CPATH_DIR=%SCRIPT_DIR%"
set "CPATH_DIR=!CPATH_DIR:\=\\!"

set /a PASSED=0
set /a FAILED=0
set "FAILED_NAMES="

echo.
echo Running tests in %TEST_DIR%
echo -----------------------------------------

:: ↓ do not quote the glob pattern ↓
for %%F in (%TEST_DIR%\*.lua) do (
    set "NAME=%%~nxF"
    
    "!LUA_EXE!" -e "package.cpath='!CPATH_DIR!?.dll;'..package.cpath" "%%F" >"%TEMP%\lua_out.txt" 2>&1

    findstr /C:"PASS" "%TEMP%\lua_out.txt" >nul
    
    if !errorlevel!==0 (
        echo   PASS  !NAME!
        
        set /a PASSED+=1
    ) else (
        echo   FAIL  !NAME!
        
        for /f "usebackq delims=" %%L in ("%TEMP%\lua_out.txt") do echo         %%L
        
        set /a FAILED+=1
        set "FAILED_NAMES=!FAILED_NAMES! !NAME!"
    )
)

echo -----------------------------------------
echo Tests: !PASSED! passed, !FAILED! failed

if !FAILED! neq 0 (
    echo Failed:!FAILED_NAMES!

    exit /b 1
)

exit /b 0