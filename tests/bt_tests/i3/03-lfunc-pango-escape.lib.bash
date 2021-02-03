bt_testcase_begin
bt_add_fifo "$main_fifo_file"
bt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
local function check(arg, expected)
    local result = luastatus.barlib.pango_escape(arg)
    if result ~= expected then
        print('~~~~~~~~~~~~~')
        print('Argument: ' .. arg)
        print('Expected: ' .. expected)
        print('Found:    ' .. result)
        print('~~~~~~~~~~~~~')
        error('check failed')
    end
end
widget = {
    plugin = '$BT_BUILD_DIR/tests/plugin-mock.so',
    opts = {make_calls = 1},
    cb = function()
        check('', '')
        check('hello', 'hello')
        check('hello & world', 'hello &amp; world')
        check('<>', '&lt;&gt;')

        f:write('ok\n')
    end,
}
__EOF__
x_spawn_luastatus
exec {pfd}<"$main_fifo_file"
bt_expect_line 'ok' <&$pfd
bt_close_fd "$pfd"
bt_testcase_end
