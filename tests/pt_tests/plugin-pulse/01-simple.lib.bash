pt_testcase_begin

pt_spawn_thing_pipe pulsetalker "$PT_SOURCE_DIR"/tests/pulsetalker.sh "$sink_name"
pt_expect_line 'ready' <&${PT_SPAWNED_THINGS_FDS_0[pulsetalker]}

pacmd set-sink-mute "$sink_name" false
pacmd set-sink-volume "$sink_name" 65536

pt_add_fifo "$main_fifo_file"
pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
local last_line = nil
widget = {
    plugin = '$PT_BUILD_DIR/plugins/pulse/plugin-pulse.so',
    opts = {
        sink = '$sink_name',
    },
    cb = function(t)
        assert(t)
        local line = string.format('[%s] cb %.0f%%', t.name, t.cur / t.norm * 100)
        if t.mute then
            line = line .. ' (mute)'
        end
        if line ~= last_line then
            f:write(line .. '\n')
        end
        last_line = line
    end,
}
__EOF__
pt_spawn_luastatus
exec {pfd}<"$main_fifo_file"
pt_expect_line 'init' <&$pfd
pt_expect_line "[$sink_name] cb 100%" <&$pfd

pacmd set-sink-volume "$sink_name" 32768
pt_expect_line "[$sink_name] cb 50%" <&$pfd

pacmd set-sink-mute "$sink_name" true
pt_expect_line "[$sink_name] cb 50% (mute)" <&$pfd

pacmd set-sink-volume "$sink_name" 2048
pt_expect_line "[$sink_name] cb 3% (mute)" <&$pfd

pt_close_fd "$pfd"
pt_testcase_end
