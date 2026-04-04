pt_testcase_begin

httpserv_spawn GET /

pt_add_fifo "$main_fifo_file"
pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')

local function strip(header)
    local res, _ = header:gsub('[\r\n]', '')
    return res
end

widget = {
    plugin = '$PT_BUILD_DIR/plugins/web/plugin-web.so',
    opts = {
        planner = function()
            while true do
                coroutine.yield({
                    action = 'request',
                    params = {
                        url = 'http://127.0.0.1:$port/',
                        with_headers = true,
                    },
                })
                coroutine.yield({action = 'sleep', period = 1.0})
            end
        end,
    },
    cb = function(t)
        assert(t.what == 'response')
        print('Total # of headers:', #t.headers)
        for _, header in ipairs(t.headers) do
            print('Header:', strip(header))
            if header:lower() == 'content-type: text/plain\r\n' then
                f:write('ok\n')
                return
            end
        end
        f:write('fail\n')
    end,
}
__EOF__
pt_spawn_luastatus
exec {pfd}<"$main_fifo_file"
pt_expect_line 'init' <&$pfd

httpserv_expect '>'
httpserv_say 'RESPONSE'
pt_expect_line 'ok' <&$pfd

pt_close_fd "$pfd"
pt_testcase_end
