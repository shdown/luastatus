pt_testcase_begin
pt_add_fifo "$main_fifo_file"
pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
widget = {
    plugin = '$PT_BUILD_DIR/plugins/web/plugin-web.so',
    opts = {
        planner = function()
            while true do
                coroutine.yield({action = 'call_cb', what = 'hi there'})
                coroutine.yield({action = 'sleep', period = 0.1})
            end
        end,
    },
    cb = function(t)
        assert(t.what == 'hi there')
        f:write(t.what .. '\n')
    end,
}
__EOF__
pt_spawn_luastatus
exec {pfd}<"$main_fifo_file"
pt_expect_line 'init' <&$pfd

for (( i = 0; i < 5; ++i )); do
    pt_expect_line 'hi there' <&$pfd
done

pt_close_fd "$pfd"
pt_testcase_end
