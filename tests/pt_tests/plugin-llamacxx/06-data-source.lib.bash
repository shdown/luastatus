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
        connection = {
            port = $port,
        },
        upd_timeout = 0,
        data_sources = {
            abcdef = [[
                i = 0
                LL_data_source = {
                    plugin = 'timer',
                    opts = {
                        period = 0.1,
                    },
                    cb = function(t)
                        i = i + 1
                        return string.format('i=%d,t=%s', i, t)
                    end,
                }
            ]],
        },
        prompt = function(data)
            return string.format('abcdef=(%s)', data.abcdef)
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

fakellama_spawn_and_wait_until_ready dyn:'{"content":"OK,@"}' 0

exec {pfd}<"$main_fifo_file"

pt_expect_line 'init' <&$pfd

for (( i = 1; i <= 7; ++i )); do
    if (( i == 1 )); then
        expected_t=hello
    else
        expected_t=timeout
    fi
    expected_prompt="abcdef=(i=${i},t=${expected_t})"
    fakellama_expect_req '{"prompt":"'"$expected_prompt"'","response_fields":["content"],"cache_prompt":true}'
    pt_expect_line "cb answer OK,${i}" <&$pfd
done

fakellama_kill

pt_close_fd "$pfd"
pt_testcase_end
