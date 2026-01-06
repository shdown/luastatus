pt_testcase_begin
using_measure

pt_add_fifo "$main_fifo_file"
pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')

function check_eq(a, b)
    if a ~= b then
        print(a, '!=', b)
        error('check_eq() failed')
    end
end

function array_contains(t, v)
    for _, x in ipairs(t) do
        if x == v then
            return true
        end
    end
    return false
end

-- this function converts return list with multiple values into a _S_ingle value
function S(x)
    return x
end

widget = {
    plugin = '$PT_BUILD_DIR/plugins/timer/plugin-timer.so',
    opts = {
        period = 1.0,
    },
    cb = function(t)
        if t ~= 'hello' then
            return
        end

        check_eq(S(luastatus.plugin.access('.')), true)
        check_eq(S(luastatus.plugin.access('./gbdc06iiw1kZ4yccnnx9')), false)

        check_eq(S(luastatus.plugin.stat('.')), 'dir')
        check_eq(S(luastatus.plugin.stat('/proc/uptime')), 'regular')
        check_eq(S(luastatus.plugin.stat('./gbdc06iiw1kZ4yccnnx9')), nil)

        assert(array_contains(S(luastatus.plugin.glob('./*')), './pt.sh'))

        assert(luastatus.plugin.is_process_alive($$))
        assert(luastatus.plugin.is_process_alive("$$"))

        assert(luastatus.plugin.is_process_alive(1))
        assert(luastatus.plugin.is_process_alive("1"))

        while true do
            local random_pid = math.random(2, 1073741823)
            print('trying random_pid=' .. random_pid)
            if not luastatus.plugin.is_process_alive(random_pid) then
                break
            end
            print('it is alive, retrying')
        end

        f:write('ok\n')
    end,
}
__EOF__
pt_spawn_luastatus
exec {pfd}<"$main_fifo_file"
pt_expect_line 'init' <&$pfd

pt_expect_line 'ok' <&$pfd

pt_close_fd "$pfd"
pt_testcase_end
