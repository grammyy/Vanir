require("vanir")

local window = windows.createWindow(400, 400, 600, 400, "fractal")
local width, height = 600, 400
local scale = 2

function math.clamp(value, min, max)
    if value < min then
        return min
    elseif value > max then
        return max
    else
        return value
    end
end

function math.round(value)
    return math.floor(value + 0.5)
end

local data = {
    xCartMin = -2.1,
    xCartMax = 0.8,
    yCartMin = -1.2,
    yCartMax = 1.2,
    maxEscape = math.clamp(math.floor(244 / 7) * 7, 14, 1792),
    loaded = false,
    colors = {},
    src = {
        RatioX = 1
    }
}

function pixelToCartY(y)
    return data.yCartMin + ((data.yCartMax - data.yCartMin) * (y / height))
end

function pixelToCartX(x)
    return data.xCartMin + ((data.xCartMax - data.xCartMin) * (x / (width / data.src.RatioX)))
end

function calcuEscape(xCart, yCart)
    local time = 0
    local x1, y1 = xCart, yCart
    local x2, y2

    while x1 * x1 + y1 * y1 < 4 and time < data.maxEscape do
        x2 = x1 * x1 - y1 * y1 + xCart
        y2 = 2 * x1 * y1 + yCart
        x1, y1 = x2, y2
        time = time + 1
    end

    return time
end

function generateColors()
    local increments = math.floor(data.maxEscape / 7)
    for time = 0, data.maxEscape do
        if time <= 2 then
            data.colors[time] = Color(0, 0, 0)
        elseif time == data.maxEscape then
            data.colors[time] = Color(0, 25, 0)
        else
            local case = math.floor(time / increments)
            local remain = time % increments
            local color

            if case == 0 then
                color = Color(0, math.floor(256 / increments) * remain, 0)
            elseif case == 1 then
                color = Color(0, 255, math.floor(256 / increments) * remain)
            elseif case == 2 then
                color = Color(math.floor(256 / increments) * remain, 255, 255)
            elseif case == 3 then
                color = Color(math.floor(256 / increments) * remain, 0, 255)
            elseif case == 4 then
                color = Color(255, math.floor(256 / increments) * remain, 255)
            elseif case == 5 then
                color = Color(255, math.floor(256 / increments) * remain, 0)
            elseif case == 6 then
                color = Color(255, 255, math.floor(256 / increments) * remain)
            end

            data.colors[time] = color
        end
    end
end

hook.add("render", "", function()
    x, y = window:getMouse()
    width, height = window:getSize()

    if not data.loaded then
        window:selectRender()
        render.clear(Color(0, 0, 0, 255))

        for py = 0, height / scale do
            local yCart = pixelToCartY(py * scale)
            for px = 0, (width / data.src.RatioX) / scale do
                local xCart = pixelToCartX(px * scale / data.src.RatioX)
                local time = calcuEscape(xCart, yCart)
                render.setColor(data.colors[time])
                render.drawRect(px * scale, py * scale, scale, scale)
            end
        end

        data.loaded = true
        window:stopRender()
        window:update()
    end
end)

hook.add("inputPressed", "", function(key)
    if x and data.src and key == 69 then
        local w = math.floor((width) * ((not input.getKey(79) and 0.2 or 1) / 2))
        local h = math.floor(height * ((not input.getKey(79) and 0.2 or 1) / 2))

        data.xCartMin = pixelToCartX((x / data.src.RatioX) - w)
        data.xCartMax = pixelToCartX((x / data.src.RatioX) + w)
        data.yCartMin = pixelToCartY((y) - h)
        data.yCartMax = pixelToCartY((y) + h)

        data.loaded = false
    end
end)

hook.add("onResize", "", function()
    data.loaded = false
end)

hook.add("onClose", "", function()
    quit()
end)

generateColors()

while true do
    hooks.run()
end
