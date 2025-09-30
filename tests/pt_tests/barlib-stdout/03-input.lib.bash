x_testcase_input() {
    pt_testcase_begin
    pt_add_fifo "$main_fifo_file"
    pt_add_fifo "$in_fifo_file"
    pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
widget = {
    plugin = '$PT_BUILD_DIR/tests/plugin-mock.so',
    opts = {make_call = 0},
    cb = function()
    end,
    event = function(t)
        f:write('event ' .. t .. '\n')
    end,
}
__EOF__
    x_spawn_luastatus -B in_filename="$in_fifo_file"
    exec {pfd}<"$main_fifo_file"
    exec {pfd_in}>"$in_fifo_file"

    local line
    for line in "$@"; do
        sleep 0.002
        printf '%s\n' "$line" >&${pfd_in} || pt_fail 'printf failed'
        pt_expect_line "event $line" <&$pfd || pt_fail 'pt_expect_line failed'
    done

    pt_close_fd "$pfd"
    pt_testcase_end

    pt_close_fd "$pfd_in"
}

suffix='lorem-ipsum'

x_testcase_input

x_testcase_input foo$suffix

x_testcase_input foo$suffix bar$suffix

x_testcase_input foo$suffix bar$suffix baz$suffix
