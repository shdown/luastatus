local COOL_TEMP = 50
local HEAT_TEMP = 75
local function getcolor(temp)
    local t = (temp - COOL_TEMP) / (HEAT_TEMP - COOL_TEMP)
    if t < 0 then t = 0 end
    if t > 1 then t = 1 end
    local red = math.floor(t * 255 + 0.5)
    return string.format('#%02x%02x00', red, 255 - red)
end

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
            local temp = entry.value
            r[#r + 1] = {full_text = string.format('%.0f°', temp), color = getcolor(temp)}
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
