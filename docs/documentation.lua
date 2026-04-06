-- helpers live under tools/bin/:
--   json.lua           — JSON parser
--   lua_classifier.lua — Lua binding detection and arg extraction
--   html_helpers.lua   — escaping, anchors, search text
--   inner_comments.lua — annotated comment block extraction and rendering
--   sidebar.lua        — file-tree HTML
--   card_renderer.lua  — symbol card HTML

-- module resolution ↓↓↓ module resolution
-- ↓ support running from either the project root or from tools/ directly ↓
local function findHelpersDir()
    local candidates = {
        "tools/bin",
        "bin",
        "../tools/bin",
    }

    for _, dir in ipairs(candidates) do
        local f = io.open(dir .. "/json.lua", "r")

        if f then
            f:close()
            return dir
        end
    end

    return nil
end

local helpersDir = findHelpersDir()

if not helpersDir then
    io.stderr:write("[documentation] error: could not find bin/ directory\n")
    io.stderr:write("[documentation] run from the project root or from tools/\n")
    os.exit(1)
end

io.write("[documentation] loading helpers from: " .. helpersDir .. "\n")

-- ↓ add helpers dir to package.path so require() works ↓
package.path = helpersDir .. "/?.lua;" .. package.path

local json = require("json")
local classifier = require("lua_classifier")
local htmlH = require("html_helpers")
local innerC = require("inner_comments")
local sidebar = require("sidebar")
local cardRenderer = require("card_renderer")

io.write("[documentation] helpers loaded OK\n")
-- module resolution ↑↑↑ module resolution

-- ↓ info.txt parser ↓
local function parseInfoFile(path)
    local f = io.open(path, "r")

    if not f then
        io.write("[documentation] info.txt not found at: " .. path .. " (skipping)\n")
        return {}, {}
    end

    local info     = {}
    local excludes = {}
    local cur      = nil
    local inExclude = false

    for line in f:lines() do
        line = line:gsub("\r", "")

        if line:match("^%s*#") then
            -- ↓ comment line, skip ↓
        elseif line:match("^%[(.+)%]%s*$") then
            local section = line:match("^%[(.+)%]%s*$")
            inExclude     = section == "exclude"

            if not inExclude then
                cur           = {}
                info[section] = cur
            else
                cur = nil
            end
        elseif inExclude then
            -- ↓ collect exclude file patterns ↓
            local val = line:match("^%s*file%s*=%s*(.-)%s*$")

            if val and val ~= "" then
                excludes[#excludes + 1] = val
            end
        elseif cur then
            local argName, argVal = line:match("^%s*arg:(%S+)%s*=%s*(.+)$")

            if argName then
                cur.args          = cur.args or {}
                cur.args[argName] = argVal:match("^%s*(.-)%s*$")
            else
                local k, v = line:match("^%s*(%w+)%s*=%s*(.+)$")

                if k then
                    cur[k] = v:match("^%s*(.-)%s*$")
                end
            end
        end
    end

    f:close()

    local count = 0
    for _ in pairs(info) do count = count + 1 end

    io.write("[documentation] loaded info.txt: " .. count .. " entries, " .. #excludes .. " excludes\n")

    return info, excludes
end

-- ↓ convert a glob pattern (* wildcard) to a Lua pattern ↓
local function globToPattern(g)
    return "^" .. g:gsub("([%.%+%-%^%$%(%)%[%]%%])", "%%%1"):gsub("%*", ".*") .. "$"
end

-- ↓ returns true when filePath matches any exclude pattern ↓
local function isExcluded(filePath, patterns)
    local norm = tostring(filePath or ""):gsub("\\", "/")

    for _, pat in ipairs(patterns) do
        if norm:match(pat) then
            return true
        end
    end

    return false
end

-- ↓ filter data.folders in-place, removing any files listed under [exclude] ↓
local function applyExcludes(data, excludes)
    if not excludes or #excludes == 0 then
        return
    end

    -- ↓ compile globs to Lua patterns once ↓
    local patterns = {}

    for _, g in ipairs(excludes) do
        patterns[#patterns + 1] = globToPattern(g)
    end

    local removed = 0

    for _, folder in ipairs(data.folders or {}) do
        local kept = {}

        for _, file in ipairs(folder.files or {}) do
            if isExcluded(file.path, patterns) then
                io.write("[documentation] excluding: " .. tostring(file.path) .. "\n")
                removed = removed + 1
            else
                kept[#kept + 1] = file
            end
        end

        folder.files = kept
    end

    if removed > 0 then
        io.write("[documentation] excluded " .. removed .. " file(s) from output\n")
    end
end

-- ↓ dependency bundle passed into card_renderer ↓
local function makeDeps(info)
    return {
        esc            = htmlH.esc,
        fileAnchor     = htmlH.fileAnchor,
        symbolAnchor   = htmlH.symbolAnchor,
        buildSearchText = htmlH.buildSearchText,
        refsHTML       = htmlH.refsHTML,
        innerHTML      = function(body, inner)
            return innerC.innerHTML(htmlH.esc, body, inner)
        end,
        extractLuaArgs = classifier.extractLuaArgs,
        isNoiseName    = classifier.isNoiseName,
        info           = info,
    }
end

-- ↓ HTML generator ↓
local function readAsset(path)
    local f = io.open(path, "r")

    if not f then
        io.stderr:write("[documentation] warning: asset not found: " .. path .. "\n")
        return ""
    end

    local s = f:read("*a")
    f:close()

    return s
end

local function generate(data, info)
    local out  = {}
    local deps = makeDeps(info)

    local function w(s) out[#out + 1] = s end

    local allFiles = sidebar.collectAllFiles(data)
    local tree     = sidebar.buildTree(allFiles)

    -- ↓ load external CSS and JS assets ↓
    local css = readAsset(helpersDir .. "/styles.css")
    local js  = readAsset(helpersDir .. "/script.js")

    io.write("[documentation] generating HTML head...\n")

    w('<!DOCTYPE html>')
    w('<html lang="en">')
    w('<head>')
    w('<meta charset="UTF-8">')
    w('<meta name="viewport" content="width=device-width, initial-scale=1.0">')
    w('<title>Vanir — Source Documentation</title>')
    w('<style>')
    w(css)
    w('</style>')
    w('</head>')
    w('<body>')

    -- ===================================================================
    -- sidebar
    -- ===================================================================

    io.write("[documentation] generating sidebar...\n")

    w('<div id="sidebar">')
    w('<h1>Vanir Docs</h1>')
    w('<div class="gen-date">Generated: ' .. htmlH.esc(data.generated or "") .. '</div>')
    w('<div id="search-wrap"><input id="search" type="text" placeholder="Search symbols…" oninput="doSearch(this.value)"></div>')
    w(sidebar.renderTree(tree, 0))
    w('</div>') -- #sidebar

    -- ===================================================================
    -- main content
    -- ===================================================================

    io.write("[documentation] generating main content...\n")

    w('<div id="main">')
    w('<div class="meta-bar">')
    w('<h1>Vanir</h1>')
    w('<span class="sub">Source Documentation · ' .. htmlH.esc(data.generated or "") .. '</span>')
    w('</div>')

    local totalCards    = 0
    local totalFolders  = 0
    local totalFiles    = 0

    for _, folder in ipairs(data.folders or {}) do
        totalFolders = totalFolders + 1

        w('<h2 class="folder-heading">' .. htmlH.esc(folder.name) .. '</h2>')

        for _, file in ipairs(folder.files or {}) do
            totalFiles = totalFiles + 1

            local fpath   = htmlH.normPath(file.path)
            local fanchor = htmlH.fileAnchor(file.path)
            local syms    = file.symbols or {}

            w('<div class="file-block" id="' .. fanchor .. '" data-file-path="' .. htmlH.esc(fpath) .. '">')
            w('<h3 class="file-heading">' .. htmlH.esc(file.path) .. '</h3>')

            -- ↓ structs ↓
            if syms.structs and #syms.structs > 0 then
                w('<div class="section-label">Structs</div>')

                for _, s in ipairs(syms.structs) do
                    local infoE = info[s.name]
                    local desc  = infoE and infoE.description
                    local note  = infoE and infoE.note

                    w('<div class="symbol-card" onclick="toggle(this)">')
                    w('<div class="symbol-header">')
                    w('<span class="symbol-kind kind-struct">' .. htmlH.esc(s.kind or "struct") .. '</span>')
                    w('<span class="symbol-name">' .. htmlH.esc(s.name) .. '</span>')
                    w('<span class="symbol-line">line ' .. htmlH.esc(s.line) .. '</span>')
                    w('</div>')
                    w('<div class="symbol-body">')

                    if s.comment and s.comment ~= "" then
                        w('<div class="symbol-comment">// ' .. htmlH.esc(s.comment) .. '</div>')
                    end

                    if desc then
                        w('<div class="symbol-description">' .. htmlH.esc(desc) .. '</div>')
                    end

                    w(innerC.innerHTML(htmlH.esc, nil, s.inner))

                    if s.refs and #s.refs > 0 then
                        local rh = htmlH.refsHTML(s.refs, file.path)

                        if rh ~= "" then
                            w('<div class="refs"><span class="refs-label">cross-refs:</span>' .. rh .. '</div>')
                        end
                    end

                    if note then
                        w('<div class="note-block">⚠ ' .. htmlH.esc(note) .. '</div>')
                    end

                    w('</div></div>')
                    totalCards = totalCards + 1
                end
            end

            -- ↓ enums ↓
            if syms.enums and #syms.enums > 0 then
                w('<div class="section-label">Enums</div>')

                for _, e in ipairs(syms.enums) do
                    local infoE = info[e.name]
                    local desc  = infoE and infoE.description
                    local note  = infoE and infoE.note

                    w('<div class="symbol-card" onclick="toggle(this)">')
                    w('<div class="symbol-header">')
                    w('<span class="symbol-kind kind-enum">enum</span>')
                    w('<span class="symbol-name">' .. htmlH.esc(e.name) .. '</span>')
                    w('<span class="symbol-line">line ' .. htmlH.esc(e.line) .. '</span>')
                    w('</div>')
                    w('<div class="symbol-body">')

                    if e.comment and e.comment ~= "" then
                        w('<div class="symbol-comment">// ' .. htmlH.esc(e.comment) .. '</div>')
                    end

                    if desc then
                        w('<div class="symbol-description">' .. htmlH.esc(desc) .. '</div>')
                    end

                    if e.members and #e.members > 0 then
                        w('<div class="enum-members">')

                        for _, m in ipairs(e.members) do
                            w('<span class="enum-member">' .. htmlH.esc(m) .. '</span>')
                        end

                        w('</div>')
                    end

                    w(innerC.innerHTML(htmlH.esc, nil, e.inner))

                    if e.refs and #e.refs > 0 then
                        local rh = htmlH.refsHTML(e.refs, file.path)

                        if rh ~= "" then
                            w('<div class="refs"><span class="refs-label">cross-refs:</span>' .. rh .. '</div>')
                        end
                    end

                    if note then
                        w('<div class="note-block">⚠ ' .. htmlH.esc(note) .. '</div>')
                    end

                    w('</div></div>')
                    totalCards = totalCards + 1
                end
            end

            -- ↓ functions: split lua bindings vs internal C ↓
            if syms.funcs and #syms.funcs > 0 then
                local luaFuncs = {}
                local cFuncs   = {}

                for _, fn in ipairs(syms.funcs) do
                    if classifier.isLuaFunc(fn) then
                        luaFuncs[#luaFuncs + 1] = fn
                    else
                        cFuncs[#cFuncs + 1] = fn
                    end
                end

                if #luaFuncs > 0 or #cFuncs > 0 then
                    w('<div class="section-label">Functions</div>')
                end

                if #luaFuncs > 0 then
                    w('<div class="func-divider">')
                    w('<span class="func-divider-lbl fd-lua">Lua</span>')
                    w('<span class="func-divider-line"></span>')
                    w('</div>')

                    for _, fn in ipairs(luaFuncs) do
                        w(cardRenderer.funcCard(deps, fn, file.path, info, true))
                        totalCards = totalCards + 1
                    end
                end

                if #cFuncs > 0 then
                    w('<div class="func-divider">')
                    w('<span class="func-divider-lbl fd-c">C Internal</span>')
                    w('<span class="func-divider-line"></span>')
                    w('</div>')

                    for _, fn in ipairs(cFuncs) do
                        w(cardRenderer.funcCard(deps, fn, file.path, info, false))
                        totalCards = totalCards + 1
                    end
                end
            end

            -- ↓ externs ↓
            if syms.externs and #syms.externs > 0 then
                w('<div class="section-label">Externs</div>')

                for _, ex in ipairs(syms.externs) do
                    local infoE = info[ex.name]
                    local desc  = infoE and infoE.description

                    w('<div class="symbol-card" onclick="toggle(this)">')
                    w('<div class="symbol-header">')
                    w('<span class="symbol-kind kind-extern">extern</span>')
                    w('<span class="symbol-name">' .. htmlH.esc(ex.name) .. '</span>')
                    w('<span class="symbol-ret">' .. htmlH.esc(ex.decl) .. '</span>')
                    w('<span class="symbol-line">line ' .. htmlH.esc(ex.line) .. '</span>')
                    w('</div>')

                    if (ex.comment and ex.comment ~= "") or desc then
                        w('<div class="symbol-body">')

                        if ex.comment and ex.comment ~= "" then
                            w('<div class="symbol-comment">// ' .. htmlH.esc(ex.comment) .. '</div>')
                        end

                        if desc then
                            w('<div class="symbol-description">' .. htmlH.esc(desc) .. '</div>')
                        end

                        w('</div>')
                    end

                    w('</div>')
                    totalCards = totalCards + 1
                end
            end

            -- ↓ macros ↓
            if syms.macros and #syms.macros > 0 then
                w('<div class="section-label">Macros</div>')

                for _, m in ipairs(syms.macros) do
                    local infoE = info[m.name]
                    local desc  = infoE and infoE.description

                    w('<div class="symbol-card" onclick="toggle(this)">')
                    w('<div class="symbol-header">')
                    w('<span class="symbol-kind kind-macro">#define</span>')
                    w('<span class="symbol-name">' .. htmlH.esc(m.name) .. '</span>')

                    if m.value and m.value ~= "" then
                        w('<span class="symbol-ret">' .. htmlH.esc(m.value:sub(1, 60)) .. '</span>')
                    end

                    w('<span class="symbol-line">line ' .. htmlH.esc(m.line) .. '</span>')
                    w('</div>')

                    if (m.comment and m.comment ~= "") or desc then
                        w('<div class="symbol-body">')

                        if m.comment and m.comment ~= "" then
                            w('<div class="symbol-comment">// ' .. htmlH.esc(m.comment) .. '</div>')
                        end

                        if desc then
                            w('<div class="symbol-description">' .. htmlH.esc(desc) .. '</div>')
                        end

                        w('</div>')
                    end

                    w('</div>')
                    totalCards = totalCards + 1
                end
            end

            w('</div>') -- .file-block
        end
    end

    w('</div>') -- #main

    io.write(
        "[documentation] content generated —" ..
        " folders=" .. totalFolders ..
        "  files="  .. totalFiles ..
        "  cards="  .. totalCards ..
        "\n"
    )

    -- ===================================================================
    -- javascript
    -- ===================================================================

    io.write("[documentation] appending JavaScript...\n")

    w('<script>')
    w(js)
    w('</script>')

    w('</body></html>')

    return table.concat(out, "\n")
end

local function main()
    local symbolsPath = "tools/symbols.json"
    local infoPath    = "tools/info.txt"
    local outPath     = "tools/docs.html"

    -- ↓ fall back to running from within tools/ ↓
    local f = io.open(symbolsPath, "r")

    if not f then
        symbolsPath = "symbols.json"
        infoPath    = "info.txt"
        outPath     = "docs.html"
        f           = io.open(symbolsPath, "r")
    end

    if not f then
        io.stderr:write("[documentation] error: symbols.json not found — run map_symbols.lua first\n")
        return
    end

    local raw = f:read("*a")
    f:close()

    io.write("[documentation] parsing symbols.json (" .. #raw .. " bytes)...\n")

    local ok, data = pcall(json.decode, raw)

    if not ok then
        io.stderr:write("[documentation] JSON parse error: " .. tostring(data) .. "\n")
        return
    end

    local folderCount = 0
    local fileCount   = 0

    for _, folder in ipairs(data.folders or {}) do
        folderCount = folderCount + 1
        fileCount   = fileCount + #(folder.files or {})
    end

    io.write(
        "[documentation] symbol map loaded —" ..
        " generated=" .. (data.generated or "?") ..
        "  folders="  .. folderCount ..
        "  files="    .. fileCount ..
        "\n"
    )

    local info, excludes = parseInfoFile(infoPath)

    -- ↓ remove excluded files from the symbol data before generating HTML ↓
    applyExcludes(data, excludes)

    io.write("[documentation] generating HTML...\n")

    local html = generate(data, info)

    local outFile = io.open(outPath, "w")

    if not outFile then
        io.stderr:write("[documentation] error: could not write to: " .. outPath .. "\n")
        return
    end

    outFile:write(html)
    outFile:close()

    io.write("[documentation] wrote: " .. outPath .. " (" .. #html .. " bytes)\n")
    io.write("[documentation] done.\n")
end

main()