pt_testcase_begin
fakempd_spawn
pt_write_widget_file <<__EOF__
widget = {
    plugin = '$PT_BUILD_DIR/plugins/mpd/plugin-mpd.so',
    opts = {
        port = $port,
        retry_in = -1,
    },
    cb = function(t) end,
}
__EOF__
pt_spawn_luastatus -e
fakempd_say "I'm not music player daemon, huh."
pt_wait_luastatus || pt_fail "luastatus exited with non-zero exit code $?"

fakempd_wait

pt_testcase_end
