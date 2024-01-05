require("vanir")
requiredir("test")

hooks.add("think", "test", function()
    print("hii :3")
end)

while true do
    local keyHeld = input.getKey(85)
    
    if keyHeld then
        print("Key is held down")
    else
        --print("Key is not held down")
    end

    hooks.run("think")
end