local P = {}

function P.read_uevent(dev)
    local f = io.open('/sys/class/power_supply/' .. dev .. '/uevent', 'r')
    if not f then
        return nil
    end
    local r = {}
    for line in f:lines() do
        local key, value = line:match('POWER_SUPPLY_(.-)=(.*)')
        r[key:lower()] = value
    end
    f:close()
    return r
end

function P.read_files(dev, proplist)
    local prefix = '/sys/class/power_supply/' .. dev .. '/'
    local r = {}
    for _, prop in ipairs(proplist) do
        local f = io.open(prefix .. prop, 'r')
        if f then
            r[prop] = f:read('*line')
            f:close()
        end
    end
    return r
end

local PROPS_NAMING_SCHEMAS = {
    new = {
        EF = 'energy_full',
        EN = 'energy_now',
        PN = 'power_now',
    },
    old = {
        EF = 'charge_full',
        EN = 'charge_now',
        PN = 'current_now',
    },
}
local pns = PROPS_NAMING_SCHEMAS.new
function P.use_props_naming_scheme(name)
    pns = assert(PROPS_NAMING_SCHEMAS[name])
end
function P.get_props_naming_scheme()
    return pns
end

function P.est_rem_time(props)
    local ef, en, pn = props[pns.EF], props[pns.EN], props[pns.PN]
    if not (ef and en and pn) then
        return nil
    end
    if props.status == 'Charging' then
        return (ef - en) / pn
    elseif props.status == 'Discharging' then
        return en / pn
    else
        return nil
    end
end

function P.widget(tbl)
    if tbl.props_naming_scheme then
        P.use_props_naming_scheme(tbl.props_naming_scheme)
    end
    local dev = tbl.dev or 'BAT0'
    local read_props
    if tbl.no_uevent then
        local proplist = {'status', 'capacity'}
        if tbl.est_rem_time then
            table.insert(proplist, pns.EF)
            table.insert(proplist, pns.EN)
            table.insert(proplist, pns.PN)
        end
        read_props = function()
            return P.read_files(dev, proplist)
        end
    else
        read_props = function()
            return P.read_uevent(dev)
        end
    end
    return {
        plugin = 'timer',
        opts = tbl.timer_opts,
        cb = function()
            local t = read_props()
            if tbl.est_rem_time then
                t.rem_time = P.est_rem_time(t)
            end
            return tbl.cb(t)
        end,
        event = tbl.event,
    }
end

return P
