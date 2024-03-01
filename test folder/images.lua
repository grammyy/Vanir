require("vanir")
requiredir("test")

window = windows.createWindow(400,400,400,200,"nya")
toggle = false
test=render.newTexture("lol"):loadImage("bleh.jpg")

hooks.add("inputPressed", "", function(key)
    if key==85 and window:isFocused() then
        toggle = not toggle
    end
end)

hooks.add("render","main",function()
    width,height=window:getSize()

    window:selectRender()
    
    render.clear(Color(0,0,0))

    if toggle then
        for i=1, 100 do
            for ii=1, 100 do
                render.setColor(Color((i+ii+(timer.realtime()/10))%360,100,100):hsvToRGB())
                
                render.drawRect((width/100)*(i-1), (height/100)*(ii-1), (width/100), (height/100))
            end
        end
    else
        render.selectTexture(gl.texture2D, "lol")
        render.drawTexture(10,10,300,300)
    end

    window:update()
    
    window:stopRender()
end)

hooks.add("onClose","",function()
    quit()
end)

while true do
    hooks.run()
end