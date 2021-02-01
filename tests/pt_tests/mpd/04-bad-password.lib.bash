pt_testcase_begin

fakempd_spawn

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
exec {pfd}<"$main_fifo_file"
pt_expect_line 'init' <&$pfd
pt_expect_line 'cb connecting' <&$pfd

fakempd_say "OK MPD I-am-actually-a-shell-script"

fakempd_expect 'password "this is password with \"QUOTE MARKS\" ha-ha"'
fakempd_say 'ACK wrong password (even though it is with "QUOTE MARKS" he-he)'

pt_expect_line 'cb error' <&$pfd

fakempd_wait

pt_testcase_end
