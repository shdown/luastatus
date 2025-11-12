pt_testcase_begin
pt_add_fifo "$main_fifo_file"

pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
if rawlen == nil then
    -- Lua 5.1
    assert(os.execute() ~= 0)
    assert(type(os.execute('exit 42')) == 'number')
    f:write('ok\n')
else
    -- Lua >=5.2
    assert(os.execute() == true)
    local is_ok, what, code

    is_ok, what, code = os.execute('exit 42')
    assert(is_ok == nil)
    assert(what == 'exit')
    assert(code == 42)

    is_ok, what, code = os.execute('exit 0')
    assert(is_ok == true)
    assert(what == 'exit')
    assert(code == 0)

    is_ok, what, code = os.execute('kill -9 \$\$')
    assert(is_ok == nil)
    assert(what == 'signal')
    assert(code == 9)

    f:write('ok\n')
end
__EOF__

pt_spawn_luastatus_directly -b "$mock_barlib"

exec {pfd}<"$main_fifo_file"
pt_expect_line 'ok' <&$pfd
pt_close_fd "$pfd"

pt_testcase_end
