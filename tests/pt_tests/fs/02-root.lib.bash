pt_testcase_begin
using_measure
pt_add_fifo "$main_fifo_file"
pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
$preface
widget = {
    plugin = '$PT_BUILD_DIR/plugins/fs/plugin-fs.so',
    opts = {
        paths = {'/'},
        period = 0.25,
    },
    cb = function(t)
        _validate_t(t, {'/'})
        f:write('cb ok\n')
    end,
}
__EOF__
pt_spawn_luastatus
exec {pfd}<"$main_fifo_file"
measure_start
pt_expect_line 'init' <&$pfd
measure_check_ms 0
pt_expect_line 'cb ok' <&$pfd
measure_check_ms 0
pt_expect_line 'cb ok' <&$pfd
measure_check_ms 250
pt_expect_line 'cb ok' <&$pfd
measure_check_ms 250
pt_close_fd "$pfd"
pt_testcase_end
