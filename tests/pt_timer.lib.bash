main_fifo_file=./tmp-fifo-main
wakeup_fifo_file=./tmp-fifo-wakeup

testcase_begin 'timer/default'
add_fifo "$main_fifo_file"
write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
widget = {
    plugin = '$BUILD_DIR/plugins/timer/plugin-timer.so',
    cb = function(t)
        f:write('cb ' .. t .. '\n')
    end,
}
__EOF__
spawn_luastatus
exec 3<"$main_fifo_file"
measure_start
expect_line 'init' <&3
measure_check_ms 0
expect_line 'cb hello' <&3
measure_check_ms 0
expect_line 'cb timeout' <&3
measure_check_ms 1000
expect_line 'cb timeout' <&3
measure_check_ms 1000
exec 3<&-
testcase_end

testcase_begin 'timer/period'
add_fifo "$main_fifo_file"
write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
widget = {
    plugin = '$BUILD_DIR/plugins/timer/plugin-timer.so',
    opts = {
        period = 0.1,
    },
    cb = function(t)
        f:write('cb ' .. t .. '\n')
    end,
}
__EOF__
spawn_luastatus
exec 3<"$main_fifo_file"
measure_start
expect_line 'init' <&3
measure_check_ms 0
expect_line 'cb hello' <&3
measure_check_ms 0
expect_line 'cb timeout' <&3
measure_check_ms 100
expect_line 'cb timeout' <&3
measure_check_ms 100
exec 3<&-
testcase_end

testcase_begin 'timer/wakeup_fifo'
add_fifo "$main_fifo_file"
add_fifo "$wakeup_fifo_file"
write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
widget = {
    plugin = '$BUILD_DIR/plugins/timer/plugin-timer.so',
    opts = {
        fifo = '$wakeup_fifo_file',
    },
    cb = function(t)
        f:write('cb ' .. t .. '\n')
    end,
}
__EOF__
spawn_luastatus
exec 3<"$main_fifo_file"
measure_start
expect_line 'init' <&3
measure_check_ms 0
expect_line 'cb hello' <&3
measure_check_ms 0
expect_line 'cb timeout' <&3
measure_check_ms 1000
touch "$wakeup_fifo_file" || fail "Cannot touch FIFO $wakeup_fifo_file."
expect_line 'cb fifo' <&3
measure_check_ms 0
expect_line 'cb timeout' <&3
measure_check_ms 1000
expect_line 'cb timeout' <&3
measure_check_ms 1000
exec 3<&-
testcase_end

testcase_begin 'timer/lfuncs'
add_fifo "$main_fifo_file"
write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
n=0
widget = {
    plugin = '$BUILD_DIR/plugins/timer/plugin-timer.so',
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
spawn_luastatus
exec 3<"$main_fifo_file"
measure_start
expect_line 'init' <&3
measure_check_ms 0
expect_line 'cb hello' <&3
measure_check_ms 0
expect_line 'cb timeout n=1' <&3
measure_check_ms 100
for (( i = 0; i < 3; ++i )); do
    expect_line 'cb timeout n=2' <&3
    measure_check_ms 100
    expect_line 'cb timeout n=0' <&3
    measure_check_ms 100
    expect_line 'cb timeout n=1' <&3
    measure_check_ms 600
done
exec 3<&-
testcase_end
