pt_testcase_begin
pt_add_fifo "$main_fifo_file"
myfile=$(mktemp) || pt_fail "Cannot create temporary file."
pt_add_file_to_remove "$myfile"
pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
$preface
widget = {
    plugin = '$PT_BUILD_DIR/plugins/inotify/plugin-inotify.so',
    opts = {
        watch = {['$myfile'] = {'close_write', 'delete_self'}},
    },
    cb = function(t)
        assert(t.what == 'event', 'unexpected t.what')
        f:write('cb event mask=' .. _fmt_mask(t.mask) .. '\n')
    end,
}
__EOF__
pt_spawn_luastatus
exec 3<"$main_fifo_file"
pt_expect_line 'init' <&3

# Try to avoid race condition: we may modify the file before the watch is set up.
sleep 1 || pt_fail 'Cannot sleep 1.'

echo hello >> "$myfile" || pt_fail "Cannot write to $myfile."
pt_expect_line 'cb event mask=close_write' <&3
rm -f "$myfile" || pt_fail "Cannot remove $myfile."
pt_expect_line 'cb event mask=delete_self' <&3
pt_expect_line 'cb event mask=ignored' <&3
exec 3<&-
pt_testcase_end
