local P = {}

function P.shell_escape(x)
    if type(x) == 'string' then
        if x:find('\0') then
            error('shell argument contains NUL character')
        end
        return "'" .. x:gsub("'", "'\\''") .. "'"
    elseif type(x) == 'table' then
        local t = {}
        for _, arg in ipairs(x) do
            t[#t + 1] = shell_escape(arg)
        end
        return table.concat(t, ' ')
    end
end

local function make_eof_handler()
    local hang = false
    return function()
        if hang then
            while true do
                os.execute('sleep 3600')
            end
        else
            hang = true
            error('child process has closed its stdout')
        end
    end
end

P.widget = function(tbl)
    local f = assert(io.popen(tbl.command, 'r'))
    local on_eof = tbl.on_eof or make_eof_handler()
    return {
        plugin = 'timer',
        opts = {period = 0},
        cb = function()
            local r = f:read('*line')
            if r ~= nil then
                return tbl.cb(r)
            end
            on_eof()
        end,
        event = tbl.event,
    }
end

return P
