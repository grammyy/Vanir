require("vanir")
--requiredir("test")

--hooks.add("think", "test", function()
--    print("hii :3")
--end)

local test={}

test[#test+1]=windows.createWindow(400,400,400,200,"nya")

hooks.add("inputPressed","test",function(key)
    if key==69 then
        print("pressed -> "..key)

        test[#test+1]=windows.createWindow(400,400,400,200,"nya")
    elseif key==70 then
        print("test")
        print("returning..")

        if true then
            return
        end

        print("failed :(")
    end
end)

hooks.add("inputReleased","test",function(key)
    print("released -> "..key)
end)

hooks.add("inputReleased","test2",function(key)
    print("released2 -> "..key)
end)

hooks.add("render","main",function()
    for i, window in ipairs(test) do
        window:selectRender()
    
        render.clear()
    
        render.enable(gl.blend)
        render.setBlend(gl.srcAlpha, gl["1-srcAlpha"])
        render.enable(gl.lineSmooth)
        render.setQuality(gl.lines, gl.nicest)

        render.drawLine(math.cos(timer.realtime()/1000), -1.0, math.sin(timer.realtime()/1000), 1.0)

        render.disable(gl.blend)
        render.disable(gl.lineSmooth)

        window:update()

        window:stopRender()
    end
end)

while true do
    local keyHeld = input.getKey(85)
    
    if keyHeld then
        print("Key is held down")
    else
        --print("Key is not held down")
    end

    hooks.run()
end