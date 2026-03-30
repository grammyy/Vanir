-- renders symbol cards (functions, structs, enums, externs, macros) to HTML.

local M = {}

-- ↓ render the Lua signature span for a function card ↓
local function luaArgSignature(luaArgs, isNoiseName)
    if #luaArgs == 0 then return "" end

    local parts = {}

    for _, a in ipairs(luaArgs) do
        local hasName = a.name and not isNoiseName(a.name)

        if hasName then
            parts[#parts + 1] =
                '<span class="lsig-type">' .. a.type .. '</span>' ..
                ' <span class="lsig-name">' .. a.name .. '</span>'
        else
            parts[#parts + 1] =
                '<span class="lsig-type">' .. a.type .. '</span>'
        end
    end

    return '(<span class="lsig">' ..
           table.concat(parts, '<span class="lsig-sep">, </span>') ..
           '</span>)'
end

-- ↓ plain-text version of the Lua signature (used in search index) ↓
local function luaArgSignatureText(luaArgs, isNoiseName)
    if #luaArgs == 0 then return "" end

    local parts = {}

    for _, a in ipairs(luaArgs) do
        local hasName = a.name and not isNoiseName(a.name)

        if hasName then
            parts[#parts + 1] = a.type .. " " .. a.name
        else
            parts[#parts + 1] = a.type
        end
    end

    return "(" .. table.concat(parts, ", ") .. ")"
end

-- ↓ render the Lua stack args block inside a card body ↓
local function luaArgsBodyHTML(esc, luaArgs, infoEntry, isNoiseName)
    if #luaArgs == 0 and (not infoEntry or not infoEntry.args) then
        return ""
    end

    local parts = {}

    for _, a in ipairs(luaArgs) do
        local hasName = a.name and not isNoiseName(a.name)
        local desc    = hasName and infoEntry and infoEntry.args and infoEntry.args[a.name] or nil

        parts[#parts + 1] =
            '<div class="lua-arg">' ..
            '<span class="lua-arg-idx">'  .. a.idx          .. '</span>' ..
            '<span class="lua-arg-type">' .. esc(a.type)    .. '</span>' ..
            (hasName and ('<span class="lua-arg-name">' .. esc(a.name) .. '</span>') or '') ..
            (desc    and ('<span class="lua-arg-desc">' .. esc(desc)   .. '</span>') or '') ..
            '</div>'
    end

    if infoEntry and infoEntry.args and infoEntry.args["return"] then
        parts[#parts + 1] =
            '<div class="lua-arg lua-arg-ret">' ..
            '<span class="lua-arg-idx">→</span>' ..
            '<span class="lua-arg-type">return</span>' ..
            '<span class="lua-arg-desc">' .. esc(infoEntry.args["return"]) .. '</span>' ..
            '</div>'
    end

    if #parts == 0 then return "" end

    return
        '<div class="lua-args-block">' ..
        '<div class="lua-args-label">Lua stack args</div>' ..
        table.concat(parts, "") ..
        '</div>'
end

-- ↓ render the C parameter block inside a card body ↓
local function cArgsBodyHTML(esc, argsRaw, infoEntry)
    if not infoEntry or not infoEntry.args then
        return ""
    end

    local parts   = {}
    local argList = {}

    if argsRaw and argsRaw ~= "" and argsRaw ~= "void" then
        for arg in (argsRaw .. ","):gmatch("([^,]+),") do
            arg = arg:match("^%s*(.-)%s*$")

            if arg ~= "" then
                argList[#argList + 1] = arg
            end
        end
    end

    for _, arg in ipairs(argList) do
        local name = arg:match("([%w_]+)%s*$")
        local desc = name and infoEntry.args[name]

        if desc then
            parts[#parts + 1] =
                '<div class="lua-arg">' ..
                '<span class="lua-arg-type">' .. esc(arg)  .. '</span>' ..
                '<span class="lua-arg-desc">' .. esc(desc) .. '</span>' ..
                '</div>'
        end
    end

    if infoEntry.args["return"] then
        parts[#parts + 1] =
            '<div class="lua-arg lua-arg-ret">' ..
            '<span class="lua-arg-idx">→</span>' ..
            '<span class="lua-arg-type">return</span>' ..
            '<span class="lua-arg-desc">' .. esc(infoEntry.args["return"]) .. '</span>' ..
            '</div>'
    end

    if #parts == 0 then return "" end

    return
        '<div class="lua-args-block">' ..
        '<div class="lua-args-label">Parameters</div>' ..
        table.concat(parts, "") ..
        '</div>'
end

-- ↓ render a single function card ↓
function M.funcCard(deps, fn, filePath, info, isLua)
    local esc          = deps.esc
    local fileAnchor   = deps.fileAnchor
    local symbolAnchor = deps.symbolAnchor
    local buildSearch  = deps.buildSearchText
    local refsHTML     = deps.refsHTML
    local innerHTMLfn  = deps.innerHTML
    local extractArgs  = deps.extractLuaArgs
    local isNoiseName  = deps.isNoiseName

    local infoE    = info[fn.name]
    local desc     = infoE and infoE.description
    local note     = infoE and infoE.note
    local luaArgs  = isLua and extractArgs(fn.body) or {}
    local sig      = isLua and luaArgSignature(luaArgs, isNoiseName) or ""
    local sigText  = isLua and luaArgSignatureText(luaArgs, isNoiseName) or ""

    local kindCls = isLua and "kind-lua"             or "kind-func"
    local kindLbl = isLua and "lua"                  or "func"
    local cardCls = isLua and "symbol-card lua-card" or "symbol-card"

    local symbolId = symbolAnchor(filePath, isLua and "lua" or "func", fn.name, fn.line)

    local search = buildSearch(
        fn.name,
        fn.comment or "",
        desc or "",
        note or "",
        sigText,
        fn.ret or "",
        filePath or ""
    )

    local parts = {}
    local function w(s) parts[#parts + 1] = s end

    w('<div class="' .. cardCls .. '" id="' .. symbolId ..
      '" data-kind="' .. (isLua and "lua" or "func") ..
      '" data-file="' .. esc(fileAnchor(filePath)) ..
      '" data-filepath="' .. esc(filePath) ..
      '" data-symbol="' .. esc(symbolId) ..
      '" data-search="' .. esc(search) .. '">')

    w('<div class="symbol-header">')
    w('<span class="symbol-kind ' .. kindCls .. '">' .. kindLbl .. '</span>')
    w('<span class="symbol-name">' .. esc(fn.name) .. '</span>')

    if sig ~= "" then
        w('<span class="symbol-lua-sig">' .. sig .. '</span>')
    elseif fn.ret and fn.ret ~= "" then
        w('<span class="symbol-ret">→ ' .. esc(fn.ret) .. '</span>')
    end

    w('<span class="symbol-line">line ' .. esc(fn.line) .. '</span>')
    w('</div>') -- .symbol-header

    w('<div class="symbol-body">')

    if fn.comment and fn.comment ~= "" then
        w('<div class="symbol-comment">// ' .. esc(fn.comment) .. '</div>')
    end

    if desc then
        w('<div class="symbol-description">' .. esc(desc) .. '</div>')
    end

    if isLua then
        w(luaArgsBodyHTML(esc, luaArgs, infoE, isNoiseName))
    else
        w(cArgsBodyHTML(esc, fn.args, infoE))
    end

    w(innerHTMLfn(fn.body, fn.inner))

    if fn.refs and #fn.refs > 0 then
        local rh = refsHTML(fn.refs, filePath)

        if rh ~= "" then
            w('<div class="refs"><span class="refs-label">cross-refs:</span>' .. rh .. '</div>')
        end
    end

    if note then
        w('<div class="note-block">⚠ ' .. esc(note) .. '</div>')
    end

    w('</div></div>') -- .symbol-body, card

    return table.concat(parts, "\n")
end

return M