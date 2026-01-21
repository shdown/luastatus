widget = {
    plugin = 'timer',
    opts = {
        make_self_pipe = true,
        period = 2.0,
    },
    cb = function(t)
        if t == 'self_pipe' then
            return {full_text = 'Thanks!'}
        else
            return {full_text = 'Click me'}
        end
    end,
    event = function(_)
        luastatus.plugin.wake_up()
    end,
}
