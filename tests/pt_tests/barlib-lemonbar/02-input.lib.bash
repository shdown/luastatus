x_testcase_input() {
    pt_testcase_begin
    pt_add_fifo "$main_fifo_file"
    pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
widget = {
    plugin = '$PT_BUILD_DIR/tests/plugin-mock.so',
    opts = {make_calls = 0},
    cb = function()
    end,
    event = function(t)
        f:write('event ' .. t .. '\n')
    end,
}
__EOF__
    x_spawn_luastatus
    exec {pfd}<"$main_fifo_file"
    printf '%s\n' "$1" >&${PT_SPAWNED_THINGS_FDS_1[luastatus]}
    pt_expect_line "event $2" <&$pfd
    pt_close_fd "$pfd"
    pt_testcase_end
}

x_testcase_input '0_foo bar' 'foo bar'

x_testcase_input '0wtf_is_this
1_no_such_widget
1234567_no_such_widget
0__some_thing' '_some_thing'

x_testcase_input '-0_what
0_okay' 'okay'

x_testcase_input '0_' ''

x_testcase_input '
0_previous line is empty' 'previous line is empty'
