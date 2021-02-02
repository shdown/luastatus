main_fifo_file=./tmp-fifo-main

preface='
local function _fmt_x(x)
    local function _validate_str(s)
        assert(s:find("\n") == nil, "string contains a line break")
        assert(s:find("\"") == nil, "string contains a double quote sign")
    end
    local t = type(x)
    if t == "table" then
        local tk = type(next(x))
        if tk == "nil" then
            return "{}"
        elseif tk == "number" then
            local s = {}
            for _, v in ipairs(x) do
                s[#s + 1] = _fmt_x(v)
            end
            return string.format("{%s}", table.concat(s, ","))
        elseif tk == "string" then
            local ks = {}
            for k, _ in pairs(x) do
                assert(type(k) == "string", "table has mixed-type keys")
                _validate_str(k)
                ks[#ks + 1] = k
            end
            table.sort(ks)
            local s = {}
            for _, k in ipairs(ks) do
                local v = x[k]
                s[#s + 1] = string.format("[\"%s\"]=%s", k, _fmt_x(v))
            end
            return string.format("{%s}", table.concat(s, ","))
        else
            error("table key has unexpected type: " .. tk)
        end
    elseif t == "string" then
        _validate_str(x)
        return string.format("\"%s\"", x)
    else
        return tostring(x)
    end
end
'
