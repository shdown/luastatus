-- Note that this widget only shows backlight level when it changes.
widget = luastatus.require_plugin('backlight-linux').widget{
    cb = function(level)
        if level ~= nil then
            local body = string.format('*%3.0f%%', level * 100)
            return luastatus.barlib.escape(body)
        end
    end,
}
