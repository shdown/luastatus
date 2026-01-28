-- === Description ===

-- This is a widget to monitor the state of a systemd unit.

-- === Settings ===

local UNIT = 'tor.service'
local HUMAN_NAME = 'Tor'

-- === Implementation ===

local function unwrap1_into_str(x)
    assert(type(x) == 'table')
    assert(#x == 1)
    local res = x[1]
    assert(type(res) == 'string')
    return res
end

local function subscribe()
    local is_ok, res = luastatus.plugin.call_method_str({
        bus = 'system',
        dest = 'org.freedesktop.systemd1',
        object_path = '/org/freedesktop/systemd1',
        interface = 'org.freedesktop.systemd1.Manager',
        method = 'Subscribe',
        -- no "arg_str" parameter
    })
    assert(is_ok, res)
end

local function get_unit_path()
    local is_ok, res = luastatus.plugin.call_method_str({
        bus = 'system',
        dest = 'org.freedesktop.systemd1',
        object_path = '/org/freedesktop/systemd1',
        interface = 'org.freedesktop.systemd1.Manager',
        method = 'GetUnit',
        arg_str = UNIT,
    })
    assert(is_ok, res)
    return unwrap1_into_str(res)
end

local function get_state(unit_path)
    local is_ok, res = luastatus.plugin.get_property({
        bus = 'system',
        dest = 'org.freedesktop.systemd1',
        object_path = unit_path,
        interface = 'org.freedesktop.systemd1.Unit',
        property_name = 'ActiveState',
    })
    assert(is_ok, res)
    return unwrap1_into_str(res)
end

local function make_output(text)
    return string.format('[%s: %s]', HUMAN_NAME, text)
end

local is_subscribed = false

widget = {
    plugin = 'dbus',
    opts = {
        signals = {
            {
                bus = 'system',
                interface = 'org.freedesktop.DBus.Properties',
                signal = 'PropertiesChanged',
                arg0 = 'org.freedesktop.systemd1.Unit',
            },
            {
                bus = 'system',
                interface = 'org.freedesktop.systemd1.Manager',
                signal = 'UnitFilesChanged',
                arg0 = 'org.freedesktop.systemd1.Unit',
            },
        },
        timeout = 30,
        greet = true,
    },
    cb = function(_)
        if not is_subscribed then
            subscribe()
            is_subscribed = true
        end

        local unit_path = get_unit_path()
        local state = get_state(unit_path)

        if state == 'active' then
            return make_output('✓')
        elseif state == 'reloading' or state == 'activating' then
            return make_output('•')
        elseif state == 'inactive' or state == 'deactivating' then
            return make_output('-')
        elseif state == 'failed' then
            return make_output('x')
        else
            print('WARNING: unknown state:', state)
            return make_output('?')
        end
    end,
}
