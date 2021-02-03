x_testcase_input() {
    bt_testcase_begin
    bt_add_fifo "$main_fifo_file"
    bt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
local function _fmt_x(x)
    local function _validate_str(s)
        assert(s:find("\n") == nil, "string contains a line break")
        assert(s:find("\"") == nil, "string contains a double quote sign")
    end
    local function _fmt_num(n)
        if n == math.floor(n) then
            return string.format('%d', n)
        else
            return string.format('%.4f', n)
        end
    end
    local t = type(x)
    if t == "table" then
        local tk = type(next(x))
        if tk == "nil" then
            return "{}"
        elseif tk == "number" then
            local s = {}
            for _, v in ipairs(x) do
                s[#s + 1] = _fmt_x(v)
            end
            return string.format("{%s}", table.concat(s, ","))
        elseif tk == "string" then
            local ks = {}
            for k, _ in pairs(x) do
                assert(type(k) == "string", "table has mixed-type keys")
                _validate_str(k)
                ks[#ks + 1] = k
            end
            table.sort(ks)
            local s = {}
            for _, k in ipairs(ks) do
                local v = x[k]
                s[#s + 1] = string.format("[\"%s\"]=%s", k, _fmt_x(v))
            end
            return string.format("{%s}", table.concat(s, ","))
        else
            error("table key has unexpected type: " .. tk)
        end
    elseif t == "string" then
        _validate_str(x)
        return string.format("\"%s\"", x)
    elseif t == "number" then
        return _fmt_num(x)
    else
        return tostring(x)
    end
end
widget = {
    plugin = '$BT_BUILD_DIR/tests/plugin-mock.so',
    opts = {make_calls = 0},
    cb = function()
    end,
    event = function(t)
        f:write('event ' .. _fmt_x(t) .. '\n')
    end,
}
__EOF__
    x_spawn_luastatus
    local pfd
    exec {pfd}<"$main_fifo_file"
    bt_expect_line '{"version":1,"click_events":true,"stop_signal":0,"cont_signal":0}' <&${BT_SPAWNED_THINGS_FDS_0[luastatus]}
    bt_expect_line '[' <&${BT_SPAWNED_THINGS_FDS_0[luastatus]}
    printf '[\n%s\n' "$1" >&${BT_SPAWNED_THINGS_FDS_1[luastatus]} || bt_fail "Cannot communicate with luastatus."
    bt_expect_line "event $2" <&$pfd
    bt_close_fd "$pfd"
    bt_testcase_end
}

x_testcase_input '{"name":"0","foo":"bar"}' '{["foo"]="bar",["name"]="0"}'
x_testcase_input '{"name":"0","foo":[1,2,3,{"k":"v"},4]}' '{["foo"]={1,2,3,{["k"]="v"},4},["name"]="0"}'
x_testcase_input '{"name":"0","foo":null,"bar":true,"baz":false}' '{["bar"]=true,["baz"]=false,["name"]="0"}'
