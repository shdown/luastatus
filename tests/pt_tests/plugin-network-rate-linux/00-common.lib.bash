pt_require_tools mktemp

main_fifo_file=./tmp-fifo-main

netdev_content_1="\
Inter-|   Receive                                                |  Transmit
 face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed
    lo: 1563691862  518094    0    0    0     0          0         0 1563691862  518094    0    0    0     0       0          0
enp3s0:       0       0    0    0    0     0          0         0        0       0    0    0    0     0       0          0
 wlan0: 14599861312 19393562    0  706    0     0          0         0 14123982636 19982115    0   23    0     0       0          0
  tun0:    1482      19    0    0    0     0          0         0   251702    1316    0    0    0     0       0          0
"

netdev_content_2="\
Inter-|   Receive                                                |  Transmit
 face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed
    lo: 1563691862  518094    0    0    0     0          0         0 1563691862  518094    0    0    0     0       0          0
enp3s0:       0       0    0    0    0     0          0         0        0       0    0    0    0     0       0          0
 wlan0: 14601405091 19395437    0  706    0     0          0         0 14125578402 19984370    0   23    0     0       0          0
  tun0:    1482      19    0    0    0     0          0         0   251702    1316    0    0    0     0       0          0
"

res_wlan0_div1='R:1543779.0,S:1595766.0'
res_wlan0_div2='R:771889.5,S:797883.0'

network_rate_linux_testcase() {
    local extra_lua_opts=$1
    local expect_str=$2

    pt_testcase_begin
    pt_add_fifo "$main_fifo_file"

    local proc_dir; proc_dir=$(mktemp -d) || pt_fail "'mktemp -d' failed"
    pt_check mkdir "$proc_dir"/net
    pt_add_dirs_to_remove_inorder "$proc_dir"/net "$proc_dir"

    local netdev_file=$proc_dir/net/dev

    printf '%s' "$netdev_content_1" > "$netdev_file" || pt_fail 'cannot write netdev_file'
    pt_add_file_to_remove "$netdev_file"
    pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
function my_event_func() end
x = dofile('$PT_SOURCE_DIR/plugins/network-rate-linux/network-rate-linux.lua')

function stringify_results(tbl)
    if type(next(tbl)) ~= "number" then
        local new_tbl = {}
        for P, Q in pairs(tbl) do
            table.insert(new_tbl, {P, Q})
        end
        table.sort(new_tbl, function(PQ_a, PQ_b) return PQ_a[1] < PQ_b[1] end)
        tbl = new_tbl
    end
    local chunks = {}
    for _, PQ in ipairs(tbl) do
        table.insert(chunks, string.format("%s=>{R:%s,S:%s}", PQ[1], PQ[2].R, PQ[2].S))
    end
    return string.format("[%s]", table.concat(chunks, ","))
end

widget = x.widget{
    _procpath = '$proc_dir',
    cb = function(t)
        f:write('cb ' .. stringify_results(t) .. '\n')
    end,
    event = my_event_func,
    $extra_lua_opts
}
widget.plugin = ('$PT_BUILD_DIR/plugins/{}/plugin-{}.so'):gsub('{}', widget.plugin)
assert(widget.event == my_event_func)
__EOF__
    pt_spawn_luastatus
    exec {pfd}<"$main_fifo_file"
    pt_expect_line 'init' <&$pfd
    pt_expect_line 'cb []' <&$pfd
    printf '%s' "$netdev_content_2" > "$netdev_file"
    pt_expect_line "$expect_str" <&$pfd
    pt_close_fd "$pfd"
    pt_testcase_end
}
