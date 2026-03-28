pt_testcase_begin
pt_add_fifo "$main_fifo_file"
pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')

widget = {
    plugin = '$PT_BUILD_DIR/plugins/pipev2/plugin-pipev2.so',
    opts = {
        argv = {'/bin/sh', '-c', 'echo one; echo two; echo three'},
    },
    cb = function(t)
        if t.what == 'line' then
            f:write('cb line ' .. t.line .. '\n')
        else
            f:write('cb ' .. t.what .. '\n')
        end
    end,
}
__EOF__
pt_spawn_luastatus
exec {pfd}<"$main_fifo_file"
pt_expect_line 'init' <&$pfd
pt_expect_line 'cb line one' <&$pfd
pt_expect_line 'cb line two' <&$pfd
pt_expect_line 'cb line three' <&$pfd
pt_close_fd "$pfd"
pt_testcase_end
