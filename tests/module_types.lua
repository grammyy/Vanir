require("vanir")

-- module_types.lua
-- tests: Quaternion constructor, fields (x/y/z/w), __tostring, __eq, __mul
--        (Hamilton product), length, normalize, conjugate, dot, slerp, toAngle

local PASS = 0
local FAIL = 0

local function check(label, got, expected)
    if got == expected then
        print("[types] OK  " .. label .. " -> " .. tostring(got))
        PASS = PASS + 1
    else
        print("[types] FAIL " .. label .. "  got=" .. tostring(got) .. "  expected=" .. tostring(expected))
        FAIL = FAIL + 1
    end
end

local function approx(label, got, expected, tol)
    tol = tol or 1e-5
    if type(got) == "number" and math.abs(got - expected) <= tol then
        print("[types] OK  " .. label .. " -> " .. got)
        PASS = PASS + 1
    else
        print("[types] FAIL " .. label .. "  got=" .. tostring(got) .. "  expected~=" .. tostring(expected))
        FAIL = FAIL + 1
    end
end

-- ── constructor ───────────────────────────────────────────────────────────────

local q = Quaternion(1, 2, 3, 4)
check("q.x", q.x, 1)
check("q.y", q.y, 2)
check("q.z", q.z, 3)
check("q.w", q.w, 4)

-- default w = 1, rest = 0
local qd = Quaternion()
check("default x", qd.x, 0)
check("default y", qd.y, 0)
check("default z", qd.z, 0)
check("default w", qd.w, 1)

-- ── __tostring ────────────────────────────────────────────────────────────────

local s = tostring(q)
check("tostring type",   type(s), "string")
check("tostring has x",  s:find("1") ~= nil, true)
check("tostring has w",  s:find("4") ~= nil, true)
print("[types]    tostring: " .. s)

-- ── __eq ─────────────────────────────────────────────────────────────────────

local qa = Quaternion(1, 0, 0, 0)
local qb = Quaternion(1, 0, 0, 0)
local qc = Quaternion(0, 1, 0, 0)
check("eq same",      qa == qb, true)
check("eq different", qa == qc, false)

-- ── length ───────────────────────────────────────────────────────────────────

-- identity quaternion has length 1
local qi = Quaternion(0, 0, 0, 1)
approx("identity length", qi:length(), 1.0)

-- known: ||(1,2,3,4)|| = sqrt(30)
approx("length(1,2,3,4)", q:length(), math.sqrt(30), 1e-4)

-- ── normalize ────────────────────────────────────────────────────────────────

local qn = q:normalize()
approx("normalize length", qn:length(), 1.0, 1e-5)

-- ── conjugate ────────────────────────────────────────────────────────────────

local qconj = Quaternion(1, 2, 3, 4):conjugate()
check("conjugate x", qconj.x, -1)
check("conjugate y", qconj.y, -2)
check("conjugate z", qconj.z, -3)
check("conjugate w", qconj.w,  4)

-- ── dot ──────────────────────────────────────────────────────────────────────

local da = Quaternion(1, 0, 0, 0)
local db = Quaternion(1, 0, 0, 0)
approx("dot self = 1", da:dot(db), 1.0)

local dc = Quaternion(0, 1, 0, 0)
approx("dot perpendicular = 0", da:dot(dc), 0.0)

-- ── __mul (Hamilton product) ──────────────────────────────────────────────────

-- identity * identity = identity
local qii = qi * qi
approx("identity*identity x", qii.x, 0.0)
approx("identity*identity y", qii.y, 0.0)
approx("identity*identity z", qii.z, 0.0)
approx("identity*identity w", qii.w, 1.0)

-- i * i = -1  (Quaternion(1,0,0,0) * Quaternion(1,0,0,0) = Quaternion(0,0,0,-1))
local qi_unit = Quaternion(1, 0, 0, 0)   -- pure i
local qi_sq   = qi_unit * qi_unit
approx("i*i = -identity w", qi_sq.w, -1.0, 1e-5)
approx("i*i x", qi_sq.x, 0.0)

-- ── slerp ────────────────────────────────────────────────────────────────────

local sa = Quaternion(0, 0, 0, 1)  -- identity
local sb = Quaternion(0, 0, 1, 0)  -- 180° around Z (pure j in convention)

-- slerp at t=0 should return sa
local s0 = sa:slerp(sb, 0)
approx("slerp t=0 w", s0.w, sa.w, 1e-4)
approx("slerp t=0 z", s0.z, sa.z, 1e-4)

-- slerp at t=1 should return sb
local s1 = sa:slerp(sb, 1)
approx("slerp t=1 w", s1.w, sb.w, 1e-4)
approx("slerp t=1 z", s1.z, sb.z, 1e-4)

-- slerp result at t=0.5 should have length 1
local sm = sa:slerp(sb, 0.5)
approx("slerp t=0.5 length", sm:length(), 1.0, 1e-4)

-- ── toAngle ──────────────────────────────────────────────────────────────────

-- identity quaternion should produce near-zero roll/pitch/yaw
local ang = qi:toAngle()
check("toAngle type",       type(ang), "userdata")
check("toAngle has roll",   type(ang.roll),  "number")
check("toAngle has pitch",  type(ang.pitch), "number")
check("toAngle has yaw",    type(ang.yaw),  "number")
approx("toAngle identity roll",  ang.roll,  0.0, 0.01)
approx("toAngle identity pitch", ang.pitch, 0.0, 0.01)
approx("toAngle identity yaw",   ang.yaw,   0.0, 0.01)

-- ── summary ──────────────────────────────────────────────────────────────────

print("[types] passed: " .. PASS .. "  failed: " .. FAIL)

if FAIL == 0 then
    print("[types] PASS")
else
    print("[types] FAIL (" .. FAIL .. " checks failed)")
end

quit()
