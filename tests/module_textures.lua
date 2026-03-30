require("vanir")

local win = windows.createWindow(200, 200, 800, 600, "Texture Test")

-- load the texture from file
local tex = textures.load("./tests/bleh.jpg", "bleh")

if not tex then
    print("[texture] FAIL: failed to load bleh.jpg")
    quit()
end

print("[texture] loaded bleh.jpg")

local startTime = timer.realtime()

hook.add("render", "texture_test", function()
    local elapsed = timer.realtime() - startTime

    -- make the texture scale with the window size

    local w,h = win:getSize()

    win:selectRender()
    render.clear(Color(20, 20, 30, 255))

    if elapsed > 2500 and tex.path == "./tests/bleh.jpg" then
        if tex.path ~= "C:/Users/Elias/Downloads/09384ae4f45f1dfe4ccfcc19502f8cde.png" then
            print(tex.path, tex.fileSize)
        end

        --tex:setImage("C:/Users/Elias/Downloads/09384ae4f45f1dfe4ccfcc19502f8cde.png")
    end

    -- draw texture centered
    local imgW, imgH = textures.getSize(tex)
    --local imgW=200
    --local imgH=200

    render.setTexture(tex)

    -- draw textured rect
    render.drawTexturedRect(0, 0, imgW, imgH, 0, 0, w, h)
render.drawTexturedRect(0, 0, imgW, imgH, 0, 0, w/2, h/2)
    -- optional: draw an overlay box around it

    win:stopRender()
    win:update()

    -- quit after 8 seconds
    if elapsed > 8000 then
        print("[texture] PASS " .. tex.fileSize)
        quit()
    end
end)

while true do
    hooks.run()
end
