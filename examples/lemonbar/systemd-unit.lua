local COLORS = {
    pink   = '#ec93d3',
    green  = '#60b48a',
    yellow = '#dfaf8f',
    red    = '#dca3a3',
    dim    = '#909090',
}

local function make_output(text, color_name)
    local color_begin = '%{F' .. assert(COLORS[color_name]) .. '}'
    local color_end = '%{F-}'
    return string.format('[Tor: %s%s%s]', color_begin, text, color_end)
end

widget = luastatus.require_plugin('systemd-unit').widget{
    unit_name = 'tor.service',
    cb = function(state)
        if state == 'active' then
            return make_output('✓', 'green')
        elseif state == 'reloading' or state == 'activating' then
            return make_output('•', 'yellow')
        elseif state == 'inactive' or state == 'deactivating' then
            return make_output('-', 'dim')
        elseif state == 'failed' then
            return make_output('x', 'red')
        else
            return make_output('?', 'pink')
        end
    end,
}
