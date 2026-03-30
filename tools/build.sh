#!/bin/bash
set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

LUA_BIN=""

# ↓ get lua executable ↓ #
for candidate in lua5.4 lua54 luajit lua; do
    if command -v "$candidate" &>/dev/null; then
        LUA_BIN="$candidate"

        break
    fi
done

if [ -z "$LUA_BIN" ]; then
    echo "[ERROR] No Lua interpreter found in PATH."
    echo "  Install with: sudo apt install lua5.4   (or luajit)"

    exit 1
fi

echo "Using Lua: $LUA_BIN"
echo "─────────────────────────────────────────"

# ↓ step 1: map_symbols.lua ↓ #
echo "Running map_symbols.lua..."

"$LUA_BIN" "$SCRIPT_DIR/map_symbols.lua"

# ↓ step 2: documentation.lua ↓ #
echo "Running documentation.lua..."

"$LUA_BIN" "$SCRIPT_DIR/documentation.lua"

# ↓ step 3: rename and move docs.html → index.html ↓ #
DOCS_HTML="$SCRIPT_DIR/docs.html"
OUT_HTML="$SCRIPT_DIR/index.html"

if [ ! -f "$DOCS_HTML" ]; then
    echo "[ERROR] Expected output not found: $DOCS_HTML"

    exit 1
fi

mv "$DOCS_HTML" "$OUT_HTML"

echo "─────────────────────────────────────────"
echo "Done. Documentation written to: $OUT_HTML"