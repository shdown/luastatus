coproc socat stdio "TCP-LISTEN:$port,reuseaddr,bind=localhost" || pt_fail "coproc failed"
pt_add_spawned_thing socat "$COPROC_PID"

mpd_wait_for_socat

pt_testcase_begin
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
echo "I'm not music player daemon, huh." >&${COPROC[1]}
pt_wait_luastatus || pt_fail "luastatus exited with non-zero code"

pt_close_fd "${COPROC[0]}"
pt_close_fd "${COPROC[1]}"
pt_kill_thing socat

pt_testcase_end
