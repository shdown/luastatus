pt_testcase_begin
using_measure

pt_add_fifo "$main_fifo_file"
pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
widget = {
    plugin = '$PT_BUILD_DIR/plugins/multiplex/plugin-multiplex.so',
    opts = {
        data_sources = {
            src1 = [[
                i = 0
                widget = {
                    plugin = '$PT_BUILD_DIR/plugins/timer/plugin-timer.so',
                    cb = function(t)
                        i = i + 1
                        return string.format("%s %d", t, i)
                    end,
                }
            ]],
        },
    },
    cb = function(t)
        assert(t.what == 'update')
        local text = t.updates.src1
        f:write('cb ' .. text .. '\n')
    end,
}
__EOF__
pt_spawn_luastatus
exec {pfd}<"$main_fifo_file"
pt_expect_line 'init' <&$pfd
measure_start
pt_expect_line 'cb hello 1' <&$pfd
measure_check_ms 0
pt_expect_line 'cb timeout 2' <&$pfd
measure_check_ms 1000
pt_expect_line 'cb timeout 3' <&$pfd
measure_check_ms 1000
pt_close_fd "$pfd"
pt_testcase_end
