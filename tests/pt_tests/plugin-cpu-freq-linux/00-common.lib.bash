pt_require_tools mktemp

main_fifo_file=./tmp-fifo-main

cpu_dir=
x_testcase_begin() {
    cpu_dir=$(mktemp -d) || pt_fail 'mktemp -d failed'
}

x_add_or_set_cpu() {
    local i=${1?}; shift
    local max=${1?}; shift
    local min=${1?}; shift
    local cur=${1?}; shift

    if ! [[ -d "$cpu_dir"/cpu"$i" ]]; then

        mkdir "$cpu_dir"/cpu"$i" || return $?
        mkdir "$cpu_dir"/cpu"$i"/cpufreq || return $?

        pt_add_dirs_to_remove_inorder \
            "$cpu_dir"/cpu"$i"/cpufreq \
            "$cpu_dir"/cpu"$i" \
            || return $?

        pt_add_file_to_remove "$cpu_dir"/cpu"$i"/cpufreq/cpuinfo_max_freq || return $?
        pt_add_file_to_remove "$cpu_dir"/cpu"$i"/cpufreq/cpuinfo_min_freq || return $?
        pt_add_file_to_remove "$cpu_dir"/cpu"$i"/cpufreq/scaling_cur_freq || return $?
    fi

    echo "$max" > "$cpu_dir"/cpu"$i"/cpufreq/cpuinfo_max_freq || return $?
    echo "$min" > "$cpu_dir"/cpu"$i"/cpufreq/cpuinfo_min_freq || return $?
    echo "$cur" > "$cpu_dir"/cpu"$i"/cpufreq/scaling_cur_freq || return $?
}

x_testcase() {
    local reload_each_time=${1?}; shift
    local callback=${1?}; shift

    local reload_code=
    if (( reload_each_time )); then
        reload_code='data.please_reload = true;'
    fi

    pt_testcase_begin
    pt_add_fifo "$main_fifo_file"

    pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
function my_event_func() end
x = dofile('$PT_SOURCE_DIR/plugins/cpu-freq-linux/cpu-freq-linux.lua')

function format_result_entry(x)
    return string.format('max=%d,min=%d,cur=%d', x.max, x.min, x.cur)
end

function format_result(arr)
    local chunks = {}
    for _, x in ipairs(arr) do
        table.insert(chunks, format_result_entry(x))
    end
    return table.concat(chunks, ' ')
end

data = {_cpu_dir = '$cpu_dir'}
widget = x.widget({
    period = 0.1,
    cb = function(t)
        $reload_code
        if type(t) == 'table' then
            f:write('cb ' .. format_result(t) .. '\n')
        else
            assert(t == nil)
            f:write('cb nil\n')
        end
    end,
    event = my_event_func,
}, data)
widget.plugin = ('$PT_BUILD_DIR/plugins/{}/plugin-{}.so'):gsub('{}', widget.plugin)
assert(widget.event == my_event_func)
__EOF__
    pt_spawn_luastatus
    exec {pfd}<"$main_fifo_file"
    pt_expect_line 'init' <&$pfd

    local i
    for (( i = 0; i < $#; ++i )); do
        pt_check $callback $i
        local j=$(( i + 1 ))
        pt_expect_line "${!j}" <&$pfd
    done

    pt_close_fd "$pfd"
    pt_testcase_end
}

x_testcase_end() {
    pt_add_dir_to_remove "$cpu_dir"
    cpu_dir=
}
