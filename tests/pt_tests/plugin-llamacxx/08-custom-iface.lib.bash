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
            custom_iface = '192.0.2.0',
            req_timeout = 1.0,
        },
        prompt = function()
            return 'What is the purpose of life?'
        end,
    },
    cb = function(t)
        if t.what == 'error' then
            f:write('cb error')
            f:write(' meta=' .. t.meta)
            f:write('\n')
        else
            f:write('cb ' .. t.what .. '\n')
        end
    end,
}
__EOF__
pt_spawn_luastatus

fakellama_spawn_and_wait_until_ready fxd:'{"content":"something"}' 0

exec {pfd}<"$main_fifo_file"

pt_expect_line 'init' <&$pfd

pt_read_line <&$pfd

prefix='cb error meta=C'
if [[ $PT_LINE != "$prefix"?* ]]; then
    printf '%s\n' >&2 "Got line:"
    printf '%s\n' >&2 "    '$PT_LINE'"
    printf '%s\n' >&2 "Expected: line with the following prefix:"
    printf '%s\n' >&2 "    '$prefix'"
    pt_fail 'Read unexpected line'
fi

fakellama_kill

pt_close_fd "$pfd"
pt_testcase_end
