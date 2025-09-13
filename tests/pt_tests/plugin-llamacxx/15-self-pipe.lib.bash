pt_testcase_begin
using_measure

pt_add_fifo "$main_fifo_file"
pt_add_fifo "$R_fifo_file"

pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
i = 0
widget = {
    plugin = '$PT_BUILD_DIR/plugins/llamacxx/plugin-llamacxx.so',
    opts = {
        greet = true,
        connection = {
            port = $port,
        },
        prompt = function()
            i = i + 1
            return string.format('request %d', i)
        end,
        upd_timeout = 0.5,
        make_self_pipe = true,
    },
    cb = function(t)
        if t.what == 'answer' then
            f:write('cb answer ' .. t.answer .. '\n')
        elseif t.what == 'error' then
            f:write('cb error')
            f:write(' error=' .. t.error)
            f:write(' meta=' .. t.meta)
            f:write('\n')
        else
            f:write('cb ' .. t.what .. '\n')
        end
        if i == 5 then
            luastatus.plugin.wake_up()
        end
    end,
}
__EOF__
pt_spawn_luastatus

fakellama_spawn_and_wait_until_ready dyn:'{"content":"response @"}' 0

exec {pfd}<"$main_fifo_file"

pt_expect_line 'init' <&$pfd
pt_expect_line 'cb hello' <&$pfd

fakellama_expect_req '{"prompt":"request 1","response_fields":["content"],"cache_prompt":true}'
pt_expect_line 'cb answer response 1' <&$pfd

measure_start

for (( i = 2; i <= 5; ++i )); do
    fakellama_expect_req '{"prompt":"request '$i'","response_fields":["content"],"cache_prompt":true}'
    measure_check_ms 500
    pt_expect_line "cb answer response $i" <&$pfd
done

pt_expect_line 'cb fifo' <&$pfd
measure_check_ms 0

fakellama_kill

pt_close_fd "$pfd"
pt_testcase_end
