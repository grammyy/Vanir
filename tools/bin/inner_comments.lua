-- extracts and renders annotated comment blocks from function bodies.

local M = {}

local function splitLines(s)
    local out = {}

    if not s or s == "" then
        return out
    end

    s = tostring(s):gsub("\r", "")

    for line in (s .. "\n"):gmatch("(.-)\n") do
        out[#out + 1] = line
    end

    return out
end

local function splitCommentLine(line)
    line = tostring(line or "")

    do
        local code, comment = line:match("^(.-)%s*/%*%s*(.-)%s*%*/%s*$")
        if comment then return code, comment end
    end

    do
        local code, comment = line:match("^(.-)%s*//%s*(.-)%s*$")
        if comment then return code, comment end
    end

    do
        local comment = line:match("^%s*/%*%s*(.-)%s*%*/%s*$")
        if comment then return "", comment end
    end

    do
        local comment = line:match("^%s*//%s*(.-)%s*$")
        if comment then return "", comment end
    end

    return nil, nil
end

local function isBlank(line)
    return tostring(line or ""):match("^%s*$") ~= nil
end

-- ↓ pull annotated comment blocks out of a function body ↓
-- each block is { header = string, code = {lines} }
function M.extractInnerCommentBlocks(body)
    local lines  = splitLines(body)
    local blocks = {}

    for i = 1, #lines do
        local prefixCode, header = splitCommentLine(lines[i])

        -- ↓ skip closing-arrow comments (↑ … ↑) — only the opening ↓ is used ↓
        if header and header:match("^%s*↑") then
            header = nil
        end

        if header then
            local start = i - 1
            while start >= 1 and not isBlank(lines[start]) and not (select(2, splitCommentLine(lines[start]))) do
                start = start - 1
            end
            start = start + 1

            local finish = i + 1
            while finish <= #lines and not isBlank(lines[finish]) and not (select(2, splitCommentLine(lines[finish]))) do
                finish = finish + 1
            end
            finish = finish - 1

            local code = {}

            for j = start, i - 1 do
                if not isBlank(lines[j]) and not (select(2, splitCommentLine(lines[j]))) then
                    code[#code + 1] = lines[j]
                end
            end

            if prefixCode and prefixCode:match("%S") then
                code[#code + 1] = prefixCode
            end

            for j = i + 1, finish do
                if not isBlank(lines[j]) and not (select(2, splitCommentLine(lines[j]))) then
                    code[#code + 1] = lines[j]
                end
            end

            blocks[#blocks + 1] = { header = header, code = code }
        end
    end

    return blocks
end

-- ↓ render inner comment blocks (or fall back to raw inner strings) as HTML ↓
function M.innerHTML(esc, body, inner)
    if body and body ~= "" then
        local blocks = M.extractInnerCommentBlocks(body)

        if #blocks > 0 then
            local parts = {}

            for _, b in ipairs(blocks) do
                local codeHTML = ""

                if b.code and #b.code > 0 then
                    codeHTML =
                        '<pre class="inner-snippet-code">' ..
                        esc(table.concat(b.code, "\n")) ..
                        '</pre>'
                end

                parts[#parts + 1] =
                    '<div class="inner-snippet">' ..
                        '<div class="inner-snippet-header">' .. esc(b.header) .. '</div>' ..
                        codeHTML ..
                    '</div>'
            end

            return '<div class="inner-comments">' .. table.concat(parts, "") .. '</div>'
        end
    end

    if not inner or #inner == 0 then
        return ""
    end

    local parts = {}

    for _, c in ipairs(inner) do
        if c and c ~= "" then
            parts[#parts + 1] = '<span class="inner-comment">// ' .. esc(c) .. '</span>'
        end
    end

    if #parts == 0 then
        return ""
    end

    return '<div class="inner-comments">' .. table.concat(parts, "") .. '</div>'
end

return M