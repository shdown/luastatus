pt_testcase_begin

httpserv_spawn --max-requests=1 --freeze-for=10000 GET /

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
                        timeout = 2.0,
                    },
                })
                coroutine.yield({action = 'sleep', period = 0.1})
            end
        end,
    },
    cb = function(t)
        assert(t.what == 'response')
        if t.error then
            f:write('error\n')
        else
            f:write('resp ' .. t.body .. '\n')
        end
    end,
}
__EOF__
pt_spawn_luastatus
exec {pfd}<"$main_fifo_file"
pt_expect_line 'init' <&$pfd

httpserv_expect '>'
httpserv_say 'RESPONSE'
pt_expect_line 'resp RESPONSE' <&$pfd

pt_expect_line 'error' <&$pfd

pt_close_fd "$pfd"
pt_testcase_end
