main_fifo_file=./tmp-fifo-main
socket_file=./pt-test-socket

rm -f "$socket_file" || fail "Cannot remove $socket_file."

wait_for_socket_to_appear() {
    echo >&2 "Waiting for socket $socket_file to appear..."
    while ! [[ -S $socket_file ]]; do
        true
    done
}

send_verbatim_to_socket() {
    printf '%s' "$1" | socat stdio "UNIX-CONNECT:$socket_file" \
        || fail "socat failed"
}

testcase_begin 'unixsock/simple'
add_fifo "$main_fifo_file"
add_file_to_remove "$socket_file"
write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
widget = {
    plugin = '$BUILD_DIR/plugins/unixsock/plugin-unixsock.so',
    opts = {
        path = '$socket_file',
    },
    cb = function(t)
        assert(t.what == 'line')
        f:write('line ' .. t.line .. '\n')
    end,
}
__EOF__
spawn_luastatus
exec 3<"$main_fifo_file"
expect_line 'init' <&3
wait_for_socket_to_appear
send_verbatim_to_socket $'one\n'
expect_line 'line one' <&3
send_verbatim_to_socket $'two\n'
expect_line 'line two' <&3
send_verbatim_to_socket 'without newline'
send_verbatim_to_socket $'with newline\n'
expect_line 'line with newline' <&3
send_verbatim_to_socket $'aypibmmwxrdxdknh\ngmstvxwxamouhlmw\ncybymucjtfxrauwn'
expect_line 'line aypibmmwxrdxdknh' <&3
exec 3<&-
testcase_end

testcase_begin 'unixsock/greet'
add_fifo "$main_fifo_file"
add_file_to_remove "$socket_file"
write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
widget = {
    plugin = '$BUILD_DIR/plugins/unixsock/plugin-unixsock.so',
    opts = {
        path = '$socket_file',
        greet = true,
    },
    cb = function(t)
        if t.what == 'line' then
            f:write('line ' .. t.line .. '\n')
        else
            f:write(t.what .. '\n')
        end
    end,
}
__EOF__
spawn_luastatus
exec 3<"$main_fifo_file"
expect_line 'init' <&3
wait_for_socket_to_appear
expect_line 'hello' <&3
send_verbatim_to_socket $'one\n'
expect_line 'line one' <&3
send_verbatim_to_socket $'two\n'
expect_line 'line two' <&3
send_verbatim_to_socket $'three\n'
expect_line 'line three' <&3
exec 3<&-
testcase_end

testcase_begin 'unixsock/do_not_try_unlink_failure'
true > "$socket_file" || fail "Cannot create regular file $socket_file."
add_file_to_remove "$socket_file"
write_widget_file <<__EOF__
widget = {
    plugin = '$BUILD_DIR/plugins/unixsock/plugin-unixsock.so',
    opts = {
        path = '$socket_file',
        try_unlink = false,
    },
    cb = function(t) end,
}
__EOF__
spawn_luastatus -e
wait_luastatus || fail "luastatus exited with non-zero code"
testcase_end

testcase_begin 'unixsock/do_try_unlink_success'
add_fifo "$main_fifo_file"
true > "$socket_file" || fail "Cannot create regular file $socket_file."
add_file_to_remove "$socket_file"
write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
widget = {
    plugin = '$BUILD_DIR/plugins/unixsock/plugin-unixsock.so',
    opts = {
        path = '$socket_file',
    },
    cb = function(t)
        assert(t.what == 'line')
        f:write('line ' .. t.line .. '\n')
    end,
}
__EOF__
spawn_luastatus
exec 3<"$main_fifo_file"
expect_line 'init' <&3
wait_for_socket_to_appear
send_verbatim_to_socket $'aloha\n'
expect_line 'line aloha' <&3
exec 3<&-
testcase_end

testcase_begin 'unixsock/timeout'
add_fifo "$main_fifo_file"
add_file_to_remove "$socket_file"
write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
widget = {
    plugin = '$BUILD_DIR/plugins/unixsock/plugin-unixsock.so',
    opts = {
        path = '$socket_file',
        greet = true,
        timeout = 0.25,
    },
    cb = function(t)
        if t.what == 'line' then
            f:write('line ' .. t.line .. '\n')
        else
            f:write(t.what .. '\n')
        end
    end,
}
__EOF__
spawn_luastatus
exec 3<"$main_fifo_file"
expect_line 'init' <&3
wait_for_socket_to_appear
expect_line 'hello' <&3
measure_start
expect_line 'timeout' <&3
measure_check_ms 250
expect_line 'timeout' <&3
measure_check_ms 250
send_verbatim_to_socket $'boo\n'
expect_line 'line boo' <&3
measure_check_ms 0
expect_line 'timeout' <&3
measure_check_ms 250
exec 3<&-
testcase_end

testcase_begin 'unixsock/lfuncs'
add_fifo "$main_fifo_file"
add_file_to_remove "$socket_file"
write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
n=0
widget = {
    plugin = '$BUILD_DIR/plugins/unixsock/plugin-unixsock.so',
    opts = {
        path = '$socket_file',
        greet = true,
        timeout = 0.1,
    },
    cb = function(t)
        if t.what == 'line' then
            f:write('line ' .. t.line .. '\n')
        else
            n=(n+1)%3
            f:write(t.what .. ' n=' .. n .. '\n')
            if n == 0 then
                luastatus.plugin.push_timeout(0.6)
            end
        end
    end,
}
__EOF__
spawn_luastatus
exec 3<"$main_fifo_file"
expect_line 'init' <&3
wait_for_socket_to_appear
expect_line 'hello n=1' <&3
measure_start
for (( i = 0; i < 4; ++i )); do
    send_verbatim_to_socket "foobar$i"$'\n'
    expect_line "line foobar$i" <&3
    measure_check_ms 0
    expect_line 'timeout n=2' <&3
    measure_check_ms 100
    expect_line 'timeout n=0' <&3
    measure_check_ms 100
    expect_line 'timeout n=1' <&3
    measure_check_ms 600
done
exec 3<&-
testcase_end
