widget = luastatus.require_plugin('mem-usage-linux').widget{
    timer_opts = {period = 2},
    cb = function(t)
        local used_kb = t.total.value - t.avail.value
        return {full_text = string.format('[%3.2f %s]', used_kb / 1024 / 1024), color = '#af8ec3'}
    end,
}
