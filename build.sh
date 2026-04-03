#!/bin/bash
set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# ── Usage ──────────────────────────────────────────────────────────────────
usage() {
  echo "Usage: $0 --platform <linux|windows> [options]"
  echo ""
  echo "Options:"
  echo "  --platform <linux|windows>   Target platform (required)"
  echo "  --luajit                     Use LuaJIT instead of Lua 5.4"
  echo "  --lua54                      Use Lua 5.4 (default)"
  echo "  --verbose                    Enable vanir_log / vanir_log_info output"
  echo "  --test                       Run tests after building"
  exit 1
}

# ── Args ───────────────────────────────────────────────────────────────────
PLATFORM=""
LUAJIT=OFF
RUN_TESTS=OFF
VERBOSE=OFF

for arg in "$@"; do
  case $arg in
    --platform=*) PLATFORM="${arg#*=}" ;;
    --platform)   ;;  # handled below via shift-style — see note
    --luajit)     LUAJIT=ON ;;
    --lua54)      LUAJIT=OFF ;;
    --verbose)    VERBOSE=ON ;;
    --test)       RUN_TESTS=ON ;;
    --help|-h)    usage ;;
    *)
      # allow "--platform linux" (two-arg form)
      if [ "$PREV" = "--platform" ]; then
        PLATFORM="$arg"
      fi
      ;;
  esac
  PREV="$arg"
done

if [ -z "$PLATFORM" ]; then
  echo "Error: --platform is required."
  echo ""
  usage
fi

if [ "$PLATFORM" != "linux" ] && [ "$PLATFORM" != "windows" ]; then
  echo "Error: unknown platform '$PLATFORM'. Use 'linux' or 'windows'."
  exit 1
fi

echo "Building for $PLATFORM (LuaJIT=$LUAJIT, verbose=$VERBOSE)..."

# ── GLFW / WebGPU paths — edit these to match your machine ────────────────
if [ "$PLATFORM" = "linux" ]; then
  GLFW_INC="/mnt/c/Users/Elias/Documents/languages/glfw/linux/include"
  GLFW_LIB="/mnt/c/Users/Elias/Documents/languages/glfw/linux/lib/libglfw.so"
  WEBGPU_INC="/mnt/c/Users/Elias/Documents/languages/webgpu/linux/include"
  WEBGPU_LIB="/mnt/c/Users/Elias/Documents/languages/webgpu/linux/lib/libwgpu_native.so"
  FREETYPE_INC="/usr/include/freetype2"
  FREETYPE_LIB="/usr/lib/x86_64-linux-gnu/libfreetype.so"
else
  GLFW_INC="/mnt/c/Users/Elias/Documents/languages/glfw/windows/include"
  GLFW_LIB="/mnt/c/Users/Elias/Documents/languages/glfw/windows/lib-mingw-w64/libglfw3.a"
  WEBGPU_INC="/mnt/c/Users/Elias/Documents/languages/webgpu/windows/include"
  WEBGPU_LIB="/mnt/c/Users/Elias/Documents/languages/webgpu/windows/lib/wgpu_native.dll"
  FREETYPE_INC="/usr/include/freetype2"
  FREETYPE_LIB="/usr/lib/x86_64-linux-gnu/libfreetype.a"
fi

GLFW3WEBGPU_DIR="/mnt/c/Users/Elias/Documents/languages/glfw3webgpu"

# ── Lua paths ──────────────────────────────────────────────────────────────
COMPAT53_ARGS=""

if [ "$PLATFORM" = "linux" ]; then
  if [ "$LUAJIT" = "ON" ]; then
    LUA_BIN="luajit"
    LUA_INC="/usr/include/luajit-2.1"
    LUA_LIB="/usr/lib/x86_64-linux-gnu/libluajit-5.1.so"
    if ! dpkg -s lua-compat53 &>/dev/null; then
      echo "Installing compat53..."
      sudo apt install -y lua-compat53
    fi
    COMPAT53_ARGS="-DCOMPAT53_INCLUDE_DIR=/usr/include -DCOMPAT53_LIBRARY=/usr/lib/x86_64-linux-gnu/lua/5.4/compat53.so"
  else
    LUA_BIN="lua5.4"
    LUA_INC="/usr/include/lua5.4"
    LUA_LIB="/usr/lib/x86_64-linux-gnu/liblua5.4.so"
  fi
else  # windows
  COMPAT53_WIN_INC="/mnt/c/Users/Elias/Documents/languages/compat53/include"
  COMPAT53_WIN_LIB="/mnt/c/Users/Elias/Documents/languages/compat53/compat53.dll"
  if [ "$LUAJIT" = "ON" ]; then
    LUA_INC="/mnt/c/Users/Elias/Documents/languages/luajit/include"
    LUA_LIB="/mnt/c/Users/Elias/Documents/languages/luajit/lua51.dll"
    COMPAT53_ARGS="-DCOMPAT53_INCLUDE_DIR=$COMPAT53_WIN_INC -DCOMPAT53_LIBRARY=$COMPAT53_WIN_LIB"
  else
    LUA_INC="/mnt/c/Users/Elias/Documents/languages/lua/include"
    LUA_LIB="/mnt/c/Users/Elias/Documents/languages/lua/liblua54.a"
  fi
fi

# ── Build ──────────────────────────────────────────────────────────────────
BUILD_DIR="$SCRIPT_DIR/build/$PLATFORM"
mkdir -p "$BUILD_DIR" && cd "$BUILD_DIR"

TOOLCHAIN_ARG=""
if [ "$PLATFORM" = "windows" ]; then
  TOOLCHAIN_ARG="-DCMAKE_TOOLCHAIN_FILE=../../toolchain-windows.cmake"
fi

cmake "../.." \
  $TOOLCHAIN_ARG \
  "-DUSE_LUAJIT=$LUAJIT" \
  "-DVANIR_VERBOSE=$VERBOSE" \
  "-DLUA_INCLUDE_DIR=$LUA_INC" \
  "-DLUA_LIBRARIES=$LUA_LIB" \
  "-DLUA_LIBRARY=$LUA_LIB" \
  "-DGLFW3WEBGPU_DIR=$GLFW3WEBGPU_DIR" \
  "-DGLFW_INCLUDE_DIR=$GLFW_INC" \
  "-DGLFW_LIBRARIES=$GLFW_LIB" \
  "-DWEBGPU_INCLUDE_DIR=$WEBGPU_INC" \
  "-DWEBGPU_LIBRARIES=$WEBGPU_LIB" \
  "-DFREETYPE_INCLUDE_DIR=$FREETYPE_INC" \
  "-DFREETYPE_LIBRARIES=$FREETYPE_LIB" \
  $COMPAT53_ARGS

cmake --build .

if [ "$PLATFORM" = "linux" ]; then
  cp Vanir.so "$SCRIPT_DIR/"
  echo "✓ Built Vanir.so"
else
  cp Vanir.dll "$SCRIPT_DIR/"
  echo "✓ Built Vanir.dll"
fi

# ── Tests (only if --test was passed) ──────────────────────────────────────
if [ "$RUN_TESTS" = "OFF" ]; then
  echo "ℹ  Skipping tests (pass --test to run them)."
  exit 0
fi

cd "$SCRIPT_DIR"
TEST_DIR="$SCRIPT_DIR/tests"

if [ ! -d "$TEST_DIR" ]; then
  echo "⚠  No tests/ folder found, skipping."
  exit 0
fi

mapfile -t TESTS < <(find "$TEST_DIR" -name "*.lua" | sort)
if [ ${#TESTS[@]} -eq 0 ]; then
  echo "⚠  No .lua test files found in tests/, skipping."
  exit 0
fi

PASSED=0
FAILED=0
ERRORS=()

if [ "$PLATFORM" = "linux" ]; then
  for TEST in "${TESTS[@]}"; do
    NAME=$(basename "$TEST")
    if "$LUA_BIN" -e "package.cpath='$SCRIPT_DIR/?.so;'..package.cpath" "$TEST" > /tmp/lua_test_out 2>&1; then
      echo "  ✓ $NAME"
      PASSED=$((PASSED + 1))
    else
      echo "  ✗ $NAME"
      sed 's/^/      /' /tmp/lua_test_out
      FAILED=$((FAILED + 1))
      ERRORS+=("$NAME")
    fi
  done
else
  LUA_EXE=$([ "$LUAJIT" = "ON" ] && echo "luajit.exe" || echo "lua54.exe")
  TMP_WSL="/tmp/vanir_test_$$"
  mkdir -p "$TMP_WSL"
  trap 'rm -rf "$TMP_WSL"' EXIT
  cp "$SCRIPT_DIR/Vanir.dll" "$TMP_WSL/"
  WIN_TMP=$(wslpath -w "$TMP_WSL")

  for TEST in "${TESTS[@]}"; do
    NAME=$(basename "$TEST")
    cp "$TEST" "$TMP_WSL/$NAME"
    if cmd.exe /C "cd /d \"$WIN_TMP\" && $LUA_EXE -e \"package.cpath='.\\\\?.dll;'..package.cpath\" $NAME" \
         > /tmp/lua_test_out 2>&1; then
      echo "  ✓ $NAME"
      PASSED=$((PASSED + 1))
    else
      echo "  ✗ $NAME"
      sed 's/^/      /' /tmp/lua_test_out
      FAILED=$((FAILED + 1))
      ERRORS+=("$NAME")
    fi
  done
fi

echo ""
echo "Tests: $PASSED passed, $FAILED failed"
if [ $FAILED -ne 0 ]; then
  echo "Failed: ${ERRORS[*]}"
  exit 1
fi
