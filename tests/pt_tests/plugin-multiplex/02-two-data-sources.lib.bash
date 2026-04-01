pt_testcase_begin

pt_add_fifo "$main_fifo_file"
pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')

function make_data_source(num, period)
    local prefix1 = string.format("MY_NUM = %d\n", num)
    local prefix2 = string.format("MY_PERIOD = %.3f\n", period)
    return prefix1 .. prefix2 .. [[
        i = 0
        widget = {
            plugin = '$PT_BUILD_DIR/plugins/timer/plugin-timer.so',
            opts = {
                period = MY_PERIOD,
            },
            cb = function(t)
                if t == 'hello' then
                    return nil
                end
                i = i + 1
                return string.format("[src%d] %s %d", MY_NUM, t, i)
            end,
        }
    ]]
end

widget = {
    plugin = '$PT_BUILD_DIR/plugins/multiplex/plugin-multiplex.so',
    opts = {
        data_sources = {
            src1 = make_data_source(1, 3.0),
            src2 = make_data_source(2, 7.5),
        },
    },
    cb = function(t)
        assert(t.what == 'update')
        local text = t.updates.src1 or t.updates.src2
        if type(text) == 'string' then
            f:write('cb ' .. text .. '\n')
        end
    end,
}
__EOF__
pt_spawn_luastatus
exec {pfd}<"$main_fifo_file"
pt_expect_line 'init' <&$pfd

pt_expect_line "cb [src1] timeout 1" <&$pfd
pt_expect_line "cb [src1] timeout 2" <&$pfd
pt_expect_line "cb [src2] timeout 1" <&$pfd
pt_expect_line "cb [src1] timeout 3" <&$pfd

pt_close_fd "$pfd"
pt_testcase_end
