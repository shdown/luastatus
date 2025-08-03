pt_require_tools mktemp

pt_testcase_begin
pt_add_fifo "$main_fifo_file"
myfile=$(mktemp)
pt_add_file_to_remove "$myfile"
pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
$preface
widget = {
    plugin = '$PT_BUILD_DIR/plugins/inotify/plugin-inotify.so',
    opts = {
        watch = {},
        greet = true,
    },
    cb = function(t)
        local line
        if t.what == 'hello' then
            line = 'cb hello'
        else
            assert(t.what == 'event')
            line = 'cb event mask=' .. _fmt_mask(t.mask)
        end
        local wd = luastatus.plugin.add_watch('$myfile', {'close_write', 'delete_self', 'oneshot'})
        if wd then
            f:write(line .. ' (add_watch: ok)\n')
        else
            f:write(line .. ' (add_watch: error)\n')
        end
    end,
}
__EOF__
pt_spawn_luastatus
exec {pfd}<"$main_fifo_file"
pt_expect_line 'init' <&$pfd
pt_expect_line 'cb hello (add_watch: ok)' <&$pfd
for (( i = 0; i < 5; ++i )); do
    echo hello >> "$myfile"
    pt_expect_line 'cb event mask=close_write (add_watch: ok)' <&$pfd
    pt_expect_line 'cb event mask=ignored (add_watch: ok)' <&$pfd
done
rm -f "$myfile"
pt_expect_line 'cb event mask=delete_self (add_watch: error)' <&$pfd
pt_expect_line 'cb event mask=ignored (add_watch: error)' <&$pfd
pt_close_fd "$pfd"
pt_testcase_end
