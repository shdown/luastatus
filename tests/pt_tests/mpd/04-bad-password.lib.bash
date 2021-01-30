pt_testcase_begin

coproc "$PT_PARROT" --reuseaddr --print-line-when-ready TCP-SERVER "$port" \
    || pt_fail "coproc failed"

pt_add_spawned_thing parrot "$COPROC_PID"

pt_expect_line 'parrot: ready' <&${COPROC[0]}

pt_add_fifo "$main_fifo_file"
pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
$preface
widget = {
    plugin = '$PT_BUILD_DIR/plugins/mpd/plugin-mpd.so',
    opts = {
        port = $port,
        password = 'this is password with "QUOTE MARKS" ha-ha',
    },
    cb = function(t)
        f:write('cb ' .. t.what .. '\n')
    end,
}
__EOF__
pt_spawn_luastatus
exec 3<"$main_fifo_file"
pt_expect_line 'init' <&3
pt_expect_line 'cb connecting' <&3
echo "OK MPD I-am-actually-a-shell-script" >&${COPROC[1]}

pt_expect_line 'password "this is password with \"QUOTE MARKS\" ha-ha"' <&${COPROC[0]}
echo 'ACK wrong password (even though it is with "QUOTE MARKS" he-he)' >&${COPROC[1]}

pt_expect_line 'cb error' <&3

pt_wait_thing parrot

pt_testcase_end
