main_fifo_file=./tmp-fifo-main

mem_usage_testcase() {
    local expect_str=$1
    local meminfo_content=$2

    pt_testcase_begin
    pt_add_fifo "$main_fifo_file"
    local proc_dir; proc_dir=$(mktemp -d) || pt_fail "Cannot create temporary directory."
    pt_add_dir_to_remove "$proc_dir"
    local meminfo_file=$proc_dir/meminfo
    printf '%s' "$meminfo_content" > "$meminfo_file" || pt_fail "Cannot write to $meminfo_file."
    pt_add_file_to_remove "$meminfo_file"
    pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
function my_event_func() end
x = dofile('$PT_SOURCE_DIR/plugins/mem-usage-linux/mem-usage-linux.lua')
widget = x.widget{
    _procpath = '$proc_dir',
    cb = function(t)
        f:write(string.format('cb avail=%s(%s) total=%s(%s)\n', t.avail.value, t.avail.unit, t.total.value, t.total.unit))
    end,
    event = my_event_func,
}
widget.plugin = ('$PT_BUILD_DIR/plugins/{}/plugin-{}.so'):gsub('{}', widget.plugin)
assert(widget.event == my_event_func)
__EOF__
    pt_spawn_luastatus
    exec {pfd}<"$main_fifo_file"
    pt_expect_line 'init' <&$pfd
    pt_expect_line "$expect_str" <&$pfd
    pt_close_fd "$pfd"
    pt_testcase_end
}
