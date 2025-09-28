local plugin_data = {}

local plugin_params = {
    timer_opts = {
        period = 2,
    },
    cb = function(t)
        if not t then
            return nil
        end
        local r = {}
        for _, entry in ipairs(t) do
            r[#r + 1] = string.format('%.0f°', entry.value)
        end
        return r
    end,
    event = function(t)
        if t.button == 1 then
            plugin_data.please_reload = true
        end
    end,
}

widget = luastatus.require_plugin('cpu-temp-linux').widget(plugin_params, plugin_data)
