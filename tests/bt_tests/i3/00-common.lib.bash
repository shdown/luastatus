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

x_testcase() {
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
    x_spawn_luastatus
    bt_expect_line '{"version":1,"click_events":true,"stop_signal":0,"cont_signal":0}' <&${BT_SPAWNED_THINGS_FDS_0[luastatus]}
    bt_expect_line '[' <&${BT_SPAWNED_THINGS_FDS_0[luastatus]}
    if [[ -n "$expect_json" ]]; then
        bt_read_line <&${BT_SPAWNED_THINGS_FDS_0[luastatus]}
        if [[ $BT_LINE != *, ]]; then
            bt_fail "Line does not end with ','." "Line: '$BT_LINE'."
        fi
        x_assert_json_eq "$expect_json" "${BT_LINE%,}"
    fi
    bt_expect_line '[{"full_text":"(Error)","color":"#ff0000","background":"#000000"}],' <&${BT_SPAWNED_THINGS_FDS_0[luastatus]}
    bt_testcase_end
}
