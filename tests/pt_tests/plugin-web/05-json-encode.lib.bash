pt_testcase_begin
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
                coroutine.yield({action = 'call_cb', what = 'test'})
                coroutine.yield({action = 'sleep', period = 1.0})
            end
        end,
    },
    cb = function(t)
        assert(t.what == 'test')

        local r

        r = luastatus.plugin.json_encode_str([[xyz"\\]] .. '\n')
        assert(r == [[xyz\u0022\u005C\u000A]])

        r = luastatus.plugin.json_encode_num(0.5)
        assert(tonumber(r) == 0.5)

        f:write('ok\n')
    end,
}
__EOF__
pt_spawn_luastatus
exec {pfd}<"$main_fifo_file"
pt_expect_line 'init' <&$pfd

pt_expect_line 'ok' <&$pfd

pt_close_fd "$pfd"
pt_testcase_end
