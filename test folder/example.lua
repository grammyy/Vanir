require("vanir")
--requiredir("test")

--hooks.add("think", "test", function()
--    print("hii :3")
--end)

local test={}
keyHeld=false

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

red=Color(255,0,0)

local triangle = {
    {0, 0, 0},  -- Vertex 1: (0, 0)
    {1, 1, 0},  -- Vertex 2: (0.5, 1.0)
    {1, 0, 0}  -- Vertex 3: (1.0, 0.0)
}

hooks.add("render","main",function()
    for i, window in ipairs(test) do
        if window and window:isFocused() then
            window:selectRender()
            
            render.clear(Color(0,0,0))

            x,y=window:getMousePos()
            render.drawCircle(x,y,4)

            render.drawPoly(triangle)

            if keyHeld then
                render.begin(gl.polygon)

                render.setColor(Color((timer.realtime()/10)%360,100,100):hsvToRGB()); render.drawVertex(-0.85, -0.75, 0.5)
                render.setColor(Color((120+timer.realtime()/10)%360,100,100):hsvToRGB()); render.drawVertex(0.85, -0.75, 0)
                render.setColor(Color((240+timer.realtime()/10)%360,100,100):hsvToRGB()); render.drawVertex(0, 0.75, 0)

                render.exit()
            else
                render.setColor(Color(255-((255/2)*math.cos(timer.realtime()/1000)),255,255))

                render.drawCircle(0,0,20,100)

                for i=1, 500 do
                    render.drawLine(math.cos((timer.realtime()/1000)+i), -1.0, math.sin((timer.realtime()/1000)+i), 1.0, math.sin(timer.realtime()/1000)*5)
                end
            end

            window:update()
            
            window:stopRender()
        end
    end
end)

while true do
    keyHeld = input.getKey(85)
    
    if keyHeld then
        print("Key is held down")
    else
        --print("Key is not held down")
    end

    hooks.run()
end