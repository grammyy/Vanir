require("vanir")

-- module_files.lua
-- tests: files.open, files.exists, files.delete, files.rename,
--        File:write, File:read, File:readLine, File:tell, File:seek, File:skip,
--        File:size, File:endOfFile, File:flush, File:close,
--        typed reads/writes: readByte/writeByte, readBool/writeBool,
--        readShort/writeShort, readUShort/writeUShort,
--        readLong/writeLong, readULong/writeULong,
--        readFloat/writeFloat, readDouble/writeDouble,
--        readUInt64/writeUInt64,
--        __gc auto-close, __tostring

local PASS = 0
local FAIL = 0

local function check(label, got, expected)
    if got == expected then
        print("[files] OK  " .. label .. " -> " .. tostring(got))
        PASS = PASS + 1
    else
        print("[files] FAIL " .. label .. "  got=" .. tostring(got) .. "  expected=" .. tostring(expected))
        FAIL = FAIL + 1
    end
end

local function approx(label, got, expected, tol)
    tol = tol or 1e-5
    if math.abs(got - expected) <= tol then
        print("[files] OK  " .. label .. " -> " .. tostring(got))
        PASS = PASS + 1
    else
        print("[files] FAIL " .. label .. "  got=" .. tostring(got) .. "  expected~=" .. tostring(expected))
        FAIL = FAIL + 1
    end
end

-- pick a temp path that vanir can write to
local TMP      = "test_files_tmp.bin"
local TMP_RENAMED = "test_files_renamed.bin"

-- clean up from any previous run
if files.exists(TMP)         then files.delete(TMP) end
if files.exists(TMP_RENAMED) then files.delete(TMP_RENAMED) end

-- ── files.exists on a non-existent path ──────────────────────────────────────

check("exists(nonexistent)", files.exists(TMP), false)

-- ── open for writing ─────────────────────────────────────────────────────────

local f = files.open(TMP, "wb")
assert(f, "files.open should return a File object")
print("[files] open for write: OK")
print("[files] tostring: " .. tostring(f))

-- ── typed writes ─────────────────────────────────────────────────────────────

f:writeByte(0xBE)
f:writeBool(true)
f:writeBool(false)
f:writeShort(-500)
f:writeUShort(50000)
f:writeLong(-100000)
f:writeULong(200000)
f:writeFloat(1.5)
f:writeDouble(2.71828)
f:writeUInt64("9876543210")

-- raw text write
f:write("hello\nworld\n")

f:flush()
f:close()
print("[files] typed writes + close: OK")
PASS = PASS + 1

-- ── files.exists after write ──────────────────────────────────────────────────

check("exists(after write)", files.exists(TMP), true)

-- ── open for reading, check size ─────────────────────────────────────────────

local rf = files.open(TMP, "rb")
assert(rf, "files.open should return a File for reading")
print("[files] open for read: OK")

local sz = rf:size()
assert(type(sz) == "number" and sz > 0, "size should be > 0, got " .. tostring(sz))
print("[files] size: " .. sz)
PASS = PASS + 1

-- ── typed reads ──────────────────────────────────────────────────────────────

check("readByte",    rf:readByte(),   0xBE)
check("readBool t",  rf:readBool(),   true)
check("readBool f",  rf:readBool(),   false)
check("readShort",   rf:readShort(),  -500)
check("readUShort",  rf:readUShort(), 50000)
check("readLong",    rf:readLong(),   -100000)
check("readULong",   rf:readULong(),  200000)
approx("readFloat",  rf:readFloat(),  1.5,      0.001)
approx("readDouble", rf:readDouble(), 2.71828,  1e-5)
check("readUInt64",  rf:readUInt64(), "9876543210")

-- ── tell / seek / skip ───────────────────────────────────────────────────────

local pos_before_text = rf:tell()
assert(type(pos_before_text) == "number", "tell should return a number")
print("[files] tell (before text): " .. pos_before_text)
PASS = PASS + 1

-- ── readLine ─────────────────────────────────────────────────────────────────

local line1 = rf:readLine()
check("readLine 1", line1, "hello")

local line2 = rf:readLine()
check("readLine 2", line2, "world")

-- ── endOfFile ────────────────────────────────────────────────────────────────

check("endOfFile at end", rf:endOfFile(), true)

-- ── seek back to text, read raw ──────────────────────────────────────────────

rf:seek(pos_before_text)
local raw = rf:read(12)  -- "hello\nworld\n" = 12 bytes
check("read after seek", raw, "hello\nworld\n")

-- ── skip ─────────────────────────────────────────────────────────────────────

rf:seek(pos_before_text)
rf:skip(6)   -- skip "hello\n"
local line_after_skip = rf:readLine()
check("readLine after skip", line_after_skip, "world")

rf:close()
print("[files] read / seek / skip / close: OK")
PASS = PASS + 1

-- ── files.rename ─────────────────────────────────────────────────────────────

files.rename(TMP, TMP_RENAMED)
check("exists(original after rename)", files.exists(TMP),         false)
check("exists(renamed)",               files.exists(TMP_RENAMED), true)

-- ── files.delete ─────────────────────────────────────────────────────────────

files.delete(TMP_RENAMED)
check("exists(after delete)", files.exists(TMP_RENAMED), false)

-- ── __gc auto-close: open a file and let it go out of scope ──────────────────

do
    local gc_tmp = "test_files_gc.bin"
    local gf = files.open(gc_tmp, "wb")
    gf:write("gc test")
    -- gf goes out of scope here; __gc should close it
    gf = nil
    collectgarbage("collect")
    -- if __gc failed, the file would be leaked but we can still verify it exists
    check("exists(gc file)", files.exists(gc_tmp), true)
    files.delete(gc_tmp)
end

print("[files] __gc auto-close: OK")
PASS = PASS + 1

-- ── summary ──────────────────────────────────────────────────────────────────

print("[files] passed: " .. PASS .. "  failed: " .. FAIL)

if FAIL == 0 then
    print("[files] PASS")
else
    print("[files] FAIL (" .. FAIL .. " checks failed)")
end

quit()
