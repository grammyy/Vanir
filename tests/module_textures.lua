require("vanir")

-- module_textures.lua
-- tests: textures.load, textures.getSize, tex.path, tex.fileSize,
--        tex:setImage (reload same file), render.setTexture,
--        render.drawTexturedRect

local WIN_DURATION = 8000  -- ms

local win = windows.createWindow(200, 200, 800, 600, "Texture Test")

-- load the bundled test image
local tex = textures.load("./tests/bleh.jpg", "bleh")

if not tex then
    print("[texture] FAIL: failed to load bleh.jpg")
    quit()
end

print("[texture] loaded: " .. tostring(tex.path))
print("[texture] fileSize: " .. tostring(tex.fileSize))

-- basic field checks
assert(type(tex.path)     == "string" and #tex.path > 0, "tex.path should be a non-empty string")
assert(type(tex.fileSize) == "number" and tex.fileSize > 0, "tex.fileSize should be > 0")

local imgW, imgH = textures.getSize(tex)
assert(type(imgW) == "number" and imgW > 0, "image width should be > 0")
assert(type(imgH) == "number" and imgH > 0, "image height should be > 0")
print("[texture] size: " .. imgW .. "x" .. imgH)

-- setImage: reload the same file to exercise the code path without
-- requiring a second image on disk
local reloadDone = false

local startTime = timer.realtime()

hook.add("render", "texture_test", function()
    local elapsed = timer.realtime() - startTime

    -- reload at 2.5 s
    if elapsed > 2500 and not reloadDone then
        reloadDone = true
        tex:setImage("./tests/bleh.jpg")
        print("[texture] setImage reload: OK")
    end

    local w, h = win:getSize()

    win:selectRender()
    render.clear(Color(20, 20, 30, 255))

    render.setTexture(tex)

    -- full-window stretch
    render.drawTexturedRect(0, 0, imgW, imgH, 0, 0, w, h)

    -- half-size inset in top-left corner
    render.drawTexturedRect(0, 0, imgW, imgH, 0, 0, w / 2, h / 2)

    win:stopRender()
    win:update()

    if elapsed > WIN_DURATION then
        print("[texture] PASS  fileSize=" .. tex.fileSize)
        quit()
    end
end)

while true do
    hooks.run()
end
