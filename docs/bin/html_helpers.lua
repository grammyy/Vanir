-- HTML escaping, anchor generation, and search-text helpers.

local M = {}

-- ↓ escape characters that are special in HTML ↓
function M.esc(s)
    if not s then return "" end

    s = tostring(s)
    s = s:gsub("&",  "&amp;")
    s = s:gsub("<",  "&lt;")
    s = s:gsub(">",  "&gt;")
    s = s:gsub('"',  "&quot;")

    return s
end

-- ↓ normalise path separators to forward slash ↓
function M.normPath(p)
    return tostring(p or ""):gsub("\\", "/")
end

-- ↓ turn a file path into a safe HTML id ↓
function M.fileAnchor(path)
    return M.normPath(path):gsub("[^%w]", "_")
end

-- ↓ unique anchor for a symbol within a file ↓
function M.symbolAnchor(path, kind, name, line)
    return M.fileAnchor(path) .. "__" .. tostring(kind or "sym") .. "__" ..
           tostring(name or "sym"):gsub("[^%w]", "_") .. "__" .. tostring(line or "0")
end

-- ↓ build the data-search text for a symbol card ↓
function M.buildSearchText(...)
    local args  = { ... }
    local parts = {}

    -- ↓ include raw strings first so camelCase names are searchable unsplit ↓
    for i = 1, #args do
        local raw = tostring(args[i] or ""):lower()
        if raw ~= "" then
            parts[#parts + 1] = raw
        end
    end

    -- ↓ also include split/normalised tokens for word-level matching ↓
    for i = 1, #args do
        local norm = tostring(args[i] or "")
            :gsub("([%l])([%u])", "%1 %2")
            :gsub("[_%./%-]+", " ")
            :gsub("%s+", " ")
            :lower()
        if norm ~= "" then
            parts[#parts + 1] = norm
        end
    end

    return table.concat(parts, " ")
end

-- ↓ render cross-reference badges ↓
function M.refsHTML(refs, selfPath)
    if not refs or #refs == 0 then
        return ""
    end

    local parts = {}

    for _, r in ipairs(refs) do
        if r.file ~= selfPath then
            local cls = r.role == "defined" and "badge-defined" or "badge-used"

            parts[#parts + 1] =
                '<span class="badge ' .. cls .. '" title="' .. M.esc(r.role) .. '">' ..
                M.esc(r.file) .. ':' .. (r.line or "?") ..
                '</span>'
        end
    end

    return table.concat(parts, " ")
end

return M