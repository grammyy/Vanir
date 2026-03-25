local mod = require("vanir")

print("Module version: " .. mod.version)

local ok = testt()
print("test() returned: " .. tostring(ok))

testt("custom message from lua!")

print(Color(255,0,0))
print(test)
print(test.test)

--hook.add("think", "test2", function()
--    print("meow2 >" .. os.clock())
--end)
--
--hook.add("think", "test", function()
--    print("meow >" .. os.clock())
--end)
--
--hook.remove("think", "test2")

test = windows.createWindow(400,400,400,200,"nya")
x, y = nil

hook.add("inputPressed", "test", function(key)
    print(key)

    if (key == 69) then 
        --window("test")
    
        --print(test:isHovering())
    end
end)

hook.add("render", "main", function()
    x, y = test:getMouse()
    width, height = test:getSize()

    test:selectRender()
    
    render.clear(Color(255,0,0,255))

    for i=1, 500 do
        render.drawLine(math.cos((timer.realtime()/1000)+i)*width+width, 0, math.sin((timer.realtime()/1000)+i)*width+width, height, math.sin(timer.realtime()/1000)*5)
    end

    test:stopRender()
end)

while true do
    keyHeld = input.getKey(69)
    
    if keyHeld then
        test:update()

        print(timer.realtime() .. " > Key 69 is held down > " .. tostring(x) .. ", " .. tostring(y))
    end

    hooks.run()
end
