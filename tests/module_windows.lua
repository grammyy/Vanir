require("vanir")

-- test_windows.lua
-- tests: createWindow, getSize, getTitle, getID, isHovering, isFocused,
--        setTitle, setPos, getPos, setSize, setOpacity, setAlwaysOnTop,
--        minimize + getSize while minimized (should keep last size), focus
-- window stays open for ~8 seconds so you can see it

local WIN_DURATION=8000  -- ms

local win=windows.createWindow(400,300,500,300,"windows test")

print("[windows] created window: "..win:getTitle())
print("[windows] id: "..tostring(win:getID()))

local w,h=win:getSize()
print("[windows] initial size: "..w.."x"..h)

-- setTitle
win:setTitle("windows test (renamed)")
print("[windows] title after rename: "..win:getTitle())

-- setPos / getPos
win:setPos(200,150)
local px,py=win:getPos()
print("[windows] pos after setPos: "..px..", "..py)

-- setSize (resize to 600x400)
win:setSize(600,400)

-- setOpacity
win:setOpacity(0.92)

-- setAlwaysOnTop
win:setAlwaysOnTop(true)
win:setAlwaysOnTop(false)

-- minimize and check getSize still returns something sane
win:minimize(true)
local mw,mh=win:getSize()
print("[windows] size while minimized: "..mw.."x"..mh.." (should be non-zero)")
win:minimize(false)
win:focus()

local startTime=timer.realtime()
local printed={}

hook.add("render","test_windows",function()
    local elapsed=timer.realtime()-startTime

    -- log hovering/focused once per state change
    local hov=win:isHovering()
    local foc=win:isFocused()
    local key=tostring(hov)..tostring(foc)

    if not printed[key] then
        printed[key]=true
        print("[windows] hovering="..tostring(hov).."  focused="..tostring(foc))
    end

    win:selectRender()
    render.clear(Color(20,20,40,255))

    -- draw a pulsing rect so there's something visible
    local t=elapsed/1000
    local r=math.floor(128+127*math.sin(t))
    local g=math.floor(128+127*math.sin(t+2))
    local b=math.floor(128+127*math.sin(t+4))

    render.setColor(Color(r,g,b,255))

    local cw,ch=win:getSize()
    render.drawRect(cw/2-60,ch/2-30,120,60)

    -- countdown text would go here once font is implemented
    win:stopRender()
    win:update()

    if elapsed>=WIN_DURATION then
        print("[windows] PASS (window stayed open for "..WIN_DURATION.."ms)")
        quit()
    end
end)

while true do
    hooks.run()
end
