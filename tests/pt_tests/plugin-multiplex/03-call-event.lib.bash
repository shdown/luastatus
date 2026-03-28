pt_testcase_begin

pt_add_fifo "$main_fifo_file"
pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
widget = {
    plugin = '$PT_BUILD_DIR/plugins/multiplex/plugin-multiplex.so',
    opts = {
        data_sources = {
            src1 = [[
                alt_text = ''
                i = 0
                widget = {
                    plugin = '$PT_BUILD_DIR/plugins/timer/plugin-timer.so',
                    cb = function(t)
                        if #alt_text ~= 0 then
                            return alt_text
                        end
                        i = i + 1
                        return string.format("%s %d", t, i)
                    end,
                    event = function(s)
                        alt_text = string.format('event %s', s)
                    end,
                }
            ]],
        },
    },
    cb = function(t)
        assert(t.what == 'update')
        local text = t.updates.src1
        f:write('cb ' .. text .. '\n')
        if text == 'timeout 3' then
            assert(luastatus.plugin.call_event('src1', '[some data]'))
        end
    end,
}
__EOF__
pt_spawn_luastatus
exec {pfd}<"$main_fifo_file"
pt_expect_line 'init' <&$pfd
pt_expect_line 'cb hello 1' <&$pfd
pt_expect_line 'cb timeout 2' <&$pfd
pt_expect_line 'cb timeout 3' <&$pfd
pt_expect_line 'cb event [some data]' <&$pfd
pt_close_fd "$pfd"
pt_testcase_end
