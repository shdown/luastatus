pt_testcase_begin
pt_add_fifo "$main_fifo_file"
pt_add_file_to_remove "$socket_file"
pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
widget = {
    plugin = '$PT_BUILD_DIR/plugins/unixsock/plugin-unixsock.so',
    opts = {
        path = '$socket_file',
        greet = true,
    },
    cb = function(t)
        if t.what == 'line' then
            f:write('line ' .. t.line .. '\n')
        else
            f:write(t.what .. '\n')
        end
    end,
}
__EOF__
pt_spawn_luastatus
exec {pfd}<"$main_fifo_file"
pt_expect_line 'init' <&$pfd
unixsock_wait_socket
pt_expect_line 'hello' <&$pfd
unixsock_send_verbatim $'one\n'
pt_expect_line 'line one' <&$pfd
unixsock_send_verbatim $'two\n'
pt_expect_line 'line two' <&$pfd
unixsock_send_verbatim $'three\n'
pt_expect_line 'line three' <&$pfd
pt_close_fd "$pfd"
pt_testcase_end
