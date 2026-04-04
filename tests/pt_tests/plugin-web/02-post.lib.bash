pt_testcase_begin

httpserv_spawn POST /

pt_add_fifo "$main_fifo_file"
pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
widget = {
    plugin = '$PT_BUILD_DIR/plugins/web/plugin-web.so',
    opts = {
        planner = function()
            while true do
                coroutine.yield({
                    action = 'request',
                    params = {
                        url = 'http://127.0.0.1:$port/',
                        post_fields = 'test=TEST',
                    },
                })
                coroutine.yield({action = 'sleep', period = 1.0})
            end
        end,
    },
    cb = function(t)
        assert(t.what == 'response')
        f:write('resp ' .. t.body .. '\n')
    end,
}
__EOF__
pt_spawn_luastatus
exec {pfd}<"$main_fifo_file"
pt_expect_line 'init' <&$pfd

httpserv_expect '>test=TEST'
httpserv_say 'RESPONSE'
pt_expect_line 'resp RESPONSE' <&$pfd

pt_close_fd "$pfd"
pt_testcase_end
