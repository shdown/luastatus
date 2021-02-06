# Regression test for https://github.com/shdown/luastatus/issues/63.

pt_testcase_begin
for (( i = 0; i < 8; ++i )); do
    pt_write_widget_file <<__EOF__
widget = {
    plugin = '$PT_BUILD_DIR/plugins/mpd/plugin-mpd.so',
    opts = {
        port = 0,
        retry_in = -1,
    },
    cb = function(t) end,
}
__EOF__
done
pt_spawn_luastatus -e
pt_wait_luastatus || pt_fail "luastatus exited with non-zero code $?"
pt_testcase_end
