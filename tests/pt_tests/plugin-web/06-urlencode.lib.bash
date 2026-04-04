pt_testcase_begin
pt_add_fifo "$main_fifo_file"
pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')

local function check(src, plus_encoding, expected)
    local r = luastatus.plugin.urlencode(src, plus_encoding)
    if r ~= expected then
        print('Expected:', expected)
        print('Found:', r)
        print('plus_encoding:', plus_encoding)
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

        check('test!foo?bar', true,  'test%21foo%3Fbar')
        check('test!foo?bar', false, 'test%21foo%3Fbar')

        check('check it', true, 'check+it')
        check('check it', false, 'check%20it')

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
