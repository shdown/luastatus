pt_require_tools mktemp

main_fifo_file=./tmp-fifo-main

battery_testcase() {
    local expect_str=$1
    local uevent_content=$2 # empty string here means remove the file

    pt_testcase_begin
    pt_add_fifo "$main_fifo_file"
    local batdev_dir; batdev_dir=$(mktemp -d) || pt_fail "'mktemp -d' failed"
    pt_add_dir_to_remove "$batdev_dir"
    local uevent_file=$batdev_dir/uevent
    if [[ -n "$uevent_content" ]]; then
        printf '%s' "$uevent_content" > "$uevent_file" || pt_fail "cannot write uevent_file"
    else
        rm -f "$uevent_file"
    fi
    pt_add_file_to_remove "$uevent_file"
    pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
function my_event_func() end
x = dofile('$PT_SOURCE_DIR/plugins/battery-linux/battery-linux.lua')
widget = x.widget{
    _devpath = '$batdev_dir',
    cb = function(t)
        f:write('cb')
        if t.status then
            f:write(string.format(' status="%s"', t.status))
        end
        if t.capacity then
            f:write(string.format(' capacity=%.0f', t.capacity))
        end
        if t.consumption then
            f:write(string.format(' consumption=%.1f', t.consumption))
        end
        if t.rem_time then
            f:write(string.format(' rem_time=%.1f', t.rem_time))
        end
        f:write('\n')
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
