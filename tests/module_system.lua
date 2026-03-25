require("vanir")

-- test_system.lua
-- tests: getOS, getUsername, getSystemInfo, getMonitorCount, getScreenSize, clipboard, getEnv, getTime

print("[system] os:       "..system.getOS())
print("[system] username: "..system.getUsername())

local info=system.getSystemInfo()
print("[system] sysname:  "..(info.sysname  or "?"))
print("[system] nodename: "..(info.nodename or "?"))
print("[system] release:  "..(info.release  or "?"))
print("[system] machine:  "..(info.machine  or "?"))

local monCount=system.getMonitorCount()
print("[system] monitors: "..monCount)

local monitors=system.getMonitors()
for i,m in ipairs(monitors) do
    print("[system] monitor "..i..": "..m.width.."x"..m.height.." @"..m.refreshRate.."hz  ("..m.name..")")
end

local sw,sh=system.getScreenSize()
print("[system] primary screen: "..sw.."x"..sh)

-- clipboard round-trip
local testStr="vanir_clipboard_test"
system.setClipboard(testStr)
local got=system.getClipboard()

if got==testStr then
    print("[system] clipboard: PASS")
else
    print("[system] clipboard: FAIL (got "..(got or "nil")..")")
end

-- env
local path=system.getEnv("PATH")
print("[system] PATH set: "..(path and "yes" or "no"))

-- time
local t0=system.getTime()
local spun=0
for i=1,500000 do spun=spun+1 end
local t1=system.getTime()
print("[system] getTime delta: "..(t1-t0).."s")

if t1>t0 then
    print("[system] PASS")
else
    print("[system] FAIL: time did not advance")
end

quit()
