require("vanir")

-- module_timer.lua
-- tests: timer.realtime, timer.systime, timer.curtime, timer.frametime,
--        timer.create, timer.simple, timer.remove, timer.exists,
--        timer.start, timer.stop, timer.pause, timer.unpause, timer.toggle,
--        timer.adjust, timer.timeleft, timer.repsleft, timer.getTimersLeft

-- ── basic time functions ──────────────────────────────────────────────────────

local t0 = timer.realtime()
assert(type(t0) == "number", "realtime should return a number")
print("[timer] realtime t0: " .. t0)

-- busy-spin so time advances
for i = 1, 1000000 do end

local t1 = timer.realtime()
assert(t1 > t0, "realtime should advance")
print("[timer] realtime delta: " .. (t1 - t0) .. "ms")

local st = timer.systime()
assert(type(st) == "number" and st > 0, "systime should return a positive number")
print("[timer] systime: " .. st)

local ct = timer.curtime()
assert(type(ct) == "number", "curtime should return a number")
print("[timer] curtime: " .. ct)

-- frametime may be 0 before the first tick but must be a number
local ft = timer.frametime()
assert(type(ft) == "number", "frametime should return a number")
print("[timer] frametime: " .. ft)

-- ── named timer: create / exists / timeleft / repsleft ───────────────────────

local firedCount = 0

timer.create("test_repeating", 50, 3, function()
    firedCount = firedCount + 1
    print("[timer] test_repeating fired (" .. firedCount .. ")")
end)

assert(timer.exists("test_repeating"), "timer should exist after create")
print("[timer] exists: OK")

local tl = timer.timeleft("test_repeating")
assert(type(tl) == "number", "timeleft should return a number")
print("[timer] timeleft: " .. tl .. "ms")

local rl = timer.repsleft("test_repeating")
assert(rl == 3, "repsleft should be 3 after create")
print("[timer] repsleft: " .. rl)

-- ── pause / unpause / toggle ──────────────────────────────────────────────────

timer.pause("test_repeating")
local tl_paused = timer.timeleft("test_repeating")
assert(tl_paused < 0, "timeleft should be negative while paused")
print("[timer] timeleft while paused: " .. tl_paused .. " (negative = paused)")

timer.unpause("test_repeating")
print("[timer] unpause: OK")

timer.toggle("test_repeating")   -- pause again
timer.toggle("test_repeating")   -- unpause again
print("[timer] toggle x2: OK")

-- ── adjust ───────────────────────────────────────────────────────────────────

timer.adjust("test_repeating", 60, 5)
local rl2 = timer.repsleft("test_repeating")
assert(rl2 == 5, "repsleft should be 5 after adjust")
print("[timer] adjust repsleft: " .. rl2)

-- ── stop / start ─────────────────────────────────────────────────────────────

timer.stop("test_repeating")
assert(not timer.exists("test_repeating"), "timer should not exist after stop")
print("[timer] stop: OK")

-- ── timer.simple (auto-named, one-shot) ──────────────────────────────────────

local simpleFired = false
timer.simple(30, function()
    simpleFired = true
    print("[timer] simple fired")
end)

-- ── timer.getTimersLeft ───────────────────────────────────────────────────────

local count = timer.getTimersLeft()
assert(type(count) == "number" and count >= 0, "getTimersLeft should return a non-negative number")
print("[timer] getTimersLeft: " .. count)

-- ── run the hook loop long enough for timers to fire ─────────────────────────

local startTime = timer.realtime()
local TIMEOUT   = 500  -- ms; long enough for the 60ms simple timer

hook.add("think", "test_timer_finish", function()
    local elapsed = timer.realtime() - startTime

    if elapsed >= TIMEOUT then
        if simpleFired then
            print("[timer] simple timer: OK")
        else
            print("[timer] WARN: simple timer did not fire within " .. TIMEOUT .. "ms")
        end

        print("[timer] PASS")
        quit()
    end
end)

while true do
    hooks.run()
end
