pt_require_tools mktemp

main_fifo_file=./tmp-fifo-main

disk_io_linux_testcase() {
    local expect_str_1=$1
    local expect_str_2=$2
    local content_1=$3
    local content_2=$4

    pt_testcase_begin
    pt_add_fifo "$main_fifo_file"
    local proc_dir; proc_dir=$(mktemp -d) || pt_fail "'mktemp -d' failed"
    pt_add_dir_to_remove "$proc_dir"
    local diskstats_file=$proc_dir/diskstats

    pt_add_file_to_remove "$diskstats_file"
    pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
function my_event_func() end
x = dofile('$PT_SOURCE_DIR/plugins/disk-io-linux/disk-io-linux.lua')
widget = x.widget{
    _proc_path = '$proc_dir',
    cb = function(t)
        f:write('cb')

        table.sort(t, function(a, b)
            return a.name < b.name
        end)

        for _, x in ipairs(t) do
            f:write(string.format(
                " (%d,%d name=>%s r=>%d w=>%d)",
                x.num_major,
                x.num_minor,
                x.name,
                x.read_bytes,
                x.written_bytes
            ))
        end

        if #t == 0 then
            f:write(' <none>')
        end

        f:write('\n')
    end,
    event = my_event_func,
}
widget.plugin = ('$PT_BUILD_DIR/plugins/{}/plugin-{}.so'):gsub('{}', widget.plugin)
assert(widget.event == my_event_func)
__EOF__

    printf '%s' "$content_1" > "$diskstats_file" || pt_fail "cannot write diskstats_file"

    pt_spawn_luastatus
    exec {pfd}<"$main_fifo_file"
    pt_expect_line 'init' <&$pfd

    pt_expect_line "$expect_str_1" <&$pfd

    printf '%s' "$content_2" > "$diskstats_file" || pt_fail "cannot write diskstats_file"

    pt_expect_line "$expect_str_2" <&$pfd

    pt_close_fd "$pfd"
    pt_testcase_end
}
