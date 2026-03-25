require("vanir")

-- test_timer.lua
-- tests: timer.realtime returns a number, advances over time

local t0=timer.realtime()

print("[timer] t0: "..t0)

-- spin for a bit
local spun=0
for i=1,1000000 do spun=spun+1 end

local t1=timer.realtime()

print("[timer] t1: "..t1)
print("[timer] delta: "..(t1-t0).."ms")

if t1>t0 then
    print("[timer] PASS: time advanced")
else
    print("[timer] FAIL: time did not advance")
end

quit()
