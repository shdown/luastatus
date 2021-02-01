pt_testcase_begin
pt_add_fifo "$main_fifo_file"
pt_add_fifo "$wakeup_fifo_file"
pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
widget = {
    plugin = '$PT_BUILD_DIR/plugins/timer/plugin-timer.so',
    opts = {
        fifo = '$wakeup_fifo_file',
    },
    cb = function(t)
        f:write('cb ' .. t .. '\n')
    end,
}
__EOF__
pt_spawn_luastatus
exec {pfd}<"$main_fifo_file"
measure_start
pt_expect_line 'init' <&$pfd
measure_check_ms 0
pt_expect_line 'cb hello' <&$pfd
measure_check_ms 0
pt_expect_line 'cb timeout' <&$pfd
measure_check_ms 1000
touch "$wakeup_fifo_file" || pt_fail "Cannot touch FIFO $wakeup_fifo_file."
pt_expect_line 'cb fifo' <&$pfd
measure_check_ms 0
pt_expect_line 'cb timeout' <&$pfd
measure_check_ms 1000
pt_expect_line 'cb timeout' <&$pfd
measure_check_ms 1000
pt_close_fd "$pfd"
pt_testcase_end
