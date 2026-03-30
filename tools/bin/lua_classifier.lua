-- detects whether a C function is a Lua binding and extracts its stack args.

local M = {}

-- ↓ calls that indicate a function is a lua binding ↓
local LUA_ARG_CALLS = {
    "luaL_checkstring",
    "luaL_checknumber",
    "luaL_checkinteger",
    "luaL_checkudata",
    "luaL_checktype",
    "luaL_checkany",
    "luaL_checklstring",
    "luaL_optstring",
    "luaL_optnumber",
    "luaL_optinteger",
    "lua_tostring",
    "lua_tonumber",
    "lua_tointeger",
    "lua_toboolean",
    "lua_touserdata",
    "lua_isnumber",
    "lua_isstring",
    "lua_istable",
    "lua_isfunction",
    "lua_isnoneornil",
    "lua_isnone",
    "lua_isnil",
}

-- ↓ maps lua C API call names to their Lua type strings ↓
local TYPE_MAP = {
    luaL_checkstring  = "string",
    luaL_checklstring = "string",
    luaL_checknumber  = "number",
    luaL_checkinteger = "integer",
    luaL_checkudata   = "userdata",
    luaL_checktype    = "value",
    luaL_checkany     = "any",
    luaL_optstring    = "string?",
    luaL_optnumber    = "number?",
    luaL_optinteger   = "integer?",
    lua_tostring      = "string",
    lua_tonumber      = "number",
    lua_tointeger     = "integer",
    lua_toboolean     = "boolean",
    lua_touserdata    = "userdata",
    lua_isnumber      = "number",
    lua_isstring      = "string",
    lua_istable       = "table",
    lua_isfunction    = "function",
    lua_isnoneornil   = "nil?",
    lua_isnone        = "nil?",
    lua_isnil         = "nil",
}

-- ↓ maps luaL_checktype LUA_T* constants to type strings ↓
local CHECKTYPE_MAP = {
    LUA_TNONE          = "value",
    LUA_TNIL           = "nil",
    LUA_TBOOLEAN       = "boolean",
    LUA_TLIGHTUSERDATA = "lightuserdata",
    LUA_TNUMBER        = "number",
    LUA_TSTRING        = "string",
    LUA_TTABLE         = "table",
    LUA_TFUNCTION      = "function",
    LUA_TUSERDATA      = "userdata",
    LUA_TTHREAD        = "thread",
}

-- ↓ single-char variable names that are too noisy to use as argument names ↓
local NOISE_NAMES = {
    L = true, w = true, p = true, s = true,
    v = true, u = true, i = true, j = true,
}

-- ↓ parse for-loop ranges so we can resolve variable index expressions ↓
local function parseLoopRanges(body)
    local loops = {}

    for var, start, limit in body:gmatch("for%s*%(%s*int%s+([%w_]+)%s*=%s*(-?%d+)%s*;%s*.-<%s*(-?%d+)%s*;%s*%+%+%s*%1%s*%)") do
        loops[var] = { start = tonumber(start), finish = tonumber(limit) - 1 }
    end

    for var, start, limit in body:gmatch("for%s*%(%s*int%s+([%w_]+)%s*=%s*(-?%d+)%s*;%s*.-<=%s*(-?%d+)%s*;%s*%+%+%s*%1%s*%)") do
        loops[var] = { start = tonumber(start), finish = tonumber(limit) }
    end

    return loops
end

-- ↓ resolve an index expression (number, var, var+N, etc.) into a list of values ↓
local function resolveIndexExpr(expr, loops)
    expr = tostring(expr or ""):gsub("^%s*(.-)%s*$", "%1")
    expr = expr:gsub("^%((.-)%)$", "%1")
    expr = expr:gsub("%s+", "")

    local num = tonumber(expr)
    if num then return { num } end

    local var, op, off = expr:match("^([%a_][%w_]*)([+%-])(%d+)$")
    if var then
        local loop = loops[var]
        if loop then
            local delta = tonumber(off)
            if op == "-" then delta = -delta end

            local out = {}
            for i = loop.start, loop.finish do
                out[#out + 1] = i + delta
            end

            return out
        end
    end

    local num2, op2, var2 = expr:match("^(%d+)([+%-])([%a_][%w_]*)$")
    if var2 then
        local loop = loops[var2]
        if loop and op2 == "+" then
            local delta = tonumber(num2)
            local out = {}
            for i = loop.start, loop.finish do
                out[#out + 1] = delta + i
            end
            return out
        end
    end

    local var3 = expr:match("^([%a_][%w_]*)$")
    if var3 and loops[var3] then
        local loop = loops[var3]
        local out = {}
        for i = loop.start, loop.finish do
            out[#out + 1] = i
        end
        return out
    end

    return nil
end

-- ↓ fall back to "argN" for any args that didn't get a name ↓
local function finalizeLuaArgNames(luaArgs)
    for _, a in ipairs(luaArgs) do
        if not a.name or a.name == "" then
            a.name = "arg" .. tostring(a.idx)
        end
    end

    return luaArgs
end

-- ↓ returns true if the function is a Lua binding ↓
function M.isLuaFunc(fn)
    if fn.name:match("^lua%u") then
        return true
    end

    local body = fn.body or ""

    for _, call in ipairs(LUA_ARG_CALLS) do
        if body:find(call, 1, true) then
            return true
        end
    end

    return false
end

-- ↓ extract Lua stack argument types from a function body ↓
function M.extractLuaArgs(body)
    if not body or body == "" then
        return {}
    end

    local loops = parseLoopRanges(body)
    local byIdx = {}

    local function addIndexExpr(idxExpr, luaType)
        local idxs = resolveIndexExpr(idxExpr, loops)
        if not idxs then return end

        for _, idx in ipairs(idxs) do
            if idx and idx > 0 and not byIdx[idx] then
                byIdx[idx] = { idx = idx, type = luaType }
            end
        end
    end

    for idxExpr, typeConst in body:gmatch("luaL_checktype%s*%(%s*L%s*,%s*([^,]+)%s*,%s*([%w_]+)") do
        addIndexExpr(idxExpr, CHECKTYPE_MAP[typeConst] or "value")
    end

    for call, rawIdx in body:gmatch("(luaL_%w+)%s*%(%s*L%s*,%s*([^,%)]+)") do
        if call ~= "luaL_checktype" then
            local luaType = TYPE_MAP[call]
            if luaType then addIndexExpr(rawIdx, luaType) end
        end
    end

    for call, rawIdx in body:gmatch("(lua_%w+)%s*%(%s*L%s*,%s*([^,%)]+)") do
        local luaType = TYPE_MAP[call]
        if luaType then addIndexExpr(rawIdx, luaType) end
    end

    for decl, call, rawIdx in body:gmatch("([%w_%* \t]+)%s*=%s*(luaL_%w+)%s*%(%s*L%s*,%s*([^,%)]+)") do
        if call ~= "luaL_checktype" then
            local idxs    = resolveIndexExpr(rawIdx, loops)
            local varName = decl:match("([%w_]+)%s*$")

            if idxs and varName and not NOISE_NAMES[varName] then
                for _, idx in ipairs(idxs) do
                    if idx and idx > 0 and byIdx[idx] and not byIdx[idx].name then
                        byIdx[idx].name = varName
                    end
                end
            end
        end
    end

    for decl, call, rawIdx in body:gmatch("([%w_%* \t]+)%s*=%s*(lua_%w+)%s*%(%s*L%s*,%s*([^,%)]+)") do
        local idxs    = resolveIndexExpr(rawIdx, loops)
        local varName = decl:match("([%w_]+)%s*$")

        if idxs and varName and not NOISE_NAMES[varName] then
            for _, idx in ipairs(idxs) do
                if idx and idx > 0 and byIdx[idx] and not byIdx[idx].name then
                    byIdx[idx].name = varName
                end
            end
        end
    end

    local out = {}
    for _, entry in pairs(byIdx) do
        out[#out + 1] = entry
    end

    table.sort(out, function(a, b) return a.idx < b.idx end)

    return finalizeLuaArgNames(out)
end

-- ↓ returns true if a variable name is too noisy to use as an arg name ↓
function M.isNoiseName(name)
    return NOISE_NAMES[name] == true
end

return M