-- scans all .c and .h files under src/, extracts functions, structs, enums,
-- typedefs, and extern declarations, then writes a JSON symbol map to
-- tools/symbols.json for documentation.lua to consume.

-- ↓ forward declarations ↓
local scanDir, readFile, parseFile, resolveRefs, writeJSON

-- filesystem helpers ↓↓↓ filesystem helpers
-- ↓ collect all .c and .h files under a directory recursively ↓
scanDir = function(root, out)
    out = out or {}

    local cmd

    if package.config:sub(1, 1) == "\\" then
        -- ↓ windows: dir /b /s ↓
        cmd = 'dir /b /s "' .. root .. '"'
    else
        cmd = 'find "' .. root .. '" -type f \\( -name "*.c" -o -name "*.h" \\) | sort'
    end

    io.write("[map_symbols] running scan command: " .. cmd .. "\n")

    local pipe = io.popen(cmd)

    if not pipe then
        io.stderr:write("[map_symbols] error: could not list directory: " .. root .. "\n")
        return out
    end

    for line in pipe:lines() do
        -- ↓ normalise path separators ↓
        line = line:gsub("\\", "/")
        out[#out + 1] = line
    end

    pipe:close()

    return out
end

readFile = function(path)
    local f = io.open(path, "r")

    if not f then
        io.stderr:write("[map_symbols] warning: could not open file: " .. path .. "\n")
        return nil
    end

    local src = f:read("*a")

    f:close()

    -- ↓ normalise CRLF ↓
    src = src:gsub("\r\n", "\n")

    return src
end
-- filesystem helpers ↑↑↑ filesystem helpers

-- ↓ collect inline comments that appear directly above a given line number ↓
local function collectLeadingComments(lines, lineIdx)
    local comments = {}

    -- ↓ walk upward from the line, skipping blank lines, collecting comments ↓
    local i = lineIdx - 1

    while i >= 1 do
        local ln = lines[i]:match("^%s*(.-)%s*$")

        if ln == "" then
            i = i - 1
        elseif ln:match("^//") then
            table.insert(comments, 1, ln:match("^//%s*(.*)"))
            i = i - 1
        elseif ln:match("^%*") or ln:match("^/%*") then
            -- ↓ inside or start of a block comment ↓
            table.insert(comments, 1, ln:gsub("^/?%*+%s?", ""):gsub("%*+/$", ""):match("^%s*(.-)%s*$"))
            i = i - 1
        elseif ln:match("%*/$") then
            -- ↓ closing */ of a block comment — walk back to /* ↓
            table.insert(comments, 1, ln:gsub("%*+/$", ""):match("^%s*(.-)%s*$"))
            i = i - 1
        elseif ln:match("^/%*") then
            table.insert(comments, 1, ln:gsub("^/%*+%s?", ""):match("^%s*(.-)%s*$"))
            i = i - 1
            break
        else
            break
        end
    end

    -- ↓ filter empty strings ↓
    local out = {}

    for _, c in ipairs(comments) do
        if c ~= "" then
            out[#out + 1] = c
        end
    end

    return table.concat(out, " | ")
end

-- ↓ collect /* ... */ and // comments inside a brace block ↓
local function blockComments(src, startPos, endPos)
    local region = src:sub(startPos, endPos)
    local found  = {}

    -- ↓ block comments ↓
    for cm in region:gmatch("/%*(.-)%*/") do
        local t = cm:match("^%s*(.-)%s*$")

        -- ↓ strip leading * from each line ↓
        t = t:gsub("\n%s*%*%s?", " ")

        if t ~= "" then
            found[#found + 1] = t
        end
    end

    -- ↓ line comments ↓
    for cm in region:gmatch("//([^\n]*)") do
        local t = cm:match("^%s*(.-)%s*$")

        if t ~= "" then
            found[#found + 1] = t
        end
    end

    return found
end

-- ↓ brace matching helper ↓
local function findClosingBrace(src, openPos)
    local depth = 0
    local i     = openPos

    while i <= #src do
        local ch = src:sub(i, i)

        if ch == "{" then
            depth = depth + 1
        elseif ch == "}" then
            depth = depth - 1

            if depth == 0 then
                return i
            end
        end

        i = i + 1
    end

    return nil
end

-- ↓ count newlines from start of src up to pos ↓
local function lineAt(src, pos)
    local sub = src:sub(1, pos)
    local n   = 1

    for _ in sub:gmatch("\n") do
        n = n + 1
    end

    return n
end

-- ↓ core parser ↓
parseFile = function(path, relPath, src, lines)
    local symbols = {
        path     = relPath,
        structs  = {},
        enums    = {},
        funcs    = {},
        externs  = {},
        macros   = {},
        typedefs = {},
    }

    -- ↓ strip string literals so regex doesn't match inside them ↓
    -- (we keep positions intact using a placeholder of the same length)
    local clean = src:gsub('"[^"\n]*"', function(s)
        return string.rep(" ", #s)
    end)

    -- ===================================================================
    -- structs: typedef struct { ... } Name; and struct Name { ... };
    -- ===================================================================

    -- ↓ typedef struct { ... } Name; ↓
    for startPos, name in clean:gmatch("()typedef%s+struct%s*%b{}%s*(%w+)%s*;") do
        local openBrace  = src:find("{", startPos, true)
        local closeBrace = openBrace and findClosingBrace(src, openBrace)
        local lineNo     = lineAt(src, startPos)

        symbols.structs[#symbols.structs + 1] = {
            name    = name,
            kind    = "typedef struct",
            line    = lineNo,
            comment = collectLeadingComments(lines, lineNo),
            inner   = closeBrace and blockComments(src, openBrace, closeBrace) or {},
        }
    end

    -- ↓ struct Name { ... }; ↓
    for startPos, name in clean:gmatch("()struct%s+(%w+)%s*{") do
        local lineNo     = lineAt(src, startPos)
        local openBrace  = src:find("{", startPos, true)
        local closeBrace = openBrace and findClosingBrace(src, openBrace)

        -- ↓ skip if this is inside a typedef (already caught above) ↓
        local pre = clean:sub(math.max(1, startPos - 10), startPos)

        if not pre:match("typedef") then
            symbols.structs[#symbols.structs + 1] = {
                name    = name,
                kind    = "struct",
                line    = lineNo,
                comment = collectLeadingComments(lines, lineNo),
                inner   = closeBrace and blockComments(src, openBrace, closeBrace) or {},
            }
        end
    end

    -- ===================================================================
    -- enums
    -- ===================================================================

    for startPos, name in clean:gmatch("()enum%s+(%w+)%s*{") do
        local lineNo     = lineAt(src, startPos)
        local openBrace  = src:find("{", startPos, true)
        local closeBrace = openBrace and findClosingBrace(src, openBrace)

        -- ↓ collect enum member names ↓
        local members = {}

        if openBrace and closeBrace then
            local body = src:sub(openBrace + 1, closeBrace - 1)

            for m in body:gmatch("(%w+)%s*[=,\n}]") do
                members[#members + 1] = m
            end
        end

        symbols.enums[#symbols.enums + 1] = {
            name    = name,
            line    = lineNo,
            comment = collectLeadingComments(lines, lineNo),
            members = members,
            inner   = closeBrace and blockComments(src, openBrace, closeBrace) or {},
        }
    end

    -- ↓ typedef enum { ... } Name; ↓
    for startPos, name in clean:gmatch("()typedef%s+enum%s*%b{}%s*(%w+)%s*;") do
        local lineNo     = lineAt(src, startPos)
        local openBrace  = src:find("{", startPos, true)
        local closeBrace = openBrace and findClosingBrace(src, openBrace)

        local members = {}

        if openBrace and closeBrace then
            local body = src:sub(openBrace + 1, closeBrace - 1)

            for m in body:gmatch("(%w+)%s*[=,\n}]") do
                members[#members + 1] = m
            end
        end

        symbols.enums[#symbols.enums + 1] = {
            name    = name,
            kind    = "typedef enum",
            line    = lineNo,
            comment = collectLeadingComments(lines, lineNo),
            members = members,
            inner   = closeBrace and blockComments(src, openBrace, closeBrace) or {},
        }
    end

    -- ===================================================================
    -- functions: return_type name(args) { ... }
    -- ===================================================================

    -- ↓ match function definitions (not declarations — must have a body) ↓
    -- pattern: optional static/extern/inline, return type words, name, ( args ), {
    local funcPat = "([%w_%*%s]+%s)([%w_]+)%s*(%b())%s*{"

    for startPos, retRaw, name, args in clean:gmatch("()" .. funcPat) do
        -- ↓ skip keywords that look like functions ↓
        if name ~= "if" and name ~= "while" and name ~= "for" and
           name ~= "switch" and name ~= "else" and name ~= "do" then

            local ret     = retRaw:match("^%s*(.-)%s*$")
            local lineNo  = lineAt(src, startPos)
            local openPos = src:find("{", startPos + #retRaw + #name + #args, true)

            if openPos then
                local closePos = findClosingBrace(src, openPos)

                -- ↓ capture raw body for lua classification in documentation.lua ↓
                local body = closePos and src:sub(openPos + 1, closePos - 1) or ""

                symbols.funcs[#symbols.funcs + 1] = {
                    name    = name,
                    ret     = ret,
                    args    = args:sub(2, -2):match("^%s*(.-)%s*$"),
                    line    = lineNo,
                    comment = collectLeadingComments(lines, lineNo),
                    inner   = closePos and blockComments(src, openPos, closePos) or {},
                    body    = body,
                }
            end
        end
    end

    -- ===================================================================
    -- extern declarations
    -- ===================================================================

    for startPos, decl in clean:gmatch("()extern%s+([^\n;]+);") do
        local lineNo = lineAt(src, startPos)
        local name   = decl:match("(%w+)%s*$") or decl:match("(%w+)%s*%[")

        symbols.externs[#symbols.externs + 1] = {
            name    = name or "?",
            decl    = decl:match("^%s*(.-)%s*$"),
            line    = lineNo,
            comment = collectLeadingComments(lines, lineNo),
        }
    end

    -- ===================================================================
    -- macros: #define
    -- ===================================================================

    for startPos, name, rest in clean:gmatch("()#define%s+(%w+)([^\n]*)") do
        local lineNo = lineAt(src, startPos)

        -- ↓ skip include guards ↓
        if not (rest == "" and (name:upper() == name)) then
            symbols.macros[#symbols.macros + 1] = {
                name    = name,
                value   = rest:match("^%s*(.-)%s*$"),
                line    = lineNo,
                comment = collectLeadingComments(lines, lineNo),
            }
        end
    end

    return symbols
end

-- ↓ cross-reference: mark where each name is used outside its definition file ↓
resolveRefs = function(allFiles)
    local t0 = os.clock()

    local index = {}
    local totalDefs = 0

    io.write("\n[map_symbols] === BUILDING SYMBOL INDEX ===\n")

    for _, entry in ipairs(allFiles) do
        local path = entry.path
        local fileDefs = 0

        io.write(string.format("[file] %s\n", path))

        local function addSymbol(name, kind, line)
            index[name] = index[name] or {}

            table.insert(index[name], {
                file = path,
                kind = kind,
                line = line,
                role = "defined"
            })

            fileDefs = fileDefs + 1
            totalDefs = totalDefs + 1

            io.write(string.format("  [+] %-6s %-20s @ line %-5d\n", kind, name, line))
        end

        for _, s in ipairs(entry.symbols.structs) do
            addSymbol(s.name, "struct", s.line)
        end

        for _, e in ipairs(entry.symbols.enums) do
            addSymbol(e.name, "enum", e.line)
        end

        for _, f in ipairs(entry.symbols.funcs) do
            addSymbol(f.name, "func", f.line)
        end

        io.write(string.format("  -> %d symbols in file\n\n", fileDefs))
    end

    -- count unique names
    local uniqueNames = 0
    for _ in pairs(index) do uniqueNames = uniqueNames + 1 end

    io.write(string.format(
        "[map_symbols] index built: %d unique names, %d total definitions\n\n",
        uniqueNames, totalDefs
    ))

    -- detect duplicates
    io.write("[map_symbols] === DUPLICATE SYMBOL CHECK ===\n")
    for name, refs in pairs(index) do
        if #refs > 1 then
            io.write(string.format("  [!] %s has %d definitions:\n", name, #refs))
            for _, r in ipairs(refs) do
                if r.role == "defined" then
                    io.write(string.format("      - %s (%s:%d)\n", r.kind, r.file, r.line))
                end
            end
        end
    end
    io.write("\n")

    -- usage scan
    io.write("[map_symbols] === SCANNING FOR USAGES ===\n")

    local totalUsages = 0

    for _, entry in ipairs(allFiles) do
        local src  = entry.src
        local path = entry.path
        local fileHits = 0

        io.write(string.format("[scan] %s\n", path))

        for name, refs in pairs(index) do
            local definedHere = false

            for _, r in ipairs(refs) do
                if r.file == path then
                    definedHere = true
                    break
                end
            end

            if not definedHere then
                if src:match("%f[%w_]" .. name .. "%f[^%w_]") then
                    table.insert(index[name], {
                        file = path,
                        kind = refs[1] and refs[1].kind or "?",
                        role = "used"
                    })

                    fileHits = fileHits + 1
                    totalUsages = totalUsages + 1

                    io.write(string.format("  [use] %-20s (%s)\n", name, refs[1] and refs[1].kind or "?"))
                end
            end
        end

        io.write(string.format("  -> %d usages in file\n\n", fileHits))
    end

    local dt = os.clock() - t0

    io.write(string.format(
        "[map_symbols] === DONE ===\n  usages: %d\n  time: %.3fs\n\n",
        totalUsages, dt
    ))

    return index
end

-- ↓ simple table → JSON, handles strings/numbers/booleans/arrays/dicts ↓
writeJSON = function(val, indent)
    indent = indent or 0
    local pad  = string.rep("    ", indent)
    local pad1 = string.rep("    ", indent + 1)

    local t = type(val)

    if t == "nil" then
        return "null"
    elseif t == "boolean" then
        return val and "true" or "false"
    elseif t == "number" then
        return tostring(val)
    elseif t == "string" then
        -- ↓ escape special characters ↓
        val = val:gsub('\\', '\\\\')
        val = val:gsub('"',  '\\"')
        val = val:gsub('\n', '\\n')
        val = val:gsub('\r', '\\r')
        val = val:gsub('\t', '\\t')

        return '"' .. val .. '"'
    elseif t == "table" then
        -- ↓ detect array vs dict ↓
        local isArray = true
        local maxIdx  = 0

        for k, _ in pairs(val) do
            if type(k) ~= "number" or k ~= math.floor(k) or k < 1 then
                isArray = false
                break
            end

            if k > maxIdx then
                maxIdx = k
            end
        end

        isArray = isArray and (maxIdx == #val)

        if isArray then
            if #val == 0 then
                return "[]"
            end

            local parts = {}

            for _, v in ipairs(val) do
                parts[#parts + 1] = pad1 .. writeJSON(v, indent + 1)
            end

            return "[\n" .. table.concat(parts, ",\n") .. "\n" .. pad .. "]"
        else
            local parts = {}
            local keys  = {}

            for k, _ in pairs(val) do
                keys[#keys + 1] = k
            end

            table.sort(keys, function(a, b)
                return tostring(a) < tostring(b)
            end)

            for _, k in ipairs(keys) do
                parts[#parts + 1] = pad1 .. writeJSON(tostring(k)) .. ": " .. writeJSON(val[k], indent + 1)
            end

            return "{\n" .. table.concat(parts, ",\n") .. "\n" .. pad .. "}"
        end
    end

    return '"[unsupported type: ' .. t .. ']"'
end

-- ↓ folder-level grouping ↓
local function folderOf(path)
    -- ↓ strip src/ prefix and get parent dir ↓
    local rel = path:match("src/(.+)")

    if not rel then
        return "root"
    end

    local folder = rel:match("^(.+)/[^/]+$")

    return folder or "root"
end

-- ↓ convert a glob pattern (only * wildcard) to a Lua pattern ↓
local function globToPattern(g)
    return "^" .. g:gsub("([%.%+%-%^%$%(%)%[%]%%])", "%%%1"):gsub("%*", ".*") .. "$"
end

-- ↓ read [exclude] file = … lines from info.txt; returns a set of Lua patterns ↓
local function readExcludes(infoPath)
    local patterns = {}
    local f = io.open(infoPath, "r")

    if not f then
        return patterns
    end

    local inExclude = false

    for line in f:lines() do
        line = line:gsub("\r", "")

        if line:match("^%s*#") then
            -- ↓ comment, skip ↓
        elseif line:match("^%[(.+)%]%s*$") then
            inExclude = line:match("^%[(.+)%]%s*$") == "exclude"
        elseif inExclude then
            local val = line:match("^%s*file%s*=%s*(.-)%s*$")

            if val and val ~= "" then
                patterns[#patterns + 1] = globToPattern(val)
                io.write("[map_symbols] exclude pattern: " .. val .. "\n")
            end
        end
    end

    f:close()

    return patterns
end

-- ↓ returns true if relPath matches any exclude pattern ↓
local function isExcluded(relPath, patterns)
    local norm = relPath:gsub("\\", "/")

    for _, pat in ipairs(patterns) do
        if norm:match(pat) then
            return true
        end
    end

    return false
end

local function main()
    local srcRoot = "src"

    -- ↓ resolve to absolute if run from project root ↓
    local f = io.open(srcRoot .. "/vanir.h", "r")

    if not f then
        srcRoot = "../src"
    else
        f:close()
    end

    io.write("[map_symbols] scanning: " .. srcRoot .. "\n")

    -- ↓ load exclude list from info.txt before scanning ↓
    local infoPath = "tools/info.txt"
    local fi = io.open(infoPath, "r")

    if not fi then
        infoPath = "info.txt"
    else
        fi:close()
    end

    local excludePatterns = readExcludes(infoPath)

    local paths = scanDir(srcRoot)

    if #paths == 0 then
        io.stderr:write("[map_symbols] error: no files found under " .. srcRoot .. "\n")
        return
    end

    io.write("[map_symbols] found " .. #paths .. " source files\n")

    -- ↓ parse every file ↓
    local allFiles = {}

    local totalStructs = 0
    local totalEnums   = 0
    local totalFuncs   = 0
    local totalExterns = 0
    local totalMacros  = 0
    local skipped      = 0

    for _, path in ipairs(paths) do
        local relPath = path:match("src/(.+)") or path

        -- ↓ skip files listed under [exclude] in info.txt ↓
        if isExcluded(relPath, excludePatterns) then
            io.write("  [exclude] " .. relPath .. "\n")
            skipped = skipped + 1
        else
            local src = readFile(path)

            if src then
                -- ↓ build lines table once ↓
                local lines = {}

                for ln in (src .. "\n"):gmatch("([^\n]*)\n") do
                    lines[#lines + 1] = ln
                end

                allFiles[#allFiles + 1] = {
                    path    = relPath,
                    src     = src,
                    lines   = lines,
                    symbols = parseFile(path, relPath, src, lines),
                }

                local s = allFiles[#allFiles].symbols

                totalStructs = totalStructs + #s.structs
                totalEnums   = totalEnums   + #s.enums
                totalFuncs   = totalFuncs   + #s.funcs
                totalExterns = totalExterns + #s.externs
                totalMacros  = totalMacros  + #s.macros

                io.write(
                    "  [parse] " .. relPath ..
                    "  structs="  .. #s.structs ..
                    "  enums="    .. #s.enums ..
                    "  funcs="    .. #s.funcs ..
                    "  externs="  .. #s.externs ..
                    "  macros="   .. #s.macros ..
                    "  lines="    .. #lines ..
                    "\n"
                )
            else
                skipped = skipped + 1
            end
        end
    end

    io.write(
        "[map_symbols] parse complete —" ..
        " files=" .. #allFiles ..
        (skipped > 0 and ("  skipped=" .. skipped) or "") ..
        "  structs=" .. totalStructs ..
        "  enums="   .. totalEnums ..
        "  funcs="   .. totalFuncs ..
        "  externs=" .. totalExterns ..
        "  macros="  .. totalMacros ..
        "\n"
    )

    -- ↓ resolve cross-references ↓
    io.write("[map_symbols] resolving cross-references...\n")

    local xrefs = resolveRefs(allFiles)

    -- ↓ build folder structure ↓
    local folders   = {}
    local folderIdx = {}

    for _, entry in ipairs(allFiles) do
        local folder = folderOf(entry.path)

        if not folderIdx[folder] then
            folderIdx[folder] = { name = folder, files = {} }
            folders[#folders + 1] = folderIdx[folder]
        end

        local fileData = {
            path    = entry.path,
            symbols = entry.symbols,
        }

        -- ↓ strip src from symbols (not needed in output) ↓
        fileData.symbols.src = nil

        folderIdx[folder].files[#folderIdx[folder].files + 1] = fileData
    end

    io.write("[map_symbols] grouped into " .. #folders .. " folder(s): ")

    local folderNames = {}

    for _, fld in ipairs(folders) do
        folderNames[#folderNames + 1] = fld.name .. "(" .. #fld.files .. ")"
    end

    io.write(table.concat(folderNames, ", ") .. "\n")

    -- ↓ attach xrefs to each symbol ↓
    local attachedRefs = 0

    for _, entry in ipairs(allFiles) do
        local function attachRefs(list)
            for _, sym in ipairs(list) do
                local refs = xrefs[sym.name]

                if refs then
                    sym.refs = refs
                    attachedRefs = attachedRefs + 1
                end
            end
        end

        attachRefs(entry.symbols.structs)
        attachRefs(entry.symbols.enums)
        attachRefs(entry.symbols.funcs)
    end

    io.write("[map_symbols] attached cross-refs to " .. attachedRefs .. " symbol(s)\n")

    -- ↓ final output structure ↓
    local output = {
        generated = os.date("%Y-%m-%d %H:%M:%S"),
        folders   = folders,
    }

    -- ↓ write JSON ↓
    local outPath = "tools/symbols.json"

    -- ↓ try writing relative to cwd; if tools/ doesn't exist, try project root ↓
    local outFile = io.open(outPath, "w")

    if not outFile then
        outPath  = "symbols.json"
        outFile  = io.open(outPath, "w")
    end

    if not outFile then
        io.stderr:write("[map_symbols] error: could not open output file for writing\n")
        return
    end

    io.write("[map_symbols] serialising JSON...\n")

    local jsonStr = writeJSON(output)

    outFile:write(jsonStr)
    outFile:close()

    io.write("[map_symbols] wrote symbol map to: " .. outPath .. " (" .. #jsonStr .. " bytes)\n")
    io.write("[map_symbols] done.\n")
end

main()