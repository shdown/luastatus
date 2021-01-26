main_fifo_file=./tmp-fifo-main

testcase_begin 'pipe/simple'
add_fifo "$main_fifo_file"
write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
function my_event_func() end
x = dofile('$SOURCE_DIR/plugins/pipe/pipe.lua')
widget = x.widget{
    command = 'echo one; sleep 1; echo two; sleep 1; echo three',
    cb = function(line)
        f:write('cb ' .. line .. '\n')
    end,
    on_eof = function()
        f:write('eof\n')
        while true do
        end
    end,
    event = my_event_func,
}
widget.plugin = ('$BUILD_DIR/plugins/{}/plugin-{}.so'):gsub('{}', widget.plugin)
assert(widget.event == my_event_func)
__EOF__
spawn_luastatus
exec 3<"$main_fifo_file"
expect_line 'init' <&3
measure_start
expect_line 'cb one' <&3
measure_check_ms 0
expect_line 'cb two' <&3
measure_check_ms 1000
expect_line 'cb three' <&3
measure_check_ms 1000
expect_line 'eof' <&3
measure_check_ms 0
exec 3<&-
testcase_end
