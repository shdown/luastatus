if (( ! PLUGIN_DBUS_OPTIONAL )); then
    return 0
fi

x_dbus_begin
x_dbus_spawn_dbus_srv_py

pt_testcase_begin
pt_add_fifo "$main_fifo_file"
pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
$preface

local function do_call_plugin_function(f, params)
    local is_ok, res = f(params)
    assert(is_ok, res)
    return res
end

local function do_test_once(fmt, value, expected_result)
    local raw_res = do_call_plugin_function(luastatus.plugin.call_method, {
        bus = "session",
        dest = "io.github.shdown.luastatus.test",
        object_path = "/io/github/shdown/luastatus/test/MyObject",
        interface = "io.github.shdown.luastatus.test",
        method = "RecvVariant",
        args = luastatus.plugin.dbustypes.mkval_from_fmt(fmt, value),
    })
    local res = unpack1(raw_res)
    assert(type(res) == 'string')
    assert(res == expected_result)
end

local function test_all()
    do_test_once('(b)', {true}, 'bool')

    do_test_once('(y)', {42}, 'byte')
    do_test_once('(y)', {'x'}, 'byte')

    do_test_once('(d)', {1.25}, 'double')

    do_test_once('(s)', {'hello'}, 'string')
    do_test_once('(o)', {'/io/github/shdown/luastatus/foo/bar'}, 'object_path')
    do_test_once('(g)', {'(susssasa{sv}i)'}, 'signature')

    local wrapped = luastatus.plugin.dbustypes.mkval_from_fmt('s', 'hi there')
    do_test_once('(v)', {wrapped}, 'variant')

    local INT_TYPES = {
        {'(n)', 'i16'},
        {'(q)', 'u16'},
        {'(i)', 'i32'},
        {'(u)', 'u32'},
        {'(x)', 'i64'},
        {'(t)', 'u64'},
    }
    for _, pq in ipairs(INT_TYPES) do
        local fmt, expected_result = pq[1], pq[2]
        do_test_once(fmt, {53}, expected_result)
        do_test_once(fmt, {'58'}, expected_result)
    end
end

widget = {
    plugin = '$PT_BUILD_DIR/plugins/dbus/plugin-dbus.so',
    opts = {
        signals = {},
        greet = true,
    },
    cb = function(t)
        test_all()
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

x_dbus_kill_dbus_srv_py
x_dbus_end
