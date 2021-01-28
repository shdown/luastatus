main_fifo_file=./tmp-fifo-main
proc_dir=$(mktemp -d) || pt_fail "Cannot create temporary directory."
stat_file=$proc_dir/stat

stat_content_1="\
cpu  32855780 19344 19217634 468500203 300653 0 273158 0 0 0
cpu0 8358858 4763 5675358 116643347 61382 0 4672 0 0 0
cpu1 8301511 4836 5853008 116508899 59664 0 25225 0 0 0
cpu2 7952134 4371 3472358 117560453 113665 0 221818 0 0 0
cpu3 8243275 5374 4216907 117787501 65941 0 21442 0 0 0
"

stat_content_2="\
cpu  32855819 19344 19217652 468500542 300653 0 273158 0 0 0
cpu0 8358868 4763 5675362 116643433 61382 0 4672 0 0 0
cpu1 8301519 4836 5853012 116508986 59664 0 25225 0 0 0
cpu2 7952146 4371 3472364 117560535 113665 0 221818 0 0 0
cpu3 8243285 5374 4216913 117787586 65941 0 21442 0 0 0
"

cpu_usage_testcase() {
    local cpu_option=$1
    local expect_str=$2

    pt_testcase_begin
    pt_add_fifo "$main_fifo_file"
    printf '%s' "$stat_content_1" > "$stat_file" || pt_fail "Cannot write to $stat_file."
    pt_add_file_to_remove "$stat_file"
    pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
function my_event_func() end
x = dofile('$PT_SOURCE_DIR/plugins/cpu-usage-linux/cpu-usage-linux.lua')
widget = x.widget{
    _procpath = '$proc_dir',
    cpu = $cpu_option,
    cb = function(t)
        if t then
            f:write(string.format('cb %.1f\n', t))
        else
            f:write('cb nil\n')
        end
    end,
    event = my_event_func,
}
widget.plugin = ('$PT_BUILD_DIR/plugins/{}/plugin-{}.so'):gsub('{}', widget.plugin)
assert(widget.event == my_event_func)
__EOF__
    pt_spawn_luastatus
    exec 3<"$main_fifo_file"
    pt_expect_line 'init' <&3
    pt_expect_line 'cb nil' <&3
    printf '%s' "$stat_content_2" > "$stat_file" || pt_fail "Cannot write to $stat_file."
    pt_expect_line "$expect_str" <&3
    exec 3<&-
    pt_testcase_end
}

cpu_usage_cleanup() {
    rmdir "$proc_dir" || pt_fail "Cannot rmdir $proc_dir."
}
pt_push_cleanup_after_suite cpu_usage_cleanup
