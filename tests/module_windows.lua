require("vanir")

-- module_windows.lua
-- tests: createWindow, getSize, getTitle, getID, isHovering, isFocused,
--        setTitle, setPos, getPos, setSize, setOpacity, setAlwaysOnTop,
--        minimize + getSize while minimized, focus,
--        setDecorated, setResizable, setAspectRatio, setSizeLimits,
--        setCursorMode, setCursorShape, resetCursor,
--        requestAttention, restore, maximize, setVisible, setRawMouse,
--        render layer hooks (preDrawOpaque, postDrawOpaque,
--                            preDrawTranslucent, postDrawTranslucent),
--        CURSOR_MODE enum, CURSOR_SHAPE enum
-- window stays open for ~10 seconds so you can see it

local WIN_DURATION = 10000  -- ms

local win = windows.createWindow(400, 300, 500, 300, "windows test")

print("[windows] created window: " .. win:getTitle())
print("[windows] id: "            .. tostring(win:getID()))

local w, h = win:getSize()
print("[windows] initial size: " .. w .. "x" .. h)

-- setTitle
win:setTitle("windows test (renamed)")
print("[windows] title after rename: " .. win:getTitle())

-- setPos / getPos
win:setPos(200, 150)
local px, py = win:getPos()
print("[windows] pos after setPos: " .. px .. ", " .. py)

-- setSize
win:setSize(600, 400)

-- setOpacity
win:setOpacity(0.92)

-- setAlwaysOnTop
win:setAlwaysOnTop(true)
win:setAlwaysOnTop(false)

-- setDecorated / setResizable
win:setDecorated(true)
win:setResizable(true)
print("[windows] setDecorated/setResizable: OK")

-- setSizeLimits (minW, minH, maxW, maxH; -1 = no limit)
win:setSizeLimits(200, 150, 1920, 1080)
print("[windows] setSizeLimits: OK")

-- setAspectRatio (numer, denom; -1/-1 = clear)
win:setAspectRatio(16, 9)
win:setAspectRatio(-1, -1)
print("[windows] setAspectRatio: OK")

-- CURSOR_MODE enum sanity
assert(type(CURSOR_MODE.NORMAL)   == "number", "CURSOR_MODE.NORMAL should be a number")
assert(type(CURSOR_MODE.HIDDEN)   == "number", "CURSOR_MODE.HIDDEN should be a number")
assert(type(CURSOR_MODE.DISABLED) == "number", "CURSOR_MODE.DISABLED should be a number")
print("[windows] CURSOR_MODE enum OK")

-- CURSOR_SHAPE enum sanity
assert(type(CURSOR_SHAPE.ARROW)   == "number", "CURSOR_SHAPE.ARROW should be a number")
assert(type(CURSOR_SHAPE.IBEAM)   == "number", "CURSOR_SHAPE.IBEAM should be a number")
assert(CURSOR_SHAPE.ARROW ~= CURSOR_SHAPE.IBEAM, "ARROW and IBEAM should differ")
print("[windows] CURSOR_SHAPE enum OK")

-- setCursorMode / setCursorShape / resetCursor
win:setCursorMode(CURSOR_MODE.NORMAL)
win:setCursorShape(CURSOR_SHAPE.IBEAM)
win:resetCursor()
print("[windows] cursor API: OK")

-- setRawMouse (only works in DISABLED mode; exercise the call path)
win:setCursorMode(CURSOR_MODE.DISABLED)
win:setRawMouse(true)
win:setRawMouse(false)
win:setCursorMode(CURSOR_MODE.NORMAL)
print("[windows] setRawMouse: OK")

-- requestAttention
win:requestAttention()
print("[windows] requestAttention: OK")

-- maximize + restore
win:maximize()
win:restore()
print("[windows] maximize/restore: OK")

-- setVisible
win:setVisible(false)
win:setVisible(true)
print("[windows] setVisible: OK")

-- minimize and check getSize still returns something sane
win:minimize(true)
local mw, mh = win:getSize()
print("[windows] size while minimized: " .. mw .. "x" .. mh .. " (should be non-zero)")
win:minimize(false)
win:focus()

-- render layer hook counters
local preOpaqueCount      = 0
local postOpaqueCount     = 0
local preTranslucentCount = 0
local postTranslucentCount= 0

hook.add("preDrawOpaque",      "test_win_pre_opaque",      function() preOpaqueCount      = preOpaqueCount      + 1 end)
hook.add("postDrawOpaque",     "test_win_post_opaque",     function() postOpaqueCount     = postOpaqueCount     + 1 end)
hook.add("preDrawTranslucent", "test_win_pre_translucent", function() preTranslucentCount = preTranslucentCount + 1 end)
hook.add("postDrawTranslucent","test_win_post_translucent",function() postTranslucentCount= postTranslucentCount+ 1 end)

local startTime = timer.realtime()
local printed   = {}

hook.add("render", "test_windows", function()
    local elapsed = timer.realtime() - startTime

    local hov = win:isHovering()
    local foc = win:isFocused()
    local key = tostring(hov) .. tostring(foc)

    if not printed[key] then
        printed[key] = true
        print("[windows] hovering=" .. tostring(hov) .. "  focused=" .. tostring(foc))
    end

    win:selectRender()
    render.clear(Color(20, 20, 40, 255))

    local t = elapsed / 1000
    local r = math.floor(128 + 127 * math.sin(t))
    local g = math.floor(128 + 127 * math.sin(t + 2))
    local b = math.floor(128 + 127 * math.sin(t + 4))

    render.setColor(Color(r, g, b, 255))

    local cw, ch = win:getSize()
    render.drawRect(cw / 2 - 60, ch / 2 - 30, 120, 60)

    win:stopRender()
    win:update()

    if elapsed >= WIN_DURATION then
        print("[windows] render layer hook fires — preOpaque="   .. preOpaqueCount
              .. "  postOpaque="      .. postOpaqueCount
              .. "  preTranslucent="  .. preTranslucentCount
              .. "  postTranslucent=" .. postTranslucentCount)
        print("[windows] PASS (window stayed open for " .. WIN_DURATION .. "ms)")
        quit()
    end
end)

while true do
    hooks.run()
end
