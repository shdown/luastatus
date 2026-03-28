pt_testcase_begin
pt_add_fifo "$main_fifo_file"
pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')

i = 0

widget = {
    plugin = '$PT_BUILD_DIR/plugins/pipev2/plugin-pipev2.so',
    opts = {
        argv = {'/bin/sh', '-c', 'while read x; do echo "you said <\$x>"; done'},
        greet = true,
        pipe_stdin = true,
    },
    cb = function(t)
        assert(luastatus.plugin.write_to_stdin(string.format('%d\n', i)))
        i = i + 1

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
pt_expect_line 'cb hello' <&$pfd
pt_expect_line 'cb line you said <0>' <&$pfd
pt_expect_line 'cb line you said <1>' <&$pfd
pt_expect_line 'cb line you said <2>' <&$pfd

pt_close_fd "$pfd"
pt_testcase_end
