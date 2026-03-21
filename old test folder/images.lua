require("vanir")
requiredir("test")

window = windows.createWindow(400,400,400,200,"nya")
toggle = false

hook.add("inputPressed", "", function(key)
    if key==85 and window:isFocused() then
        toggle = not toggle
    end

    if key==84 then
        test=render.newTexture("lol"):loadImage("IMG_1265.png")
    end

    print(key)
end)

hook.add("render","main",function()
    width,height=window:getSize()

    window:selectRender()
    
    --render.clear(Color(0,0,0))
    --render.clear(Color(0,0,0),1)

    if toggle then
        for i=1, 100 do
            for ii=1, 100 do
                render.setColor(Color((i+ii+(timer.realtime()/10))%360,100,100):hsvToRGB())
                
                render.drawRect((width/100)*(i-1), (height/100)*(ii-1), (width/100), (height/100))
            end
        end
    else
        if test then
            --print(test:getName())

            render.selectTexture(gl.texture2D, "lol")
            render.drawTexture(10,10,300,300)
        end
    end

    window:update()
    
    window:stopRender()
end)

hook.add("onClose","",function()
    quit()
end)

while true do
    hooks.run()
end
