x_testcase_output() {
    local lua_expr=$1
    local expect_line=$2
    local opts=()
    if [[ -v O_SEPARATOR ]]; then
        opts+=(-B separator="$O_SEPARATOR")
    fi

    pt_testcase_begin
    pt_write_widget_file <<__EOF__
local _my_flag = false
widget = {
    plugin = '$PT_BUILD_DIR/tests/plugin-mock.so',
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
    x_spawn_luastatus "${opts[@]}"
    if [[ -n "$expect_line" ]]; then
        pt_expect_line "$expect_line" <&${PT_SPAWNED_THINGS_FDS_0[luastatus]}
    fi
    pt_expect_line '%{B#f00}%{F#fff}(Error)%{B-}%{F-}' <&${PT_SPAWNED_THINGS_FDS_0[luastatus]}
    pt_testcase_end
}

x_testcase_output 'nil' ''
x_testcase_output '""' ''
x_testcase_output '"foo bar"' 'foo bar'
x_testcase_output '"?%%"' '?%%'
x_testcase_output '"abc%{A:mycommand:}def"' 'abc%{A:0_mycommand:}def'
x_testcase_output '"abc%%{A:mycommand:}def"' 'abc%%{A:mycommand:}def'
x_testcase_output '{"foo", "bar", "baz", "quiz"}' 'foo | bar | baz | quiz'
O_SEPARATOR='tum' x_testcase_output '{"foo", "bar", "baz", "quiz"}' 'footumbartumbaztumquiz'
x_testcase_output '{}' ''
x_testcase_output '{nil}' ''
x_testcase_output '{"", "", nil, ""}' ''
