pt_testcase_begin
pt_add_fifo "$main_fifo_file"
true > "$socket_file" || pt_fail "Cannot create regular file $socket_file."
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
exec 3<"$main_fifo_file"
pt_expect_line 'init' <&3
unixsock_wait_socket
unixsock_send_verbatim $'aloha\n'
pt_expect_line 'line aloha' <&3
exec 3<&-
pt_testcase_end
