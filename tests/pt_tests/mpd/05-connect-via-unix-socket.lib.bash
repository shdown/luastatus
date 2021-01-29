pt_testcase_begin

socket_file=$PWD/tmp-fake-mpd-socket
rm -f "$socket_file" || pt_fail "Cannot rm $socket_file."

pt_add_file_to_remove "$socket_file"

coproc socat stdio "UNIX-LISTEN:$socket_file" || pt_fail "coproc failed"
pt_add_spawned_thing socat "$COPROC_PID"

echo >&2 "Waiting for socket file '$socket_file' to appear..."
while ! [[ -S "$socket_file" ]]; do
    true
done

pt_add_fifo "$main_fifo_file"
pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
$preface
widget = {
    plugin = '$PT_BUILD_DIR/plugins/mpd/plugin-mpd.so',
    opts = {
        hostname = '$socket_file',
    },
    cb = function(t)
        if t.what == 'update' then
            f:write(string.format('cb update song=%s status=%s\n', _fmt_kv(t.song), _fmt_kv(t.status)))
        else
            f:write('cb ' .. t.what .. '\n')
        end
    end,
}
__EOF__
pt_spawn_luastatus
exec 3<"$main_fifo_file"
pt_expect_line 'init' <&3
pt_expect_line 'cb connecting' <&3
echo "OK MPD I-am-actually-a-shell-script" >&${COPROC[1]}

for (( i = 0; i < 3; ++i )); do
    pt_expect_line 'currentsong' <&${COPROC[0]}
    echo "Song_Foo: bar" >&${COPROC[1]}
    echo "Song_Baz: quiz" >&${COPROC[1]}
    echo "OK" >&${COPROC[1]}

    pt_expect_line 'status' <&${COPROC[0]}
    echo "Status_One: ein" >&${COPROC[1]}
    echo "Status_Two: zwei" >&${COPROC[1]}
    echo "Status_Three: drei" >&${COPROC[1]}
    echo "Z: $i" >&${COPROC[1]}
    echo "OK" >&${COPROC[1]}

    pt_expect_line 'idle mixer player' <&${COPROC[0]}
    pt_expect_line "cb update song={Song_Baz=>quiz,Song_Foo=>bar} status={Status_One=>ein,Status_Three=>drei,Status_Two=>zwei,Z=>$i}" <&3

    echo "OK" >&${COPROC[1]}
done

pt_close_fd "${COPROC[0]}"
pt_close_fd "${COPROC[1]}"
pt_kill_thing socat

pt_expect_line 'cb error' <&3

pt_testcase_end
