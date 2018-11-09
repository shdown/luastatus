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

function P.widget(tbl)
    local f = assert(io.popen(tbl.command, 'r'))
    return {
        plugin = 'timer',
        opts = {period = 0},
        cb = function()
            local r = f:read('*line')
            if r ~= nil then
                return tbl.cb(r)
            end
            if tbl.on_eof ~= nil then
                tbl.on_eof()
            else
                luastatus.plugin.push_period(3600)
                error('child process has closed its stdout')
            end
        end,
        event = tbl.event,
    }
end

return P
