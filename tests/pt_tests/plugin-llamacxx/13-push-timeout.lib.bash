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
    },
    cb = function(t)
        if t.what == 'answer' then
            f:write('cb answer ' .. t.answer .. '\n')
            if t.answer:find('5') then
                f:write('pushing timeout\n')
                luastatus.plugin.push_timeout(1.0)
            end
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

fakellama_spawn_and_wait_until_ready dyn:'{"content":"response @"}' 0

exec {pfd}<"$main_fifo_file"

pt_expect_line 'init' <&$pfd
pt_expect_line 'cb hello' <&$pfd

expect_request_and_response() {
    local num=$1
    local check=$2
    fakellama_expect_req '{"prompt":"request '"$num"'","response_fields":["content"],"cache_prompt":true}'
    if (( check >= 0 )); then
        measure_check_ms "$check"
    fi
    pt_expect_line "cb answer response $num" <&$pfd
}

expect_request_and_response 1 -1

measure_start

for (( i = 2; i <= 5; ++i )); do
    expect_request_and_response "$i" 500
done

pt_expect_line 'pushing timeout' <&$pfd

expect_request_and_response 6 1000

expect_request_and_response 7 500

fakellama_kill

pt_close_fd "$pfd"
pt_testcase_end
