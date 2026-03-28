pt_testcase_begin
pt_add_fifo "$main_fifo_file"
pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')

widget = {
    plugin = '$PT_BUILD_DIR/plugins/pipev2/plugin-pipev2.so',
    opts = {
        argv = {'/usr/bin/env', 'bash', '-c', 'for (( i=0; ; ++i)); do echo \$i; sleep 1; done'},
        greet = true,
        bye = true,
    },
    cb = function(t)
        if t.what == 'line' then
            f:write('cb line ' .. t.line .. '\n')
            if t.line == '3' then
                luastatus.plugin.kill()
            end
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
pt_expect_line 'cb line 0' <&$pfd
pt_expect_line 'cb line 1' <&$pfd
pt_expect_line 'cb line 2' <&$pfd
pt_expect_line 'cb line 3' <&$pfd
pt_expect_line 'cb bye' <&$pfd

pt_close_fd "$pfd"
pt_testcase_end
