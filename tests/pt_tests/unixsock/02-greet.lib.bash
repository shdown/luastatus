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
exec 3<"$main_fifo_file"
pt_expect_line 'init' <&3
wait_for_socket_to_appear
pt_expect_line 'hello' <&3
send_verbatim_to_socket $'one\n'
pt_expect_line 'line one' <&3
send_verbatim_to_socket $'two\n'
pt_expect_line 'line two' <&3
send_verbatim_to_socket $'three\n'
pt_expect_line 'line three' <&3
exec 3<&-
pt_testcase_end
