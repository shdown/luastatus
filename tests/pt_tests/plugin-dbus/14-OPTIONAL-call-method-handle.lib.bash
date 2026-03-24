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

local function do_test()
    local is_ok, err_msg = pcall(luastatus.plugin.dbustypes.mkval_from_fmt, 'h', 42)
    assert(not is_ok, 'expected error to be thrown from "mkval_from_fmt"')
    assert(err_msg:find('creation of handles is not supported') ~= nil)
end

widget = {
    plugin = '$PT_BUILD_DIR/plugins/dbus/plugin-dbus.so',
    opts = {
        signals = {},
        greet = true,
    },
    cb = function(t)
        do_test()
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
