require("vanir")

-- test_render.lua
-- tests: setColor, clear, drawRect, drawLine, drawCircle, drawFilledCircle,
--        drawPoly, drawVertex, setViewport, resetViewport, scissor
-- window open for ~10 seconds cycling through each draw test

local WIN_DURATION=10000  -- ms

local win=windows.createWindow(300,200,640,480,"render test")

local startTime=timer.realtime()

-- each phase lasts this long in ms
local PHASE=2000

local triangle={
    {100,350,0},
    {320,100,0},
    {540,350,0},
}

hook.add("render","test_render",function()
    local elapsed=timer.realtime()-startTime
    local phase=math.floor(elapsed/PHASE)

    win:selectRender()
    render.clear(Color(15,15,20,255))

    if phase==0 then
        -- drawRect + setColor
        render.setColor(Color(255,80,80,255))
        render.drawRect(50,50,200,120)

        render.setColor(Color(80,255,80,200))
        render.drawRect(200,150,200,120)

        render.setColor(Color(80,80,255,200))
        render.drawRect(350,80,150,200)
    elseif phase==1 then
        -- drawLine
        local t=elapsed/1000

        for i=0,11 do
            local angle=i*(math.pi/6)+t
            local cx,cy=320,240

            render.setColor(Color(i*20,255-i*15,200,255))
            render.drawLine(cx,cy, cx+math.cos(angle)*180, cy+math.sin(angle)*180)
        end

    elseif phase==2 then
        -- drawCircle + drawFilledCircle
        render.setColor(Color(255,255,100,255))
        render.drawCircle(160,240,100,64)

        render.drawFilledCircle(480,240,100,64,function(i)
            render.setColor(Color((i*4+elapsed/10)%360,100,100):hsvToRGB())
        end)

    elseif phase==3 then
        -- drawPoly
        render.drawPoly(triangle,function(i)
            render.setColor(Color((i-1)*120,100,100):hsvToRGB())
        end)

        -- solid quad via drawPoly
        render.setColor(Color(200,200,200,180))
        render.drawPoly({{20,20,0},{200,20,0},{200,100,0},{20,100,0}})

    elseif phase==4 then
        -- setViewport: draw into a sub-region, then reset
        render.setViewport(160,120,320,240)

        render.setColor(Color(255,120,0,255))
        render.drawFilledCircle(160,120,80,48)

        render.resetViewport()

        -- scissor outline in full viewport
        render.setColor(Color(255,255,255,255))
        render.drawRect(158,118,324,244)

    end

    -- phase label via colored rect in corner
    render.setColor(Color(60,60,60,200))
    render.drawRect(0,0,30,480)

    for i=0,4 do
        local col=i==phase and Color(255,220,0,255) or Color(80,80,80,255)

        render.setColor(col)
        render.drawFilledCircle(15,50+i*80,10,16)
    end
render.drawRoundedBox(20, 50, 50, 50, 50)
    win:stopRender()
    win:update()

    if elapsed>=WIN_DURATION then
        print("[render] PASS (all phases completed)")
        quit()
    end
end)

while true do
    hooks.run()
end