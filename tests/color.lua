require("vanir")

local pass = 0
local fail = 0

local function check(label, got, expected)
    if got == expected then
        print("[PASS] " .. label)
        pass = pass + 1
    else
        print("[FAIL] " .. label)
        print("       expected: " .. tostring(expected))
        print("       got:      " .. tostring(got))
        fail = fail + 1
    end
end

local function checkNear(label, got, expected, tolerance)
    tolerance = tolerance or 0.5
    if math.abs(got - expected) <= tolerance then
        print("[PASS] " .. label)
        pass = pass + 1
    else
        print("[FAIL] " .. label)
        print("       expected: " .. tostring(expected) .. " (±" .. tolerance .. ")")
        print("       got:      " .. tostring(got))
        fail = fail + 1
    end
end

-- ── Color constructor ─────────────────────────────────────────────────────────

print("\n── Color constructor ────────────────────────────────────────────────────")

local c = Color(255, 128, 0, 200)
check("r field",  c.r, 255)
check("g field",  c.g, 128)
check("b field",  c.b, 0)
check("a field",  c.a, 200)

local cdef = Color()
check("default r", cdef.r, 255)
check("default g", cdef.g, 255)
check("default b", cdef.b, 255)
check("default a", cdef.a, 255)

-- ── __tostring ────────────────────────────────────────────────────────────────

print("\n── __tostring ───────────────────────────────────────────────────────────")

local s = tostring(Color(255, 0, 128, 255))
check("tostring not nil", s ~= nil, true)
check("tostring is string", type(s), "string")
-- should contain all four values
check("tostring has r", s:find("255") ~= nil, true)
check("tostring has b", s:find("128") ~= nil, true)
print("  tostring output: " .. s)

-- ── __eq ──────────────────────────────────────────────────────────────────────

print("\n── __eq ─────────────────────────────────────────────────────────────────")

check("equal colors",     Color(255,0,0) == Color(255,0,0), true)
check("unequal colors",   Color(255,0,0) == Color(0,255,0), false)
check("unequal alpha",    Color(255,0,0,255) == Color(255,0,0,0), false)

-- ── unpack ────────────────────────────────────────────────────────────────────

print("\n── unpack ───────────────────────────────────────────────────────────────")

local r, g, b, a = Color(10, 20, 30, 40):unpack()
check("unpack r", r, 10)
check("unpack g", g, 20)
check("unpack b", b, 30)
check("unpack a", a, 40)

-- ── lerp ──────────────────────────────────────────────────────────────────────

print("\n── lerp ─────────────────────────────────────────────────────────────────")

local black = Color(0,   0,   0,   255)
local white = Color(255, 255, 255, 255)

local mid = black:lerp(white, 0.5)
check("lerp r midpoint", mid.r, 127.5)
check("lerp g midpoint", mid.g, 127.5)
check("lerp b midpoint", mid.b, 127.5)
check("lerp a midpoint", mid.a, 255)

local at0 = black:lerp(white, 0)
check("lerp t=0 r", at0.r, 0)

local at1 = black:lerp(white, 1)
check("lerp t=1 r", at1.r, 255)

-- lerp result should also be a Color (has :unpack)
local lr2, lg2 = mid:unpack()
check("lerp result has methods", lr2, 127.5)

-- ── HSV round-trip ────────────────────────────────────────────────────────────

print("\n── HSV round-trip ───────────────────────────────────────────────────────")

-- red in RGB is H=0, S=100, V=100
local red    = Color(255, 0, 0)
local redHSV = red:toHSV()
checkNear("red → H", redHSV.r, 0,   1)
checkNear("red → S", redHSV.g, 100, 1)
checkNear("red → V", redHSV.b, 100, 1)

-- green: H=120
local green    = Color(0, 255, 0)
local greenHSV = green:toHSV()
checkNear("green → H", greenHSV.r, 120, 1)

-- blue: H=240
local blue    = Color(0, 0, 255)
local blueHSV = blue:toHSV()
checkNear("blue → H", blueHSV.r, 240, 1)

-- yellow: H=60
local yellow    = Color(255, 255, 0)
local yellowHSV = yellow:toHSV()
checkNear("yellow → H", yellowHSV.r, 60, 1)

-- round-trip: RGB → HSV → RGB
local orig   = Color(123, 45, 200)
local trip   = orig:toHSV():toRGB()
checkNear("round-trip r", trip.r, orig.r, 1)
checkNear("round-trip g", trip.g, orig.g, 1)
checkNear("round-trip b", trip.b, orig.b, 1)

-- backward-compat aliases
local aliasHSV = red:rgbToHSV()
checkNear("rgbToHSV alias H", aliasHSV.r, 0, 1)
local aliasRGB = aliasHSV:hsvToRGB()
checkNear("hsvToRGB alias r", aliasRGB.r, 255, 1)

-- grey: saturation = 0
local grey    = Color(128, 128, 128)
local greyHSV = grey:toHSV()
checkNear("grey → S=0", greyHSV.g, 0, 1)
local greyBack = greyHSV:toRGB()
checkNear("grey round-trip r", greyBack.r, 128, 1)

-- ── Vector constructor ────────────────────────────────────────────────────────

print("\n── Vector constructor ───────────────────────────────────────────────────")

local v = Vector(1, 2, 3)
check("x", v.x, 1)
check("y", v.y, 2)
check("z", v.z, 3)

local vdef = Vector()
check("default x", vdef.x, 0)

-- ── Vector __tostring ─────────────────────────────────────────────────────────

print("\n── Vector __tostring ────────────────────────────────────────────────────")

local vs = tostring(Vector(1, 2, 3))
check("tostring is string", type(vs), "string")
print("  tostring output: " .. vs)

-- ── Vector arithmetic ─────────────────────────────────────────────────────────

print("\n── Vector arithmetic ────────────────────────────────────────────────────")

local a = Vector(1, 2, 3)
local b = Vector(4, 5, 6)

local add = a + b
check("add x", add.x, 5)
check("add y", add.y, 7)
check("add z", add.z, 9)

local sub = b - a
check("sub x", sub.x, 3)
check("sub y", sub.y, 3)
check("sub z", sub.z, 3)

local mul = a * 2
check("mul x", mul.x, 2)
check("mul y", mul.y, 4)
check("mul z", mul.z, 6)

local rmul = 3 * a   -- number * vec
check("rmul x", rmul.x, 3)

local neg = -a
check("unm x", neg.x, -1)
check("unm y", neg.y, -2)

-- ── Vector __eq ───────────────────────────────────────────────────────────────

print("\n── Vector __eq ──────────────────────────────────────────────────────────")

check("equal",   Vector(1,2,3) == Vector(1,2,3), true)
check("unequal", Vector(1,2,3) == Vector(1,2,4), false)

-- ── Vector __len (magnitude) ──────────────────────────────────────────────────

print("\n── Vector __len (magnitude) ─────────────────────────────────────────────")

local unit_x = Vector(1, 0, 0)
checkNear("#unit_x", #unit_x, 1.0, 0.001)

local v345 = Vector(3, 4, 0)
checkNear("#(3,4,0)", #v345, 5.0, 0.001)  -- 3-4-5 triangle

-- ── Vector methods ────────────────────────────────────────────────────────────

print("\n── Vector methods ───────────────────────────────────────────────────────")

-- dot
checkNear("dot parallel",     Vector(1,0,0):dot(Vector(1,0,0)),  1.0, 0.001)
checkNear("dot perpendicular",Vector(1,0,0):dot(Vector(0,1,0)),  0.0, 0.001)
checkNear("dot antiparallel", Vector(1,0,0):dot(Vector(-1,0,0)), -1.0, 0.001)

-- cross
local cross = Vector(1,0,0):cross(Vector(0,1,0))
checkNear("cross x", cross.x, 0, 0.001)
checkNear("cross y", cross.y, 0, 0.001)
checkNear("cross z", cross.z, 1, 0.001)

-- normalize
local n = Vector(3, 4, 0):normalize()
checkNear("normalize length", #n, 1.0, 0.001)
checkNear("normalize x", n.x, 0.6, 0.001)
checkNear("normalize y", n.y, 0.8, 0.001)

-- normalize zero vector — should not crash, returns zero
local nz = Vector(0,0,0):normalize()
check("normalize zero x", nz.x, 0)

-- length method same as __len
checkNear("length method", Vector(3,4,0):length(), 5.0, 0.001)

-- chaining: result of arithmetic has methods
local chain = (a + b):normalize()
checkNear("chain normalize length", #chain, 1.0, 0.001)

-- ── Summary ───────────────────────────────────────────────────────────────────

print("\n─────────────────────────────────────────────────────────────────────────")
print(string.format("Results: %d passed, %d failed, %d total", pass, fail, pass + fail))
if fail == 0 then
    print("All tests passed!")
else
    print("Some tests failed.")
end
