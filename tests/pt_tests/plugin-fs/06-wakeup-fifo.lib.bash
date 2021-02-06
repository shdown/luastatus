pt_testcase_begin
using_measure

pt_add_fifo "$main_fifo_file"
pt_add_fifo "$wakeup_fifo_file"
pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
$preface
widget = {
    plugin = '$PT_BUILD_DIR/plugins/fs/plugin-fs.so',
    opts = {
        paths = {'/'},
        period = 0.5,
        fifo = '$wakeup_fifo_file',
    },
    cb = function(t)
        _validate_t(t, {'/'})
        f:write('cb called\n')
    end,
}
__EOF__
pt_spawn_luastatus
exec {pfd}<"$main_fifo_file"
measure_start
pt_expect_line 'init' <&$pfd
measure_check_ms 0
pt_expect_line 'cb called' <&$pfd
measure_check_ms 0
pt_expect_line 'cb called' <&$pfd
measure_check_ms 500
pt_expect_line 'cb called' <&$pfd
measure_check_ms 500
touch "$wakeup_fifo_file"
pt_expect_line 'cb called' <&$pfd
measure_check_ms 0
pt_expect_line 'cb called' <&$pfd
measure_check_ms 500
pt_close_fd "$pfd"
pt_testcase_end
