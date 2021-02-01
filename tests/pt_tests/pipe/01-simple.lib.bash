pt_testcase_begin
pt_add_fifo "$main_fifo_file"
pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
function my_event_func() end
x = dofile('$PT_SOURCE_DIR/plugins/pipe/pipe.lua')
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
widget.plugin = ('$PT_BUILD_DIR/plugins/{}/plugin-{}.so'):gsub('{}', widget.plugin)
assert(widget.event == my_event_func)
__EOF__
pt_spawn_luastatus
exec {pfd}<"$main_fifo_file"
pt_expect_line 'init' <&$pfd
measure_start
pt_expect_line 'cb one' <&$pfd
measure_check_ms 0
pt_expect_line 'cb two' <&$pfd
measure_check_ms 1000
pt_expect_line 'cb three' <&$pfd
measure_check_ms 1000
pt_expect_line 'eof' <&$pfd
measure_check_ms 0
pt_close_fd "$pfd"
pt_testcase_end
