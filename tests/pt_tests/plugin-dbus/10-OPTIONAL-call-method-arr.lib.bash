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

local function test_call_method()
    local args = luastatus.plugin.dbustypes.mkval_from_fmt('(as)', {{
        'one', 'two', 'three'
    }})
    local raw_res = do_call_plugin_function(luastatus.plugin.call_method, {
        bus = "session",
        dest = "io.github.shdown.luastatus.test",
        object_path = "/io/github/shdown/luastatus/test/MyObject",
        interface = "io.github.shdown.luastatus.test",
        method = "ConvertArrayToDictHexify",
        args = args,
    })
    local res = unpack1(raw_res)

    assert(type(res) == 'table')
    local outputs = {}
    for _, kv in ipairs(res) do
        table.insert(outputs, string.format('%s:%s', kv[1], kv[2]))
    end
    table.sort(outputs)
    local output = table.concat(outputs, ' ')
    assert(output == '30:6F6E65 31:74776F 32:7468726565')
end

widget = {
    plugin = '$PT_BUILD_DIR/plugins/dbus/plugin-dbus.so',
    opts = {
        signals = {},
        greet = true,
    },
    cb = function(t)
        test_call_method()
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
