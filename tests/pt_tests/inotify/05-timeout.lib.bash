pt_testcase_begin
pt_add_fifo "$main_fifo_file"
myfile=$(mktemp) || pt_fail "Cannot create temporary file."
pt_add_file_to_remove "$myfile"
pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
$preface
widget = {
    plugin = '$PT_BUILD_DIR/plugins/inotify/plugin-inotify.so',
    opts = {
        watch = {['$myfile'] = {'close_write', 'delete_self'}},
        timeout = 0.3,
        greet = true,
    },
    cb = function(t)
        if t.what == 'event' then
            f:write('cb event mask=' .. _fmt_mask(t.mask) .. '\n')
        else
            f:write('cb ' .. t.what .. '\n')
        end
    end,
}
__EOF__
pt_spawn_luastatus
exec {pfd}<"$main_fifo_file"

pt_expect_line 'init' <&$pfd
pt_expect_line 'cb hello' <&$pfd

measure_start
pt_expect_line 'cb timeout' <&$pfd
measure_check_ms 300
pt_expect_line 'cb timeout' <&$pfd
measure_check_ms 300

echo hello >> "$myfile" || pt_fail "Cannot write to $myfile."
pt_expect_line 'cb event mask=close_write' <&$pfd
measure_check_ms 0

pt_expect_line 'cb timeout' <&$pfd
measure_check_ms 300

echo hello >> "$myfile" || pt_fail "Cannot write to $myfile."
pt_expect_line 'cb event mask=close_write' <&$pfd
measure_check_ms 0

rm -f "$myfile" || pt_fail "Cannot remove $myfile."
pt_expect_line 'cb event mask=delete_self' <&$pfd
pt_expect_line 'cb event mask=ignored' <&$pfd
measure_check_ms 0

pt_close_fd "$pfd"
pt_testcase_end
