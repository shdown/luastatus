widget = luastatus.require_plugin('mem-usage-linux').widget{
    timer_opts = {period = 2},
    cb = function(t)
        local used_kb = t.total.value - t.avail.value
        local body = string.format('[%3.2f GiB]', used_kb / 1024 / 1024)
        return "%{F#af8ec3}" .. body .. "%{F-}"
    end,
}
