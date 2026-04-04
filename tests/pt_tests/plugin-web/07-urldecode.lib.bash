pt_testcase_begin
pt_add_fifo "$main_fifo_file"
pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')

local function check(src, expected)
    local r = luastatus.plugin.urldecode(src)
    if r ~= expected then
        print('Expected:', expected)
        print('Found:', r)
        error('check() failed')
    end
end

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

        check('test', 'test')
        check('foo+bar', 'foo bar')

        check('test%20case', 'test case')
        check('what%3F', 'what?')
        check('what%3f', 'what?')

        check('xx%xx', nil)
        check('foo%', nil)
        check('foo%1', nil)
        check('foo%1x', nil)

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
