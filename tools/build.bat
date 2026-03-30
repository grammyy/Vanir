@echo off
setlocal enabledelayedexpansion

set "SCRIPT_DIR=%~dp0"
set "LUA_EXE="

:: ↓ get lua interp ↓ ::
for %%C in (lua54.exe lua5.4.exe luajit.exe lua.exe) do (
    if "!LUA_EXE!"=="" (
        where %%C >nul 2>&1

        if !errorlevel!==0 set "LUA_EXE=%%C"
    )
)

if "!LUA_EXE!"=="" (
    echo [ERROR] No Lua interpreter found in PATH.
    echo   Install Lua 5.4 or LuaJIT and make sure it is on your PATH.

    exit /b 1
)

echo Using Lua: !LUA_EXE!
echo -----------------------------------------

:: ↓ step 1: map_symbols.lua ↓ ::
echo Running map_symbols.lua...

"!LUA_EXE!" "%SCRIPT_DIR%\map_symbols.lua"

if !errorlevel! neq 0 (
    echo [ERROR] map_symbols.lua failed.
    exit /b 1
)

:: ↓ step 2: documentation.lua ↓ ::
echo Running documentation.lua...

"!LUA_EXE!" "%SCRIPT_DIR%\documentation.lua"

if !errorlevel! neq 0 (
    echo [ERROR] documentation.lua failed.
    
    exit /b 1
)

:: ↓ step 3: rename and move docs.html → index.html ↓ ::
set "DOCS_HTML=%SCRIPT_DIR%\docs.html"
set "OUT_HTML=%SCRIPT_DIR%index.html"

if not exist "!DOCS_HTML!" (
    echo [ERROR] Expected output not found: !DOCS_HTML!

    exit /b 1
)

if exist "!OUT_HTML!" del "!OUT_HTML!"

move "!DOCS_HTML!" "!OUT_HTML!" >nul

echo -----------------------------------------
echo Done. Documentation written to: !OUT_HTML!

exit /b 0