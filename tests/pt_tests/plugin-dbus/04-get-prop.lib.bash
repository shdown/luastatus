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

        local is_ok, res = luastatus.plugin.get_property({
            bus = "session",
            object_path = "/org/freedesktop/DBus",
            prop_owner = "org.freedesktop.DBus",
            prop_name = "Features",
        })
        assert(is_ok, res)

        local features = unpack1(res)
        assert(type(features) == 'table', 'features is not an array')
        for _, x in ipairs(features) do
            print('Feature:', x)
            assert(type(x) == 'string', 'feature is not a string')
        end

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
