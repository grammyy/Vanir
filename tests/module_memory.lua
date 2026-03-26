require("vanir")

-- module_memory.lua
-- tests: memory.getAddr, memory.toHex,
--        memory.readByte/writeByte, readShort/writeShort, readUShort/writeUShort,
--        readLong/writeLong, readULong/writeULong,
--        readFloat/writeFloat, readDouble/writeDouble,
--        readInt64/writeInt64, readUInt64/writeUInt64,
--        readBool/writeBool, readString/writeString, readBytes/writeBytes

-- memory.protect and memory.pageConstants are Windows-only; tested

local PASS = 0
local FAIL = 0

local function check(label, got, expected)
    if got == expected then
        print("[memory] OK  " .. label .. " -> " .. tostring(got))
        PASS = PASS + 1
    else
        print("[memory] FAIL " .. label .. "  got=" .. tostring(got) .. "  expected=" .. tostring(expected))
        FAIL = FAIL + 1
    end
end

-- ── getAddr / toHex ──────────────────────────────────────────────────────────

-- Use a fixed-size mutable scratch buffer: pad with known bytes so writes land
-- getAddr returns the raw char* so write operations work in practice for test

local scratch = string.rep("\0", 32)   -- 32 zero bytes
local addr    = memory.getAddr(scratch)
assert(type(addr) == "number" and addr ~= 0, "getAddr should return a non-zero integer")
print("[memory] getAddr: " .. addr)

local hex = memory.toHex(addr)
assert(type(hex) == "string" and #hex > 0, "toHex should return a non-empty string")
print("[memory] toHex:   " .. hex)

-- ── byte ─────────────────────────────────────────────────────────────────────

memory.writeByte(addr, 0xAB)
check("readByte", memory.readByte(addr), 0xAB)

-- ── short (signed) ───────────────────────────────────────────────────────────

memory.writeShort(addr, -1000)
check("readShort", memory.readShort(addr), -1000)

-- ── ushort ───────────────────────────────────────────────────────────────────

memory.writeUShort(addr, 60000)
check("readUShort", memory.readUShort(addr), 60000)

-- ── long (32-bit signed) ─────────────────────────────────────────────────────

memory.writeLong(addr, -123456)
check("readLong", memory.readLong(addr), -123456)

-- ── ulong ────────────────────────────────────────────────────────────────────

memory.writeULong(addr, 3000000000)
check("readULong", memory.readULong(addr), 3000000000)

-- ── float ────────────────────────────────────────────────────────────────────

memory.writeFloat(addr, 3.14)
local rf = memory.readFloat(addr)
assert(math.abs(rf - 3.14) < 0.001, "readFloat should approximate 3.14, got " .. rf)
print("[memory] OK  readFloat -> " .. rf)
PASS = PASS + 1

-- ── double ───────────────────────────────────────────────────────────────────

memory.writeDouble(addr, 2.718281828)
local rd = memory.readDouble(addr)
assert(math.abs(rd - 2.718281828) < 1e-9, "readDouble should match 2.718281828, got " .. rd)
print("[memory] OK  readDouble -> " .. rd)
PASS = PASS + 1

-- ── int64 ────────────────────────────────────────────────────────────────────

memory.writeInt64(addr, -9000000000)
check("readInt64", memory.readInt64(addr), -9000000000)

-- ── uint64 (decimal string round-trip) ───────────────────────────────────────

local big = "12345678901234"   -- fits in uint64, safe decimal string
memory.writeUInt64(addr, big)
local ru64 = memory.readUInt64(addr)
check("readUInt64", ru64, big)

-- ── bool ─────────────────────────────────────────────────────────────────────

memory.writeBool(addr, true)
check("readBool true",  memory.readBool(addr), true)

memory.writeBool(addr, false)
check("readBool false", memory.readBool(addr), false)

-- ── string ───────────────────────────────────────────────────────────────────

local testStr = "hello"
memory.writeString(addr, testStr)
check("readString", memory.readString(addr, #testStr), testStr)

-- ── bytes (alias for writeString / readString) ────────────────────────────────

local rawBytes = "\x01\x02\x03\x04\x05"
memory.writeBytes(addr, rawBytes)
check("readBytes", memory.readBytes(addr, #rawBytes), rawBytes)

-- ── Windows-only: protect / pageConstants ────────────────────────────────────

local isWindows = package.config:sub(1, 1) == "\\"
if isWindows then
    local pc = memory.pageConstants()
    assert(type(pc) == "table", "pageConstants should return a table")
    assert(type(pc.READWRITE) == "number", "PAGE_READWRITE should be a number")
    print("[memory] pageConstants: OK")

    local ok = memory.protect(addr, 32, pc.READWRITE)
    assert(ok, "protect should succeed on a valid page")
    print("[memory] protect: OK")
    PASS = PASS + 2
else
    print("[memory] protect/pageConstants: skipped (non-Windows)")
end

-- ── summary ──────────────────────────────────────────────────────────────────

print("[memory] passed: " .. PASS .. "  failed: " .. FAIL)

if FAIL == 0 then
    print("[memory] PASS")
else
    print("[memory] FAIL (" .. FAIL .. " checks failed)")
end

quit()
