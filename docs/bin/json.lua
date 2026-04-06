-- minimal JSON parser (no external deps).

local json = {}

do
    local function skipWS(s, i)
        while i <= #s and s:sub(i, i):match("%s") do
            i = i + 1
        end

        return i
    end

    local parseVal

    local function parseStr(s, i)
        i = i + 1

        local buf = {}

        while i <= #s do
            local ch = s:sub(i, i)

            if ch == '"' then
                return table.concat(buf), i + 1
            elseif ch == "\\" then
                local esc = s:sub(i + 1, i + 1)

                if     esc == '"'  then buf[#buf + 1] = '"'
                elseif esc == "\\" then buf[#buf + 1] = "\\"
                elseif esc == "/"  then buf[#buf + 1] = "/"
                elseif esc == "n"  then buf[#buf + 1] = "\n"
                elseif esc == "r"  then buf[#buf + 1] = "\r"
                elseif esc == "t"  then buf[#buf + 1] = "\t"
                else                    buf[#buf + 1] = esc
                end

                i = i + 2
            else
                buf[#buf + 1] = ch
                i = i + 1
            end
        end

        error("unterminated string at " .. i)
    end

    local function parseArr(s, i)
        i = i + 1

        local arr = {}

        i = skipWS(s, i)

        if s:sub(i, i) == "]" then
            return arr, i + 1
        end

        while true do
            local val

            val, i = parseVal(s, i)
            arr[#arr + 1] = val
            i = skipWS(s, i)

            local ch = s:sub(i, i)

            if ch == "]" then
                return arr, i + 1
            elseif ch == "," then
                i = skipWS(s, i + 1)
            else
                error("expected ] or , at " .. i)
            end
        end
    end

    local function parseObj(s, i)
        i = i + 1

        local obj = {}

        i = skipWS(s, i)

        if s:sub(i, i) == "}" then
            return obj, i + 1
        end

        while true do
            local key

            key, i = parseStr(s, i)
            i = skipWS(s, i)

            if s:sub(i, i) ~= ":" then
                error("expected : at " .. i)
            end

            i = skipWS(s, i + 1)

            local val

            val, i = parseVal(s, i)
            obj[key] = val
            i = skipWS(s, i)

            local ch = s:sub(i, i)

            if ch == "}" then
                return obj, i + 1
            elseif ch == "," then
                i = skipWS(s, i + 1)
            else
                error("expected } or , at " .. i)
            end
        end
    end

    parseVal = function(s, i)
        i = skipWS(s, i)

        local ch = s:sub(i, i)

        if ch == '"' then
            return parseStr(s, i)
        elseif ch == "[" then
            return parseArr(s, i)
        elseif ch == "{" then
            return parseObj(s, i)
        elseif s:sub(i, i + 3) == "true" then
            return true, i + 4
        elseif s:sub(i, i + 4) == "false" then
            return false, i + 5
        elseif s:sub(i, i + 3) == "null" then
            return nil, i + 4
        else
            local num, j = s:match("^(-?%d+%.?%d*[eE]?[+-]?%d*)()", i)

            if num then
                return tonumber(num), j
            end
        end

        error("unexpected character '" .. ch .. "' at position " .. i)
    end

    function json.decode(s)
        local val, _ = parseVal(s, 1)

        return val
    end
end

return json