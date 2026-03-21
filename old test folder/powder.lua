require("vanir")

local window=windows.createWindow(400,400,400,200,"powder")
local width, height=400, 200
local particles = {}
local gravity = 0.05
local particleRadius = 5
local restitution = 0.5 -- Damping factor for collisions (0-1)
local friction = 0.99 -- Friction to slow down particles over time

local function createParticle(x, y)
    return {x = x, y = y, vx = 0, vy = 0}
end

local function distance(p1, p2)
    local dx = p2.x - p1.x
    local dy = p2.y - p1.y
    return math.sqrt(dx * dx + dy * dy), dx, dy
end

local function resolveCollision(p1, p2)
    local dist, dx, dy = distance(p1, p2)
    if dist < particleRadius * 2 then
        local nx = dx / dist
        local ny = dy / dist

        local overlap = (particleRadius * 2 - dist)

        p1.x = p1.x - nx * (overlap / 2)
        p1.y = p1.y - ny * (overlap / 2)
        p2.x = p2.x + nx * (overlap / 2)
        p2.y = p2.y + ny * (overlap / 2)

        local vx = p2.vx - p1.vx
        local vy = p2.vy - p1.vy

        local vn = vx * nx + vy * ny

        if vn > 0 then return end

        local impulse = (-(1 + restitution) * vn) / 2

        local ix = impulse * nx
        local iy = impulse * ny
        p1.vx = p1.vx - ix
        p1.vy = p1.vy - iy
        p2.vx = p2.vx + ix
        p2.vy = p2.vy + iy
    end
end

local function updateParticles()
    for i = 1, #particles do
        local p1 = particles[i]

        -- Apply gravity
        p1.vy = p1.vy + gravity

        -- Apply friction
        p1.vx = p1.vx * friction
        p1.vy = p1.vy * friction

        -- Update position
        p1.x = p1.x + p1.vx
        p1.y = p1.y + p1.vy

        -- Boundary collision
        if p1.x < particleRadius then
            p1.x = particleRadius
            p1.vx = -p1.vx * restitution
        elseif p1.x > width - particleRadius then
            p1.x = width - particleRadius
            p1.vx = -p1.vx * restitution
        end

        if p1.y < particleRadius then
            p1.y = particleRadius
            p1.vy = -p1.vy * restitution
        elseif p1.y > height - particleRadius then
            p1.y = height - particleRadius
            p1.vy = -p1.vy * restitution
        end

        -- Check for collisions with other particles
        for j = i + 1, #particles do
            local p2 = particles[j]
            resolveCollision(p1, p2)
        end
    end
end

local function drawParticles()
    for _, particle in ipairs(particles) do
        render.setColor(Color((particle.x+particle.y+timer.realtime()/10)%360,255,360):hsvToRGB())
        render.drawCircle(particle.x, particle.y, particleRadius)
    end
end

hook.add("render","main",function()
    window:selectRender()
    
    render.clear(Color(0,0,0,255))

    x,y=window:getMouse()
    width,height=window:getSize()
    width=width-particleRadius/2
    height=height-particleRadius/2
    
    if input.getKey(1) and x and y then
        table.insert(particles, createParticle(x, y))
    end
    
    updateParticles()
    drawParticles()

    window:update()
    
    window:stopRender()
end)

hook.add("onClose","",function()
    quit()
end)

while true do
    if input.getKey(69) then
        print(#particles)
    end

    hooks.run()
end
