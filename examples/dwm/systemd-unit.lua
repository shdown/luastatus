local function make_output(text)
    return string.format('[Tor: %s]', text)
end

widget = luastatus.require_plugin('systemd-unit').widget{
    unit_name = 'tor.service',
    cb = function(state)
        if state == 'active' then
            return make_output('✓')
        elseif state == 'reloading' or state == 'activating' then
            return make_output('•')
        elseif state == 'inactive' or state == 'deactivating' then
            return make_output('-')
        elseif state == 'failed' then
            return make_output('x')
        else
            return make_output('?')
        end
    end,
}
