pt_testcase_begin
using_measure

pt_add_fifo "$main_fifo_file"

pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')

function call_access(path)
    local ok, err = luastatus.plugin.access(path)
    if ok then
        f:write('exists = true\n')
    else
        if err then
            f:write('error ' .. err .. '\n')
        else
            f:write('exists = false\n')
        end
    end
end

widget = {
    plugin = '$PT_BUILD_DIR/plugins/inotify/plugin-inotify.so',
    opts = {
        watch = {},
        timeout = 1,
        greet = true,
    },
    cb = function(t)
        call_access('/')
        call_access('/k4dsqflkr8ntrpwagmw7')
    end,
}
__EOF__
pt_spawn_luastatus
exec {pfd}<"$main_fifo_file"

pt_expect_line 'init' <&$pfd
pt_expect_line 'exists = true' <&$pfd
pt_expect_line 'exists = false' <&$pfd

pt_close_fd "$pfd"
pt_testcase_end
