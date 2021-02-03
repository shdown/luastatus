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
    },
    cb = function(t)
        assert(t.what == 'line')
        f:write('line ' .. t.line .. '\n')
    end,
}
__EOF__
pt_spawn_luastatus
exec {pfd}<"$main_fifo_file"
pt_expect_line 'init' <&$pfd
unixsock_wait_socket
unixsock_send_verbatim $'one\n'
pt_expect_line 'line one' <&$pfd
unixsock_send_verbatim $'two\n'
pt_expect_line 'line two' <&$pfd
unixsock_send_verbatim 'without newline'
unixsock_send_verbatim $'with newline\n'
pt_expect_line 'line with newline' <&$pfd
unixsock_send_verbatim $'aypibmmwxrdxdknh\ngmstvxwxamouhlmw\ncybymucjtfxrauwn'
pt_expect_line 'line aypibmmwxrdxdknh' <&$pfd
pt_close_fd "$pfd"
pt_testcase_end
