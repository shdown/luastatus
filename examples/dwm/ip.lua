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
                table.insert(r, string.format('[%s: %s]', iface, addr))
            end
        end
        return r
    end,
}
