set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
TEST_DIR="$SCRIPT_DIR/tests"

# sadly a whole bunch of bullshit gets logged due to wsl nature, shouldnt on normal linux; ill probably fix another time #

# ↓ usage ↓ #
usage() {
    echo "Usage: $0 [--luajit | --lua54]"
    echo ""
    echo "Options:"
    echo "  --luajit    Force LuaJIT (luajit)"
    echo "  --lua54     Force Lua 5.4 (lua5.4)"
    echo "  (default)   Auto-detect: prefers lua5.4, then luajit, then lua"
    
    exit 1
}

FORCE=""

# ↓ args ↓ #
for arg in "$@"; do
    case $arg in
        --luajit) FORCE="luajit" ;;
        --lua54)  FORCE="lua54"  ;;
        --help|-h) usage ;;
        *) echo "[ERROR] Unknown argument: $arg"; usage ;;
    esac
done

LUA_BIN=""

# ↓ get lua executable ↓ #
if [ "$FORCE" = "luajit" ]; then
    if command -v luajit &>/dev/null; then
        LUA_BIN="luajit"
    else
        echo "[ERROR] --luajit specified but luajit not found in PATH."
        echo "  Install with: sudo apt install luajit"

        exit 1
    fi
elif [ "$FORCE" = "lua54" ]; then
    for candidate in lua5.4 lua54; do
        if command -v "$candidate" &>/dev/null; then
            LUA_BIN="$candidate"

            break
        fi
    done
    
    if [ -z "$LUA_BIN" ]; then
        echo "[ERROR] --lua54 specified but lua5.4 / lua54 not found in PATH."
        echo "  Install with: sudo apt install lua5.4"

        exit 1
    fi
else
    # ↓ try lua54 first, then luajit, then default lua ↓ #
    for candidate in lua5.4 lua54 luajit lua; do
        if command -v "$candidate" &>/dev/null; then
            LUA_BIN="$candidate"

            break
        fi
    done

    if [ -z "$LUA_BIN" ]; then
        echo "[ERROR] No Lua executable found in PATH."
        echo "  Tried: lua5.4, lua54, luajit, lua"
        echo "  Pass --luajit or --lua54, or install a Lua runtime."

        exit 1
    fi
fi

echo "Lua executable : $LUA_BIN"

# ↓ checks ↓ #
if [ ! -d "$TEST_DIR" ]; then
    echo "[WARN] No tests/ folder found, skipping."

    exit 0
fi

if [ ! -f "$SCRIPT_DIR/Vanir.so" ]; then
    echo "[ERROR] Vanir.so not found at $SCRIPT_DIR/Vanir.so"
    echo "  Run: ./build.sh --platform linux"

    exit 1
fi

# ↓ collect tests ↓ #
mapfile -t TESTS < <(find "$TEST_DIR" -maxdepth 1 -type f -name "*.lua" | sort)

if [ ${#TESTS[@]} -eq 0 ]; then
    echo "[WARN] No .lua test files found in tests/, skipping."

    exit 0
fi

PASSED=0
FAILED=0
ERRORS=()

echo ""
echo "Running tests in $TEST_DIR"
echo "─────────────────────────────────────────"

for TEST in "${TESTS[@]}"; do
    NAME=$(basename "$TEST")
    OUT_FILE="/tmp/lua_test_out_$$.txt"

    set +e
    "$LUA_BIN" -e "package.cpath='$SCRIPT_DIR/?.so;'..package.cpath" "$TEST" \
        > "$OUT_FILE" 2>&1
    STATUS=$?
    set -e

    # ↓ exit-code 0 — also check for PASS marker ↓ #
    if grep -q "PASS" "$OUT_FILE"; then
        echo "  PASS  $NAME"
        
        PASSED=$((PASSED + 1))
    elif [ $STATUS -eq 0 ] && [ ! -s "$OUT_FILE" ]; then
        echo "  PASS  $NAME"
        
        PASSED=$((PASSED + 1))
    else
        echo "  FAIL  $NAME"

        sed 's/^/        /' "$OUT_FILE"

        FAILED=$((FAILED + 1))
        ERRORS+=("$NAME")
    fi

    rm -f "$OUT_FILE"
done

# ↓ post summary ↓ #
echo "─────────────────────────────────────────"
echo "Tests: $PASSED passed, $FAILED failed"

if [ $FAILED -ne 0 ]; then
    echo "Failed: ${ERRORS[*]}"

    exit 1
fi

exit 0