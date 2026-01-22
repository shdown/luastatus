-- === Description ===

-- This is a widget to monitor the state of a systemd unit.
-- If currently does not react to D-Bus signals, instead querying
-- a D-Bus property of the unit periodically (see PERIOD variable).

-- === Settings ===

local UNIT = 'tor.service'
local HUMAN_NAME = 'Tor'
local PERIOD = 3

-- === Implementation ===

local human_name_escaped = nil

local unit_file = nil

local function requery_unit_file()
    -- This should happen on startup and then extremely rarely, so we
    -- can afford spawning a process.
    local argv = {
        'dbus-send',
        '--system',
        '--dest=org.freedesktop.systemd1',
        '--print-reply=literal',
        '/org/freedesktop/systemd1',
        'org.freedesktop.systemd1.Manager.GetUnit',
        'string:' .. UNIT,
    }
    local shell_cmd = table.concat(argv, ' ')
    local f = assert(io.popen(shell_cmd))
    local answer = f:read('*line')
    f:close()

    assert(answer and answer ~= '', 'dbus-send command failed')
    unit_file = assert(string.match(answer, '%S+'))
end

local COLORS = {
    pink   = '#ec93d3',
    green  = '#60b48a',
    yellow = '#dfaf8f',
    red    = '#dca3a3',
    dim    = '#909090',
}

local function make_output(text, color_name)
    human_name_escaped = human_name_escaped or luastatus.barlib.escape(HUMAN_NAME)
    local color = assert(COLORS[color_name])
    return string.format(
        '[%s: %s%s%s]',
        human_name_escaped,
        '%{F' .. color .. '}', -- begin color
        luastatus.barlib.escape(text),
        '%{F-}' -- end color
    )
end

widget = {
    plugin = 'dbus',
    opts = {
        signals = {},
        timeout = PERIOD,
        greet = true,
    },
    cb = function(_)
        if not unit_file then
            requery_unit_file()
        end
        local is_ok, res = luastatus.plugin.get_property({
            bus = 'system',
            dest = 'org.freedesktop.systemd1',
            object_path = assert(unit_file),
            interface = 'org.freedesktop.systemd1.Unit',
            property_name = 'ActiveState',
        })
        if not is_ok then
            print('WARNING: get_property failed:', res)
            unit_file = nil
            return make_output('no unit file', 'pink')
        end

        assert(type(res) == 'table' and #res == 1)
        local state = res[1]

        if state == 'active' then
            return make_output('✓', 'green')
        elseif state == 'reloading' or state == 'activating' then
            return make_output('•', 'yellow')
        elseif state == 'inactive' or state == 'deactivating' then
            return make_output('-', 'dim')
        elseif state == 'failed' then
            return make_output('x', 'red')
        else
            print('WARNING: unknown state:', state)
            return make_output('?', 'dim')
        end
    end,
}
