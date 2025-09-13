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
        report_mu = true,
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
                        return string.format('i=%d', i)
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
            f:write(string.format('cb answer mu=%s answer=%s\n', t.mu, t.answer))
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

fakellama_spawn_and_wait_until_ready \
    fxd:'{"content":"ANOTHER STRING"}' \
    1 \
    --freeze-for=1000

exec {pfd}<"$main_fifo_file"

pt_expect_line 'init' <&$pfd

while true; do
    # Read request line and skip it
    fakellama_get_req

    pt_read_line <&$pfd

    case "$PT_LINE" in
    'cb answer mu=false answer=ANOTHER STRING')
        echo >&2 "Got mu=false, waiting further"
        ;;
    'cb answer mu=true answer=ANOTHER STRING')
        echo >&2 "Got mu=true, done"
        break
        ;;
    *)
        pt_fail "Got unexpected line: '$PT_LINE'"
        ;;
    esac
done

fakellama_kill

pt_close_fd "$pfd"
pt_testcase_end
