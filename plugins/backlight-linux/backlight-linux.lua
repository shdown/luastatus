local P = {}

local function read_all(path)
    local f = assert(io.open(path, 'r'))
    local r = f:read('*all')
    f:close()
    return r
end

function P.widget(tbl)
    return {
        plugin = 'udev',
        opts = {
            subsystem = 'backlight',
            timeout = tbl.timeout or 2,
        },
        cb = function(t)
            if t.what ~= 'event' then
                return nil
            end
            local b = read_all(t.syspath .. '/brightness')
            local mb = read_all(t.syspath .. '/max_brightness')
            return tbl.cb(b / mb)
        end,
        event = tbl.event,
    }
end

return P
