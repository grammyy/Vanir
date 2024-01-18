require("vanir")
--requiredir("test")

--hooks.add("think", "test", function()
--    print("hii :3")
--end)

hooks.add("inputPressed","test",function(key)
    if key==69 then
        print("pressed -> "..key)
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

while true do
    local keyHeld = input.getKey(85)

    if keyHeld then
        print("Key is held down")
    else
        --print("Key is not held down")
    end

    hooks.run()
end