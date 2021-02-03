pt_testcase_begin
pt_add_fifo "$main_fifo_file"
myfile=$(mktemp) || pt_fail "Cannot create temporary file."
pt_add_file_to_remove "$myfile"
pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
$preface
wd = nil
widget = {
    plugin = '$PT_BUILD_DIR/plugins/inotify/plugin-inotify.so',
    opts = {
        watch = {},
        greet = true,
        timeout = 0.25,
    },
    cb = function(t)
        local initial_wds = luastatus.plugin.get_initial_wds()
        assert(next(initial_wds) == nil, 'get_initial_wds() result is not empty')
        if t.what == 'hello' then
            wd = luastatus.plugin.add_watch('$myfile', {'close_write'})
            assert(wd, 'add_watch() failed')
            f:write('cb hello\n')
        elseif t.what == 'event' then
            assert(wd ~= nil, 'global "wd" is nil')
            local line = 'cb event mask=' .. _fmt_mask(t.mask)
            local is_ok = luastatus.plugin.remove_watch(wd)
            if is_ok then
                f:write(line .. ' (remove_watch: ok)\n')
            else
                f:write(line .. ' (remove_watch: error)\n')
            end
        else
            f:write('cb ' .. t.what .. '\n')
        end
    end,
}
__EOF__
pt_spawn_luastatus
exec {pfd}<"$main_fifo_file"
pt_expect_line 'init' <&$pfd
pt_expect_line 'cb hello' <&$pfd
pt_expect_line 'cb timeout' <&$pfd
echo hello >> "$myfile" || pt_fail "Cannot write to $myfile."
pt_expect_line 'cb event mask=close_write (remove_watch: ok)' <&$pfd
pt_expect_line 'cb event mask=ignored (remove_watch: error)' <&$pfd
echo hello >> "$myfile" || pt_fail "Cannot write to $myfile."
pt_expect_line 'cb timeout' <&$pfd
echo hello >> "$myfile" || pt_fail "Cannot write to $myfile."
pt_expect_line 'cb timeout' <&$pfd
pt_close_fd "$pfd"
pt_testcase_end
