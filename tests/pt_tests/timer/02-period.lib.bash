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
    },
    cb = function(t)
        f:write('cb ' .. t .. '\n')
    end,
}
__EOF__
pt_spawn_luastatus
exec 3<"$main_fifo_file"
measure_start
pt_expect_line 'init' <&3
measure_check_ms 0
pt_expect_line 'cb hello' <&3
measure_check_ms 0
pt_expect_line 'cb timeout' <&3
measure_check_ms 100
pt_expect_line 'cb timeout' <&3
measure_check_ms 100
exec 3<&-
pt_testcase_end
