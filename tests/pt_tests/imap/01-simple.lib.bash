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
exec 3<"$main_fifo_file"
pt_expect_line 'init' <&3
pt_expect_line 'cb nil' <&3

fakeimap_say "* OK IMAP4rev1 Service Ready"

fakeimap_cmd_expect "LOGIN mylogin mypassword"
fakeimap_cmd_done "OK LOGIN completed"

fakeimap_cmd_expect "SELECT Inbox"
fakeimap_say "* 18 EXISTS"
fakeimap_say "* OK [UNSEEN 17] Message 17 is the first unseen message"
fakeimap_cmd_done "OK [READ-WRITE] SELECT completed"

for (( i = 17; i < 20; ++i )); do
    fakeimap_cmd_expect "STATUS Inbox (UNSEEN)"
    fakeimap_say "* STATUS Inbox (UNSEEN $i)"
    fakeimap_cmd_done "OK [READ-WRITE] SELECT completed"

    fakeimap_cmd_expect "IDLE"
    pt_expect_line "cb $i" <&3

    fakeimap_say "* SOMETHING"
    fakeimap_expect "DONE"
    fakeimap_cmd_done "OK IDLE completed"
done

fakeimap_kill

pt_expect_line 'cb nil' <&3

pt_testcase_end
