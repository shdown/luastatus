local P = {}

local function read_all(path)
    local f = assert(io.open(path, 'r'))
    local r = f:read('*all')
    f:close()
    return r
end

function P.widget(tbl)
    local last_content
    return {
        plugin = 'udev',
        opts = {
            subsystem = 'backlight',
            timeout = tbl.timeout or 2,
        },
        cb = function(t)
            if t.what ~= 'event' then
                last_content = nil
                return last_content
            end
            local s = t.syspath
            if tbl.syspath and tbl.syspath ~= s then
                return last_content
            end
            local b = read_all(s .. '/brightness')
            local mb = read_all(s .. '/max_brightness')
            last_content = tbl.cb(b / mb)
            return last_content
        end,
        event = tbl.event,
    }
end

return P
