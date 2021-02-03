main_fifo_file=./tmp-fifo-main

x_spawn_luastatus() {
    bt_spawn_luastatus_via_runner "$BT_SOURCE_DIR"/tests/bt_runners/runner-redirect-34 \
        -b "$BT_BUILD_DIR"/barlibs/i3/barlib-i3.so -B in_fd=3 -B out_fd=4 \
        "$@"
}

x_assert_json_eq() {
    echo >&2 "Comparing JSONs:
Expected: $1
Actual:   $2"

    local out
    out=$(jq --argfile a <(printf '%s\n' "$1") --argfile b <(printf '%s\n' "$2") -n '$a == $b') \
        || bt_fail "jq failed"
    case "$out" in
    true)
        ;;
    false)
        bt_fail 'x_assert_json_eq: JSON does not match.' "Expected: $1" "Found: $2"
        ;;
    *)
        bt_fail "Unexpected output of jq: '$out'."
        ;;
    esac
}

x_testcase_output() {
    local lua_expr=$1
    local expect_json=$2

    bt_testcase_begin
    bt_write_widget_file <<__EOF__
local _my_flag = false
widget = {
    plugin = '$BT_BUILD_DIR/tests/plugin-mock.so',
    opts = {make_calls = 2},
    cb = function()
        if _my_flag then
            error('Boom')
        end
        _my_flag = true
        return ($lua_expr)
    end,
}
__EOF__

    local opts=()
    if (( O_NO_SEPARATORS )); then
        opts+=(-B no_separators)
    fi
    if (( O_NO_CLICK_EVENTS )); then
        opts+=(-B no_click_events)
    fi
    if (( O_ALLOW_STOPPING )); then
        opts+=(-B allow_stopping)
    fi

    local first_line='{"version":1'
    if (( O_NO_CLICK_EVENTS )); then
        first_line+=',"click_events":false'
    else
        first_line+=',"click_events":true'
    fi
    if (( ! O_ALLOW_STOPPING )); then
        first_line+=',"stop_signal":0,"cont_signal":0'
    fi
    first_line+='}'

    local last_line='[{"full_text":"(Error)","color":"#ff0000","background":"#000000"'
    if (( O_NO_SEPARATORS )); then
        last_line+=',"separator":false'
    fi
    last_line+='}],'

    x_spawn_luastatus "${opts[@]}"
    bt_expect_line "$first_line" <&${BT_SPAWNED_THINGS_FDS_0[luastatus]}
    bt_expect_line '[' <&${BT_SPAWNED_THINGS_FDS_0[luastatus]}
    if [[ -n "$expect_json" ]]; then
        bt_read_line <&${BT_SPAWNED_THINGS_FDS_0[luastatus]}
        if [[ $BT_LINE != *, ]]; then
            bt_fail "Line does not end with ','." "Line: '$BT_LINE'."
        fi
        x_assert_json_eq "$expect_json" "${BT_LINE%,}"
    fi
    bt_expect_line "$last_line" <&${BT_SPAWNED_THINGS_FDS_0[luastatus]}
    bt_testcase_end
}

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
