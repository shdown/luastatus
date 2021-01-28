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
exec 3<"$main_fifo_file"
pt_expect_line 'init' <&3
wait_for_socket_to_appear
send_verbatim_to_socket $'one\n'
pt_expect_line 'line one' <&3
send_verbatim_to_socket $'two\n'
pt_expect_line 'line two' <&3
send_verbatim_to_socket 'without newline'
send_verbatim_to_socket $'with newline\n'
pt_expect_line 'line with newline' <&3
send_verbatim_to_socket $'aypibmmwxrdxdknh\ngmstvxwxamouhlmw\ncybymucjtfxrauwn'
pt_expect_line 'line aypibmmwxrdxdknh' <&3
exec 3<&-
pt_testcase_end
