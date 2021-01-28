pt_testcase_begin
pt_add_fifo "$main_fifo_file"
pt_add_file_to_remove "$socket_file"
pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
n=0
widget = {
    plugin = '$PT_BUILD_DIR/plugins/unixsock/plugin-unixsock.so',
    opts = {
        path = '$socket_file',
        greet = true,
        timeout = 0.1,
    },
    cb = function(t)
        if t.what == 'line' then
            f:write('line ' .. t.line .. '\n')
        else
            n=(n+1)%3
            f:write(t.what .. ' n=' .. n .. '\n')
            if n == 0 then
                luastatus.plugin.push_timeout(0.6)
            end
        end
    end,
}
__EOF__
pt_spawn_luastatus
exec 3<"$main_fifo_file"
pt_expect_line 'init' <&3
wait_for_socket_to_appear
pt_expect_line 'hello n=1' <&3
measure_start
for (( i = 0; i < 4; ++i )); do
    send_verbatim_to_socket "foobar$i"$'\n'
    pt_expect_line "line foobar$i" <&3
    measure_check_ms 0
    pt_expect_line 'timeout n=2' <&3
    measure_check_ms 100
    pt_expect_line 'timeout n=0' <&3
    measure_check_ms 100
    pt_expect_line 'timeout n=1' <&3
    measure_check_ms 600
done
exec 3<&-
pt_testcase_end
