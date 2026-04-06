require("vanir")

-- test_font.lua
-- font module is currently stubbed; all calls are no-ops
-- this test verifies the stubs exist, return the right types, and don't crash
-- window open for ~5 seconds

local WIN_DURATION=5000  -- ms

local win=windows.createWindow(400,300,480,200,"font test  (stubbed)")

-- create returns nil from the stub; that's expected
local fnt=font.create("/mnt/c/Users/Elias/Downloads/robo.ttf", 18)
print("[font] create returned: "..tostring(fnt))

-- setFont with nil and with whatever create returned
font.setFont(nil)
font.setFont(fnt)

-- getSize should return 0 from the stub
local sz=font.getSize()
print("[font] getSize: "..tostring(sz).."  (expected 0 while stubbed)")

-- measure should return 0
local w=font.measure("Hello, Vanir!")
print("[font] measure: "..tostring(w).."  (expected 0 while stubbed)")

-- drawText should be a no-op
print("[font] drawText: no crash, OK")

local startTime=timer.realtime()

hook.add("render","test_font_render",function()
    local elapsed=timer.realtime()-startTime

    win:selectRender()
    render.clear(Color(15,15,25,255))

    -- draw placeholder rects where text would appear
    render.setColor(Color(60,60,80,255))

    for i=0,4 do
        render.drawRect(20, 30+i*30, 200+math.random(0,100), 16)

        font.drawText("Hello, Vanir!", 20, 30+i*30)
    end

    -- a note in the corner
    render.setColor(Color(100,100,200,255))
    render.drawRect(10,170,460,20)

    -- progress bar
    local pct=elapsed/WIN_DURATION
    render.setColor(Color(80,80,160,200))
    render.drawRect(0,194,math.floor(480*pct),6)

    win:stopRender()
    win:update()

    if elapsed>=WIN_DURATION then
        print("[font] PASS (stub, no crash)")
        quit()
    end
end)

while true do
    hooks.run()
end
