-- === Description ===

-- This is a widget to monitor the state of a systemd unit.
-- If currently does not react to D-Bus signals, instead querying
-- a D-Bus property of the unit periodically (see PERIOD variable).

-- === Settings ===

local UNIT = 'tor.service'
local HUMAN_NAME = 'Tor'
local PERIOD = 3

-- === Implementation ===

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

local function make_output(text)
    return string.format('[%s: %s]', HUMAN_NAME, text)
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
            iface = 'org.freedesktop.systemd1.Unit',
            prop_name = 'ActiveState',
        })
        if not is_ok then
            print('WARNING: get_property failed:', res)
            unit_file = nil
            return make_output('no unit file')
        end

        assert(type(res) == 'table' and #res == 1)
        local state = res[1]

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
