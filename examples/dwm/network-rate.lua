local function make_segment(iface, R, S)
    return string.format('[%s %.0fk↓ %.0fk↑]', iface, R / 1000, S / 1000)
end

widget = luastatus.require_plugin('network-rate-linux').widget{
    iface_except = 'lo',
    period = 3,
    in_array_form = true,
    cb = function(t)
        local r = {}
        for _, PQ in ipairs(t) do
            local iface = PQ[1]
            local R, S = PQ[2].R, PQ[2].S
            r[#r + 1] = make_segment(iface, R, S)
        end
        return r
    end,
}
