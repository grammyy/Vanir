require("vanir")

-- module_system.lua
-- tests: getOS, getUsername, getSystemInfo, getMonitorCount, getScreenSize,
--        clipboard, getEnv, getTime, getCPUCount, getTotalRAM,
--        getExecutablePath, sleep, getLocale

print("[system] os:       "..system.getOS())
print("[system] username: "..system.getUsername())

local info = system.getSystemInfo()
print("[system] sysname:  "..(info.sysname  or "?"))
print("[system] nodename: "..(info.nodename or "?"))
print("[system] release:  "..(info.release  or "?"))
print("[system] machine:  "..(info.machine  or "?"))

local monCount = system.getMonitorCount()
print("[system] monitors: "..monCount)

local monitors = system.getMonitors()
for i, m in ipairs(monitors) do
    print("[system] monitor "..i..": "..m.width.."x"..m.height.." @"..m.refreshRate.."hz  ("..m.name..")")
end

local sw, sh = system.getScreenSize()
print("[system] primary screen: "..sw.."x"..sh)

-- clipboard round-trip
local testStr = "vanir_clipboard_test"
system.setClipboard(testStr)
local got = system.getClipboard()

if got == testStr then
    print("[system] clipboard: PASS")
else
    print("[system] clipboard: FAIL (got "..(got or "nil")..")")
end

-- env
local path = system.getEnv("PATH")
print("[system] PATH set: "..(path and "yes" or "no"))

-- time
local t0 = system.getTime()
local spun = 0
for i = 1, 500000 do spun = spun + 1 end
local t1 = system.getTime()
print("[system] getTime delta: "..(t1 - t0).."s")

if t1 > t0 then
    print("[system] getTime: PASS")
else
    print("[system] getTime: FAIL: time did not advance")
end

-- getCPUCount
local cpus = system.getCPUCount()
print("[system] CPU count: "..cpus)
if cpus >= 1 then
    print("[system] getCPUCount: PASS")
else
    print("[system] getCPUCount: FAIL")
end

-- getTotalRAM
local ram = system.getTotalRAM()
if ram then
    print("[system] total RAM: "..string.format("%.2f", ram / (1024*1024*1024)).." GB")
    print("[system] getTotalRAM: PASS")
else
    print("[system] getTotalRAM: FAIL (nil)")
end

-- getExecutablePath
local exe = system.getExecutablePath()
if exe then
    print("[system] executable: "..exe)
    print("[system] getExecutablePath: PASS")
else
    print("[system] getExecutablePath: FAIL (nil)")
end

-- sleep
local before = system.getTime()
system.sleep(50)
local after = system.getTime()
local sleptMs = (after - before) * 1000
print("[system] sleep(50) actual: "..string.format("%.1f", sleptMs).."ms")
if sleptMs >= 40 then
    print("[system] sleep: PASS")
else
    print("[system] sleep: FAIL (too short)")
end

-- getLocale
local locale = system.getLocale()
print("[system] locale: "..(locale or "nil"))
if locale and #locale >= 2 then
    print("[system] getLocale: PASS")
else
    print("[system] getLocale: FAIL")
end

quit()
