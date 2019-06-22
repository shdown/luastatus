local P = {}

local function read_num(path)
    local f = assert(io.open(path, 'r'))
    local r = f:read('*number')
    f:close()
    return r
end

function P.widget(tbl)
    local timeout = tbl.timeout or 2
    return {
        plugin = 'udev',
        opts = {
            subsystem = 'backlight',
        },
        cb = function(t)
            if t.what ~= 'event' then
                return tbl.cb(nil)
            end
            local s = t.syspath
            if tbl.syspath and tbl.syspath ~= s then
                return tbl.cb(nil)
            end

            luastatus.plugin.push_timeout(timeout)

            local b = read_num(s .. '/brightness')
            local mb = read_num(s .. '/max_brightness')
            return tbl.cb(b / mb)
        end,
        event = tbl.event,
    }
end

return P
