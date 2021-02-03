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

x_testcase_output 'nil' ''
x_testcase_output '{}' ''
x_testcase_output '{nil}' ''
x_testcase_output '{{}}' '[{"name":"0"}]'
x_testcase_output '{{},nil,{}}' '[{"name":"0"},{"name":"0"}]'
x_testcase_output '{full_text = "Hello, world"}' '[{"name":"0","full_text":"Hello, world"}]'
x_testcase_output '{{full_text = "Hello, world"}}' '[{"name":"0","full_text":"Hello, world"}]'
x_testcase_output '{{fpval = 1234.5}}' '[{"name":"0","fpval":1234.5}]'
x_testcase_output '{{intval = -12345}}' '[{"name":"0","intval":-12345}]'
x_testcase_output '{{name = "abc"}}' '[{"name":"0"}]'
x_testcase_output '{{name = 123}}' '[{"name":"0"}]'
x_testcase_output '{{separator = true}}' '[{"name":"0","separator":true}]'
x_testcase_output '{{separator = false}}' '[{"name":"0","separator":false}]'
x_testcase_output '{{foo = true, bar = false}}' '[{"name":"0","foo":true,"bar":false}]'

O_NO_SEPARATORS=1 x_testcase_output '{{}}' '[{"name":"0","separator":false}]'
O_NO_SEPARATORS=1 x_testcase_output '{{separator = true}}' '[{"name":"0","separator":true}]'
O_NO_SEPARATORS=1 x_testcase_output '{{separator = false}}' '[{"name":"0","separator":false}]'

O_NO_CLICK_EVENTS=1 x_testcase_output 'nil' ''

O_ALLOW_STOPPING=1 x_testcase_output 'nil' ''
