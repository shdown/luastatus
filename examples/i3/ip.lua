widget = {
    plugin = 'network-linux',
    cb = function(t)
        local r = {}
        for iface, params in pairs(t) do
            local iface_clean = iface
            local addr = params.ipv6 or params.ipv4
            if addr then
                -- strip out "label" from the interface name
                iface_clean = iface_clean:gsub(':.*', '')
                -- strip out "zone index" from the address
                addr = addr:gsub('%%.*', '')

                if iface_clean ~= 'lo' then
                    r[#r + 1] = {
                        full_text = string.format('[%s: %s]', iface_clean, addr),
                        color = '#709080',
                    }
                end
            end
        end
        return r
    end,
}
