pt_testcase_begin
using_measure

pt_add_fifo "$main_fifo_file"
pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
n=0
widget = {
    plugin = '$PT_BUILD_DIR/plugins/timer/plugin-timer.so',
    opts = {
        period = 0.1,
    },
    cb = function(t)
        if t == 'timeout' then
            n = (n+1)%3
            f:write('cb timeout n=' .. n .. '\n')
            if n == 0 then
                luastatus.plugin.push_period(0.6)
            end
        else
            f:write('cb ' .. t .. '\n')
        end
    end,
}
__EOF__
pt_spawn_luastatus
exec {pfd}<"$main_fifo_file"
pt_expect_line 'init' <&$pfd
measure_start
pt_expect_line 'cb hello' <&$pfd
measure_check_ms 0
pt_expect_line 'cb timeout n=1' <&$pfd
measure_check_ms 100
for (( i = 0; i < 3; ++i )); do
    pt_expect_line 'cb timeout n=2' <&$pfd
    measure_check_ms 100
    pt_expect_line 'cb timeout n=0' <&$pfd
    measure_check_ms 100
    pt_expect_line 'cb timeout n=1' <&$pfd
    measure_check_ms 600
done
pt_close_fd "$pfd"
pt_testcase_end
