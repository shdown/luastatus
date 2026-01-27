pt_require_tools mktemp

main_fifo_file=./tmp-fifo-main

is_program_running_testcase() {
    local opts=${1?}; shift
    local callback=${1?}; shift

    pt_testcase_begin
    pt_add_fifo "$main_fifo_file"

    pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
function my_event_func() end
x = dofile('$PT_SOURCE_DIR/plugins/is-program-running/is-program-running.lua')

function format_result_entry(x)
    return string.format('%s=>%s', x[1], tostring(x[2]))
end

function format_result(t)
    local chunks = {}
    local arr = {}

    if type(next(t)) == 'number' then
        for i, v in ipairs(t) do
            table.insert(arr, {i, v})
        end
        table.sort(arr, function(PQ_x, PQ_y)
            return PQ_x[1] < PQ_y[1]
        end)
    else
        for k, v in pairs(t) do
            table.insert(arr, {k, v})
        end
    end

    for _, x in ipairs(arr) do
        table.insert(chunks, format_result_entry(x))
    end
    return table.concat(chunks, ' ')
end

widget = x.widget{
    timer_opts = {
        period = 0.1,
    },
    cb = function(t)
        if type(t) == 'table' then
            f:write('cb ' .. format_result(t) .. '\n')
        else
            f:write('cb ' .. tostring(t) .. '\n')
        end
    end,
    event = my_event_func,
    $opts
}
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

x_do_nothing() {
    true
}
