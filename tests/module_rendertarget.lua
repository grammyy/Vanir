require("vanir")

-- test_rendertarget.lua

local WIN_DURATION=8000  -- ms

local win=windows.createWindow(350,250,640,480,"rendertarget test")

local scene   = render.createRenderTarget("scene",   640, 480)
local overlay = render.createRenderTarget("overlay", 200, 80)

print("[rendertarget] targets created")

local startTime=timer.realtime()

hook.add("render","test_rt",function()
    local elapsed=timer.realtime()-startTime
    local t=elapsed/1000

    -- ↓ scene RT ↓
    render.selectRenderTarget(scene)

    -- ↓ clear inside select so it uses LoadOp_Clear on this pass ↓
    render.clear(Color(
        math.floor(10+8*math.sin(t*0.5)),
        math.floor(5+5*math.sin(t*0.3)),
        math.floor(30+20*math.sin(t*0.7)),
        255
    ))

    render.drawFilledCircle(320,240,180,64,function(i)
        render.setColor(Color((i*6+elapsed/10)%360,100,100):hsvToRGB())
    end)

    local bx=40+math.floor(math.abs(math.sin(t*1.3))*560)
    local by=40+math.floor(math.abs(math.cos(t*0.9))*400)

    render.setColor(Color(255,220,80,220))
    render.drawRect(bx,by,60,30)

    render.stopRenderTarget()

    -- ↓ overlay RT ↓
    render.selectRenderTarget(overlay)
    render.clear(Color(20,20,20,200))

    local pct=elapsed/WIN_DURATION

    render.setColor(Color(80,200,80,255))
    render.drawRect(4,4,math.floor(192*pct),20)

    render.setColor(Color(255,255,255,180))
    render.drawRect(4,30,192,20)

    render.stopRenderTarget()

    win:selectRender()
    render.clear(Color(0,0,0,255))

    local ww,wh=win:getSize()

    render.setRenderTargetTexture(scene,   0,0, ww,wh, 0,0)
    render.setRenderTargetTexture(overlay, 0,0, 200,80, 10,wh-90)

    win:stopRender()
    win:update()

    if elapsed>=WIN_DURATION then
        print("[rendertarget] PASS")
        quit()
    end
end)

while true do
    hooks.run()
end
