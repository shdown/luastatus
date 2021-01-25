# Regression test for https://github.com/shdown/luastatus/issues/63.

testcase_begin 'simul/err'
for (( i = 0; i < 8; ++i )); do
    write_widget_file <<__EOF__
widget = {
    plugin = '$BUILD_DIR/plugins/mpd/plugin-mpd.so',
    opts = {
        port = 0,
        retry_in = -1,
    },
    cb = function(t) end,
}
__EOF__
done
spawn_luastatus -e
wait_luastatus || fail "luastatus exited with non-zero code"
testcase_end
