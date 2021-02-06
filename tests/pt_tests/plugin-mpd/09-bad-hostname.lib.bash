pt_testcase_begin
fakempd_spawn
pt_write_widget_file <<__EOF__
widget = {
    plugin = '$PT_BUILD_DIR/plugins/mpd/plugin-mpd.so',
    opts = {
        port = $port,
        hostname = '255.255.255.255',
        retry_in = -1,
    },
    cb = function(t) end,
}
__EOF__
pt_spawn_luastatus -e
pt_wait_luastatus || pt_fail "luastatus exited with non-zero code $?"

fakempd_kill

pt_testcase_end
