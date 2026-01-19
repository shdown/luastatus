pt_testcase_begin
pt_add_fifo "$main_fifo_file"
pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
local function _fmt_t(t)
    local s = {}
    for k, v in pairs(t) do
        for _, x in ipairs(v.ipv4 or {}) do
            s[#s + 1] = string.format('%s ipv4 %s', k, x)
        end
        for _, x in ipairs(v.ipv6 or {}) do
            s[#s + 1] = string.format('%s ipv6 %s', k, x)
        end
    end
    table.sort(s)
    return table.concat(s, ';') .. ';'
end
widget = {
    plugin = '$PT_BUILD_DIR/plugins/network-linux/plugin-network-linux.so',
    opts = {
        new_ip_fmt = true,
        new_overall_fmt = true,
    },
    cb = function(t)
        assert(type(t) == 'table', 't is not a table')
        assert(t.what, 't.what is not present')

        if t.what == 'error' then
            f:write('cb error\n')
        else
            f:write('cb ok ' .. _fmt_t(t.data) .. '\n')
        end
    end,
}
__EOF__
pt_spawn_luastatus
exec {pfd}<"$main_fifo_file"
pt_expect_line 'init' <&$pfd

listnets_binary=$PT_BUILD_DIR/tests/listnets
c=0; nets=$("$listnets_binary") || c=$?
case "$c" in
0)
    nets=$(printf '%s\n' "$nets" | LC_ALL=C sort | tr '\n' ';')
    expect_str="cb ok $nets"
    ;;
1)
    expect_str="cb error"
    ;;
*)
    pt_fail "listnets binary ('$listnets_binary') failed with code $c."
    ;;
esac

pt_expect_line "$expect_str" <&$pfd

pt_close_fd "$pfd"
pt_testcase_end
