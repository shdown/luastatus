pt_require_tools mktemp

pt_testcase_begin
pt_add_fifo "$main_fifo_file"

myfile=$(mktemp)
pt_add_file_to_remove "$myfile"

pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
function my_event_func() end
x = dofile('$PT_SOURCE_DIR/plugins/file-contents-linux/file-contents-linux.lua')
widget = x.widget{
    filename = '$myfile',
    cb = function(myfile)
        local s = assert(myfile:read('*all'))
        f:write('update ' .. s:gsub('\n', ';') .. '\n')
    end,
    event = my_event_func,
}
widget.plugin = ('$PT_BUILD_DIR/plugins/{}/plugin-{}.so'):gsub('{}', widget.plugin)
assert(widget.event == my_event_func)
__EOF__

pt_spawn_luastatus
exec {pfd}<"$main_fifo_file"
pt_expect_line 'init' <&$pfd
pt_expect_line 'update ' <&$pfd

echo hello > "$myfile"
pt_read_line <&$pfd
pt_expect_line 'update hello;' <&$pfd

echo bye >> "$myfile"
pt_read_line <&$pfd
pt_expect_line 'update hello;bye;' <&$pfd

echo -n nonl > "$myfile"
pt_read_line <&$pfd
pt_expect_line 'update nonl' <&$pfd

pt_close_fd "$pfd"
pt_testcase_end
