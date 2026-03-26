require("vanir")

-- module_input.lua
-- tests: input.getKey (poll), hook keyPress, hook keyRelease (Win32),
--        hook inputPressed, hook inputReleased (GLFW cross-platform),
--        KEY enum, KEY_ACTION enum
-- window open for ~10 seconds; press any key to see it logged

local WIN_DURATION = 10000  -- ms

local win = windows.createWindow(400, 300, 520, 240, "input test  (press keys!)")

local pressLog   = {}
local releaseLog = {}
local maxLog     = 6

-- legacy Win32 hooks (no-op on Linux)
hook.add("keyPress", "test_input", function(key)
    table.insert(pressLog, 1, "keyPress: " .. key)
    if #pressLog > maxLog then pressLog[#pressLog] = nil end
    print("[input] keyPress -> " .. key)
end)

hook.add("keyRelease", "test_input", function(key)
    table.insert(releaseLog, 1, "keyRelease: " .. key)
    if #releaseLog > maxLog then releaseLog[#releaseLog] = nil end
    print("[input] keyRelease -> " .. key)
end)

-- GLFW cross-platform hooks — fire on actual key events
local inputPressCount   = 0
local inputReleaseCount = 0

hook.add("inputPressed", "test_inputPressed", function(key)
    inputPressCount = inputPressCount + 1
    table.insert(pressLog, 1, "inputPressed: " .. key)
    if #pressLog > maxLog then pressLog[#pressLog] = nil end
    print("[input] inputPressed  -> " .. input.getKeyName(key))
end)

hook.add("inputReleased", "test_inputReleased", function(key)
    inputReleaseCount = inputReleaseCount + 1
    table.insert(releaseLog, 1, "inputReleased: " .. key)
    if #releaseLog > maxLog then releaseLog[#releaseLog] = nil end
    print("[input] inputReleased -> " .. input.getKeyName(key))
end)

-- KEY enum sanity checks (logged once at startup, no window needed)
assert(type(KEY.SPACE)  == "number", "KEY.SPACE should be a number")
assert(type(KEY.ESCAPE) == "number", "KEY.ESCAPE should be a number")
assert(type(KEY.A)      == "number", "KEY.A should be a number")
assert(KEY.SPACE ~= KEY.ESCAPE,      "KEY.SPACE and KEY.ESCAPE should differ")
print("[input] KEY enum OK  (SPACE=" .. KEY.SPACE .. "  ESCAPE=" .. KEY.ESCAPE .. "  A=" .. KEY.A .. ")")

-- KEY_ACTION enum sanity checks
assert(KEY_ACTION.PRESS   ~= KEY_ACTION.RELEASE, "PRESS and RELEASE should differ")
assert(KEY_ACTION.REPEAT  ~= KEY_ACTION.PRESS,   "REPEAT and PRESS should differ")
print("[input] KEY_ACTION enum OK  (PRESS=" .. KEY_ACTION.PRESS .. "  RELEASE=" .. KEY_ACTION.RELEASE .. "  REPEAT=" .. KEY_ACTION.REPEAT .. ")")

local startTime = timer.realtime()

hook.add("render", "test_input_render", function()
    local elapsed = timer.realtime() - startTime

    -- poll a few common keys each frame using KEY enum
    local shift = input.getKey(KEY.SPACE)  -- SPACE doubles as a held-key indicator on Linux
    local ctrl  = input.getKey(340)        -- GLFW_KEY_LEFT_SHIFT (raw fallback)
    local space = input.getKey(KEY.SPACE)

    win:selectRender()

    local bg = space and Color(30, 60, 30, 255) or Color(20, 20, 30, 255)
    render.clear(bg)

    -- indicator rects for polled keys
    render.setColor(shift and Color(0, 255, 120, 255) or Color(60, 60, 60, 255))
    render.drawRect(20, 20, 80, 30)

    render.setColor(ctrl and Color(0, 180, 255, 255) or Color(60, 60, 60, 255))
    render.drawRect(120, 20, 80, 30)

    render.setColor(space and Color(255, 220, 0, 255) or Color(60, 60, 60, 255))
    render.drawRect(220, 20, 80, 30)

    -- progress bar
    local pct = elapsed / WIN_DURATION
    render.setColor(Color(80, 80, 180, 255))
    render.drawRect(0, 170, math.floor(520 * pct), 10)

    win:stopRender()
    win:update()

    if elapsed >= WIN_DURATION then
        print("[input] PASS (window closed after " .. WIN_DURATION .. "ms)")
        print("[input] keyPress/keyRelease events logged: " .. #pressLog .. " / " .. #releaseLog)
        print("[input] inputPressed count: " .. inputPressCount)
        print("[input] inputReleased count: " .. inputReleaseCount)
        quit()
    end
end)

while true do
    hooks.run()
end
