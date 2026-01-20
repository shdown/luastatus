pt_require_tools dbus-send

x_dbus_begin

pt_testcase_begin
pt_add_fifo "$main_fifo_file"
pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
$preface
widget = {
    plugin = '$PT_BUILD_DIR/plugins/dbus/plugin-dbus.so',
    opts = {
        signals = {},
        greet = true,
    },
    cb = function(t)
        assert(t.what == 'hello', 't.what is not "hello"')

        local is_ok, res = luastatus.plugin.get_all_properties({
            bus = "system",
            object_path = "/org/freedesktop/DBus",
            prop_owner = "org.freedesktop.DBus",
        })
        assert(is_ok, res)

        local arr = unpack1(res)

        assert(type(arr) == 'table', 'result is not a table')

        local props_set = {}
        for _, x in ipairs(arr) do
            local name, value = unpack2(x)
            print('Property:', name, value)
            assert(type(name) == 'string', 'property name is not a string')
            props_set[name] = true
        end

        assert(props_set['Features'], 'no "Features" property')
        assert(props_set['Interfaces'], 'no "Interfaces" property')

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

x_dbus_end
