pt_testcase_begin

pt_add_fifo "$main_fifo_file"
pt_add_fifo "$R_fifo_file"

pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
widget = {
    plugin = '$PT_BUILD_DIR/plugins/llamacxx/plugin-llamacxx.so',
    opts = {
        greet = true,
        connection = {
            port = $port,
        },
        prompt = function()
            return 'REQUEST'
        end,
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
    end,
}
__EOF__
pt_spawn_luastatus

fakellama_spawn_and_wait_until_ready fxd:'{"content":"RESPONSE"}' 0

exec {pfd}<"$main_fifo_file"

pt_expect_line 'init' <&$pfd
pt_expect_line 'cb hello' <&$pfd

fakellama_expect_req '{"prompt":"REQUEST","response_fields":["content"],"cache_prompt":true}'

pt_expect_line 'cb answer RESPONSE' <&$pfd

fakellama_kill

pt_close_fd "$pfd"
pt_testcase_end
