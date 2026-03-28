pt_testcase_begin
pt_add_fifo "$main_fifo_file"
pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')

widget = {
    plugin = '$PT_BUILD_DIR/plugins/pipev2/plugin-pipev2.so',
    opts = {
        argv = {'/bin/sh', '-c', 'exit 0'},
        greet = true,
    },
    cb = function(t)
        if t.what == 'hello' then
            min, max = luastatus.plugin.get_sigrt_bounds()
            assert(min < max)
            print(string.format('sigrt bounds: min=%d, max=%d', min, max))
            f:write('ok\n')
        end
    end,
}
__EOF__
pt_spawn_luastatus
exec {pfd}<"$main_fifo_file"
pt_expect_line 'init' <&$pfd
pt_expect_line 'ok' <&$pfd

pt_close_fd "$pfd"
pt_testcase_end
