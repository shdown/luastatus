pt_testcase_begin
pt_add_fifo "$main_fifo_file"
pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
function check_events(e)
    assert(type(e) == 'table')

    -- These are supported by any glibc version.
    assert(e.access == 'io')
    assert(e.move == 'i')
    assert(e.isdir == 'o')

    -- Check sanity (all values are either "i", "o" or "io").
    for k, v in pairs(e) do
        print(string.format('Supported event: %s (%s)', k, v))
        assert(v == 'i' or v == 'o' or v == 'io')
    end
end
widget = {
    plugin = '$PT_BUILD_DIR/plugins/inotify/plugin-inotify.so',
    opts = {
        watch = {},
        greet = true,
    },
    cb = function(t)
        assert(t.what == 'hello')
        check_events(luastatus.plugin.get_supported_events())
        f:write('cb ok\n')
    end,
}
__EOF__
pt_spawn_luastatus
exec {pfd}<"$main_fifo_file"
pt_expect_line 'init' <&$pfd
pt_expect_line 'cb ok' <&$pfd
pt_testcase_end
