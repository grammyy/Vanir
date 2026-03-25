require("vanir")

local WIN_DURATION = 8000  -- run 8 seconds for multiple switches
local win = windows.createWindow(300, 200, 640, 480, "shader stress test")

-- two shaders: one inverts colors, one tints red
local WAVE_SHADER = [[
struct Viewport { width: f32, height: f32 }
@group(0) @binding(0) var<uniform> viewport: Viewport;

struct VertIn  { @location(0) pos : vec3f, @location(1) color : vec4f }
struct VertOut { @builtin(position) pos : vec4f, @location(0) color : vec4f }

@vertex
fn vs_main(v: VertIn) -> VertOut {
    var out: VertOut;
    let ndcX =  (v.pos.x / viewport.width)  * 2.0 - 1.0;
    let ndcY = -(v.pos.y / viewport.height) * 2.0 + 1.0;
    out.pos   = vec4f(ndcX, ndcY, v.pos.z, 1.0);
    out.color = v.color;
    return out;
}

@fragment
fn fs_main(in: VertOut) -> @location(0) vec4f {
    return vec4f(1.0 - in.color.r, 1.0 - in.color.g, 1.0 - in.color.b, in.color.a);
}
]]

local RED_TINT_SHADER = [[
struct Viewport { width: f32, height: f32 }
@group(0) @binding(0) var<uniform> viewport: Viewport;

struct VertIn  { @location(0) pos : vec3f, @location(1) color : vec4f }
struct VertOut { @builtin(position) pos : vec4f, @location(0) color : vec4f }

@vertex
fn vs_main(v: VertIn) -> VertOut {
    var out: VertOut;
    let ndcX =  (v.pos.x / viewport.width)  * 2.0 - 1.0;
    let ndcY = -(v.pos.y / viewport.height) * 2.0 + 1.0;
    out.pos   = vec4f(ndcX, ndcY, v.pos.z, 1.0);
    out.color = v.color;
    return out;
}

@fragment
fn fs_main(in: VertOut) -> @location(0) vec4f {
    return vec4f(in.color.r, 0.2, 0.2, in.color.a);
}
]]

-- compile shaders
local ok1 = shader.compile("wave", WAVE_SHADER)
local ok2 = shader.compile("red_tint", RED_TINT_SHADER)

if not ok1 or not ok2 then
    print("[shader] FAIL: shader compilation failed")
    quit()
end

local startTime = timer.realtime()
local switchCount = 0
local resizeDone = false

-- safe wrapper for hooks.run() to prevent undefined errors on Lua exceptions
local function safeHooksRun()
    if hooks and hooks.run then
        local success, err = pcall(hooks.run)
        if not success then
            print("[Hook ERROR] " .. tostring(err))
        end
    end
end

hook.add("render", "stress_test_shader", function()
    local elapsed = timer.realtime() - startTime

    -- resize window halfway through test while shader active
    if elapsed > WIN_DURATION / 2 and not resizeDone then
        win:setSize(800, 600)
        resizeDone = true
    end

    -- switch shaders multiple times per frame
    if shader.exists("wave") and switchCount % 2 == 0 then
        shader.setActive("wave")
    else
        shader.setActive("red_tint")
    end

    switchCount = switchCount + 1

    win:selectRender()
    render.clear(Color(10, 10, 20, 255))

    -- interleaved draw calls (lines + triangles)
    render.setColor(Color(255, 0, 0, 255))
    render.drawRect(50, 50, 200, 100)        -- tris

    render.setColor(Color(0, 255, 0, 255))
    render.drawLine(0, 0, 640, 480)          -- line

    render.setColor(Color(0, 0, 255, 255))
    render.drawFilledCircle(400, 300, 80, 32) -- tris

    render.setColor(Color(255, 255, 255, 180))
    render.drawLine(0, 480, 640, 0)          -- line

    -- release active shader while it's still selected halfway
    if shader.exists("wave") and elapsed > WIN_DURATION / 4 and elapsed < WIN_DURATION / 2 then
        shader.release("wave")
    end

    win:stopRender()
    win:update()

    if elapsed >= WIN_DURATION then
        -- cleanup
        shader.release("red_tint")
        quit()
    end
end)

-- main loop with safe hooks execution
while true do
    safeHooksRun()
end
