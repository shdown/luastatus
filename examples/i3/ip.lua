widget = {
    plugin = 'ipaddr',
    cb = function(t)
        local r = {}
        for iface, addr in pairs(t.ipv6) do
            -- strip out "label" from the interface name
            iface = iface:gsub(':.*', '')
            -- strip out "zone index" from the address
            addr = addr:gsub('%%.*', '')

            if iface ~= 'lo' then
                table.insert(r, {
                    full_text = string.format('[%s: %s]', iface, addr),
                    color = '#709080'
                })
            end
        end
        return r
    end,
}
