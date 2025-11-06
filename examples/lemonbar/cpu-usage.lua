widget = luastatus.require_plugin('cpu-usage-linux').widget{
    cb = function(usage)
        if usage ~= nil then
            local body = string.format('[%5.1f%%]', usage * 100)
            return luastatus.barlib.escape(body)
        end
    end,
}
