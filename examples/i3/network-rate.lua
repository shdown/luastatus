local function make_segment(iface, R, S)
    return {
        full_text = string.format('[%s <span color="#DCA3A3">%.0fk↓</span> <span color="#72D5A3">%.0fk↑</span>]',
            iface,
            R / 1000,
            S / 1000
        ),
        markup = 'pango',
    }
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
