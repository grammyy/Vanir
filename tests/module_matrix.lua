require("vanir")

-- module_matrix.lua
-- tests: Matrix constructor, :translate, :rotate, :scale, :inverse,
--        :transformPoint, render.pushMatrix/popMatrix/setMatrix/getMatrix/resetMatrix

local PASS = 0
local FAIL = 0

local function check(label, got, expected)
    if got == expected then
        print("[matrix] OK  " .. label .. " -> " .. tostring(got))
        PASS = PASS + 1
    else
        print("[matrix] FAIL " .. label .. "  got=" .. tostring(got) .. "  expected=" .. tostring(expected))
        FAIL = FAIL + 1
    end
end

local function approx(label, got, expected, tol)
    tol = tol or 1e-4
    if type(got) == "number" and math.abs(got - expected) <= tol then
        print("[matrix] OK  " .. label .. " -> " .. got)
        PASS = PASS + 1
    else
        print("[matrix] FAIL " .. label .. "  got=" .. tostring(got) .. "  expected~=" .. tostring(expected))
        FAIL = FAIL + 1
    end
end

-- ── constructor ───────────────────────────────────────────────────────────────

local id = Matrix()
check("identity [1]", id[1], 1)
check("identity [5]", id[5], 1)
check("identity [9]", id[9], 1)
check("identity [2]", id[2], 0)
check("identity [4]", id[4], 0)

-- from degrees
local rot90 = Matrix(90)
local ox, oy = rot90:transformPoint(1, 0)
approx("Matrix(90) on (1,0) x", ox, 0)
approx("Matrix(90) on (1,0) y", oy, 1)

-- from Angle
local rot45 = Matrix(Angle(0, 0, 45))
local ax, ay = rot45:transformPoint(1, 0)
approx("Matrix(Angle 45) on (1,0) x", ax, math.sqrt(2)/2)
approx("Matrix(Angle 45) on (1,0) y", ay, math.sqrt(2)/2)

-- with translation via Vector
local trans = Matrix(nil, Vector(10, 20))
local tx, ty = trans:transformPoint(0, 0)
approx("Matrix(nil,Vector(10,20)) on (0,0) x", tx, 10)
approx("Matrix(nil,Vector(10,20)) on (0,0) y", ty, 20)

-- ── :translate ────────────────────────────────────────────────────────────────

local t = Matrix():translate(5, 3)
local px, py = t:transformPoint(0, 0)
approx(":translate(5,3) on (0,0) x", px, 5)
approx(":translate(5,3) on (0,0) y", py, 3)

-- ── :scale ────────────────────────────────────────────────────────────────────

local s = Matrix():scale(2, 3)
local sx, sy = s:transformPoint(1, 1)
approx(":scale(2,3) on (1,1) x", sx, 2)
approx(":scale(2,3) on (1,1) y", sy, 3)

-- uniform scale
local su = Matrix():scale(4)
local sux, suy = su:transformPoint(1, 1)
approx(":scale(4) on (1,1) x", sux, 4)
approx(":scale(4) on (1,1) y", suy, 4)

-- ── :rotate ───────────────────────────────────────────────────────────────────

local r = Matrix():rotate(90)
local rx, ry = r:transformPoint(1, 0)
approx(":rotate(90) on (1,0) x", rx, 0)
approx(":rotate(90) on (1,0) y", ry, 1)

-- rotate with Angle
local ra = Matrix():rotate(Angle(0, 0, 90))
local rax, ray = ra:transformPoint(1, 0)
approx(":rotate(Angle 90) on (1,0) x", rax, 0)
approx(":rotate(Angle 90) on (1,0) y", ray, 1)

-- ── :inverse ──────────────────────────────────────────────────────────────────

local tinv = Matrix():translate(5, 3):inverse()
local bx, by = tinv:transformPoint(5, 3)
approx(":inverse of translate(5,3) on (5,3) x", bx, 0)
approx(":inverse of translate(5,3) on (5,3) y", by, 0)

-- ── :copy ─────────────────────────────────────────────────────────────────────

local orig = Matrix():translate(7, 2)
local cop  = orig:copy()
check(":copy [7]", cop[7], orig[7])
check(":copy [8]", cop[8], orig[8])

-- ── __tostring ────────────────────────────────────────────────────────────────

local str = tostring(Matrix())
check("__tostring type", type(str), "string")
check("__tostring has |", str:find("|") ~= nil, true)
print("[matrix]    tostring: " .. str)

-- ── __mul (Matrix * Matrix) ───────────────────────────────────────────────────

-- post-multiply order: last method applied first to points
-- translate(10,0):rotate(90) = T*R
-- on (0,0): R rotates (0,0)=(0,0), T translates to (10,0)
local TR = Matrix():translate(10, 0):rotate(90)
local trx, try_ = TR:transformPoint(0, 0)
approx("translate(10,0):rotate(90) on (0,0) x", trx, 10)
approx("translate(10,0):rotate(90) on (0,0) y", try_, 0)

-- on (1,0): R rotates (1,0)=(0,1), T translates to (10,1)
local trx2, try2 = TR:transformPoint(1, 0)
approx("translate(10,0):rotate(90) on (1,0) x", trx2, 10)
approx("translate(10,0):rotate(90) on (1,0) y", try2, 1)

-- ── __mul (Matrix * Vector) ───────────────────────────────────────────────────

local mv = Matrix():translate(3, 7) * Vector(1, 2)
approx("Matrix * Vector x", mv.x, 4)
approx("Matrix * Vector y", mv.y, 9)

-- ── pivot rotation ────────────────────────────────────────────────────────────

-- rotate 90° around (10, 0):
-- chain: translate(px,py) -> rotate -> translate(-px,-py)
-- points: shift by -pivot, rotate, shift back
local pvx, pvy = 10, 0
local pivot = Matrix():translate(pvx, pvy):rotate(90):translate(-pvx, -pvy)

-- pivot itself should not move
local apx, apy = pivot:transformPoint(pvx, pvy)
approx("pivot stays x", apx, pvx)
approx("pivot stays y", apy, pvy)

-- point 1 unit right of pivot rotates to 1 unit above pivot
local apx2, apy2 = pivot:transformPoint(pvx+1, pvy)
approx("rotate around pivot: (pvx+1,pvy) -> x", apx2, pvx)
approx("rotate around pivot: (pvx+1,pvy) -> y", apy2, pvy+1)

-- ── render matrix stack ───────────────────────────────────────────────────────

-- setMatrix / getMatrix round-trip
local custom = Matrix():translate(100, 200)
render.setMatrix(custom)
local got = render.getMatrix()
local gx, gy = got:transformPoint(0, 0)
approx("setMatrix/getMatrix round-trip x", gx, 100)
approx("setMatrix/getMatrix round-trip y", gy, 200)

-- pushMatrix / popMatrix restores state
render.resetMatrix()
render.pushMatrix()
render.setMatrix(Matrix():translate(50, 50))
render.popMatrix()
local after = render.getMatrix()
local afx, afy = after:transformPoint(0, 0)
approx("push/pop restores identity x", afx, 0)
approx("push/pop restores identity y", afy, 0)

-- resetMatrix clears stack too
render.setMatrix(Matrix():translate(999, 999))
render.resetMatrix()
local reset = render.getMatrix()
local rex, rey = reset:transformPoint(0, 0)
approx("resetMatrix -> identity x", rex, 0)
approx("resetMatrix -> identity y", rey, 0)

-- ── summary ───────────────────────────────────────────────────────────────────

print("[matrix] passed: " .. PASS .. "  failed: " .. FAIL)

if FAIL == 0 then
    print("[matrix] PASS")
else
    print("[matrix] FAIL (" .. FAIL .. " checks failed)")
end

quit()
