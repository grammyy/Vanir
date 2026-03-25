require("vanir")

-- test_input.lua
-- tests: input.getKey (poll), hook inputPressed, hook inputReleased
-- window open for ~10 seconds; press any key to see it logged

local WIN_DURATION=10000  -- ms

local win=windows.createWindow(400,300,480,200,"input test  (press keys!)")

local pressLog={}
local releaseLog={}
local maxLog=6

hook.add("inputPressed","test_input",function(key)
    table.insert(pressLog,1,"pressed: "..key)

    if #pressLog>maxLog then
        pressLog[#pressLog]=nil
    end

    print("[input] pressed -> "..key)
end)

hook.add("inputReleased","test_input",function(key)
    table.insert(releaseLog,1,"released: "..key)

    if #releaseLog>maxLog then
        releaseLog[#releaseLog]=nil
    end

    print("[input] released -> "..key)
end)

local startTime=timer.realtime()

hook.add("render","test_input_render",function()
    local elapsed=timer.realtime()-startTime

    -- poll a couple of common keys each frame
    local shift=input.getKey(340)  -- GLFW_KEY_LEFT_SHIFT
    local ctrl=input.getKey(341)   -- GLFW_KEY_LEFT_CONTROL
    local space=input.getKey(32)   -- GLFW_KEY_SPACE

    win:selectRender()

    local bg=space and Color(30,60,30,255) or Color(20,20,30,255)
    render.clear(bg)

    -- indicator rects for polled keys
    render.setColor(shift and Color(0,255,120,255) or Color(60,60,60,255))
    render.drawRect(20,20,80,30)

    render.setColor(ctrl and Color(0,180,255,255) or Color(60,60,60,255))
    render.drawRect(120,20,80,30)

    render.setColor(space and Color(255,220,0,255) or Color(60,60,60,255))
    render.drawRect(220,20,80,30)

    -- progress bar
    local pct=elapsed/WIN_DURATION
    render.setColor(Color(80,80,180,255))
    render.drawRect(0,170,math.floor(480*pct),10)

    win:stopRender()
    win:update()

    if elapsed>=WIN_DURATION then
        print("[input] PASS (window closed after "..WIN_DURATION.."ms)")
        print("[input] total presses logged: "..#pressLog)
        quit()
    end
end)

while true do
    hooks.run()
end
