pt_testcase_begin

pt_add_fifo "$main_fifo_file"
pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
widget = {
    plugin = '$PT_BUILD_DIR/plugins/timer/plugin-timer.so',
    opts = {
        period = 0.1,
        make_self_pipe = true,
    },
    cb = function(t)
        f:write('cb ' .. t .. '\n')
        if t == 'timeout' then
            luastatus.plugin.wake_up()
        end
    end,
}
__EOF__

pt_spawn_luastatus
exec {pfd}<"$main_fifo_file"

pt_expect_line 'init' <&$pfd
pt_expect_line 'cb hello' <&$pfd
pt_expect_line 'cb timeout' <&$pfd
pt_expect_line 'cb self_pipe' <&$pfd
pt_expect_line 'cb timeout' <&$pfd
pt_expect_line 'cb self_pipe' <&$pfd
pt_close_fd "$pfd"

pt_testcase_end
