main_fifo_file=./tmp-fifo-main
proc_dir=$(mktemp -d) || fail "Cannot create temporary directory."
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
    local testcase_name=$1
    local cpu_option=$2
    local expect_str=$3

    testcase_begin "cpu-usage-linux/$testcase_name"
    add_fifo "$main_fifo_file"
    printf '%s' "$stat_content_1" > "$stat_file" || fail "Cannot write to $stat_file."
    add_file_to_remove "$stat_file"
    write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
function my_event_func() end
x = dofile('$SOURCE_DIR/plugins/cpu-usage-linux/cpu-usage-linux.lua')
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
widget.plugin = ('$BUILD_DIR/plugins/{}/plugin-{}.so'):gsub('{}', widget.plugin)
assert(widget.event == my_event_func)
__EOF__
    spawn_luastatus
    exec 3<"$main_fifo_file"
    expect_line 'init' <&3
    expect_line 'cb nil' <&3
    printf '%s' "$stat_content_2" > "$stat_file" || fail "Cannot write to $stat_file."
    expect_line "$expect_str" <&3
    exec 3<&-
    testcase_end
}

cpu_usage_testcase 'fourcpus_total' 'nil' 'cb 0.1'
cpu_usage_testcase 'fourcpus_1' '1' 'cb 0.1'
cpu_usage_testcase 'fourcpus_2' '2' 'cb 0.1'
cpu_usage_testcase 'fourcpus_3' '3' 'cb 0.2'
cpu_usage_testcase 'fourcpus_4' '4' 'cb 0.2'

rmdir "$proc_dir" || fail "Cannot rmdir $proc_dir."
