pt_testcase_begin

fakeimap_spawn

pt_add_fifo "$main_fifo_file"
pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
x = dofile('$PT_SOURCE_DIR/plugins/imap/imap.lua')
widget = x.widget{
    host = 'localhost',
    port = $port,
    login = 'mylogin',
    password = 'mypassword',
    mailbox = 'Inbox',
    error_sleep_period = 1,
    cb = function(n)
        if n == nil then
            f:write('cb nil\n')
        else
            f:write(string.format('cb %d\n', n))
        end
    end,
    event = my_event_func,
}
widget.plugin = ('$PT_BUILD_DIR/plugins/{}/plugin-{}.so'):gsub('{}', widget.plugin)
assert(widget.event == my_event_func)
__EOF__
pt_spawn_luastatus
exec {pfd}<"$main_fifo_file"
pt_expect_line 'init' <&$pfd
pt_expect_line 'cb nil' <&$pfd

imap_interact_begin
imap_interact_middle_simple "$pfd"

fakeimap_kill
pt_expect_line 'cb nil' <&$pfd

pt_testcase_end
