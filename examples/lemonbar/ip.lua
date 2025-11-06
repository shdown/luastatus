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
                    local body = string.format('[%s: %s]', iface_clean, addr)
                    r[#r + 1] = "%{F#709080}" .. body .. "%{F-}"
                end
            end
        end
        return r
    end,
}
