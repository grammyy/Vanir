-- builds the sidebar file-tree HTML from the symbol data.

local M = {}

local function normPath(p)
    return tostring(p or ""):gsub("\\", "/")
end

local function fileAnchor(path)
    return normPath(path):gsub("[^%w]", "_")
end

local function esc(s)
    if not s then return "" end
    s = tostring(s)
    s = s:gsub("&",  "&amp;")
    s = s:gsub("<",  "&lt;")
    s = s:gsub(">",  "&gt;")
    s = s:gsub('"',  "&quot;")
    return s
end

-- ↓ collect every file entry from folder data into a flat list ↓
function M.collectAllFiles(data)
    local out = {}

    for _, folder in ipairs(data.folders or {}) do
        for _, file in ipairs(folder.files or {}) do
            out[#out + 1] = file
        end
    end

    return out
end

-- ↓ build a nested tree from a flat list of file paths ↓
function M.buildTree(files)
    local root = { children = {}, path = "" }

    for _, file in ipairs(files or {}) do
        local path = normPath(file.path)
        local cur  = root
        local accum = ""

        for part in path:gmatch("[^/]+") do
            accum = (accum == "") and part or (accum .. "/" .. part)

            cur.children = cur.children or {}

            if not cur.children[part] then
                cur.children[part] = {
                    name     = part,
                    path     = accum,
                    children = {}
                }
            end

            cur = cur.children[part]
        end

        cur.file = file
    end

    return root
end

local function sortedKeys(t)
    local keys = {}

    for k in pairs(t or {}) do
        keys[#keys + 1] = k
    end

    table.sort(keys, function(a, b)
        return tostring(a):lower() < tostring(b):lower()
    end)

    return keys
end

-- ↓ recursively render the tree as sidebar HTML ↓
function M.renderTree(node, depth)
    depth = depth or 0

    local parts       = {}
    local folderNames = {}
    local fileNames   = {}

    for _, name in ipairs(sortedKeys(node.children)) do
        local child = node.children[name]

        if child and child.file then
            fileNames[#fileNames + 1] = name
        else
            folderNames[#folderNames + 1] = name
        end
    end

    for _, name in ipairs(folderNames) do
        local child = node.children[name]

        parts[#parts + 1] =
            '<div class="sb-folder" data-folder-path="' .. esc(child.path) .. '">' ..
                '<div class="sb-folder-hd" onclick="toggleFolder(this.parentElement)">' ..
                    '<span class="sb-arrow">▶</span>' ..
                    esc(name) ..
                '</div>' ..
                '<div class="sb-files">' ..
                    M.renderTree(child, depth + 1) ..
                '</div>' ..
            '</div>'
    end

    for _, name in ipairs(fileNames) do
        local child = node.children[name]
        local file  = child.file
        local a     = fileAnchor(file.path)

        parts[#parts + 1] =
            '<a class="sb-file" href="#' .. a .. '" data-file="' .. a .. '" data-path="' .. esc(normPath(file.path)) .. '">' ..
                esc(name) ..
            '</a>'
    end

    return table.concat(parts, "\n")
end

return M