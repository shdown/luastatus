pt_testcase_begin
pt_add_fifo "$main_fifo_file"
pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
$preface
widget = {
    plugin = '$PT_BUILD_DIR/plugins/fs/plugin-fs.so',
    opts = {
        paths = {'$globtest_dir/foo/bar'},
    },
    cb = function(t)
        _validate_t(t, {})
        f:write('good\n')
    end,
}
__EOF__
pt_spawn_luastatus
exec 3<"$main_fifo_file"
measure_start
pt_expect_line 'init' <&3
pt_expect_line 'good' <&3
exec 3<&-
pt_testcase_end
