x_testcase_output() {
    local lua_expr=$1
    local expect_line=$2
    local expect_error_line='(Error)'
    local opts=()
    if [[ -n O_SEPARATOR ]]; then
        opts+=(-B separator="$O_SEPARATOR")
    fi
    if [[ -n O_ERROR ]]; then
        expect_error_line=$O_ERROR
        opts+=(-B error="$O_ERROR")
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
    pt_expect_line "$expect_error_line" <&${PT_SPAWNED_THINGS_FDS_0[luastatus]}
    pt_testcase_end
}

O_SEPARATOR=
O_ERROR=

x_testcase_output 'nil' ''
x_testcase_output '""' ''
x_testcase_output '"foo bar"' 'foo bar'
x_testcase_output '"?%%"' '?%%'
x_testcase_output '{"foo", "bar", "baz", "quiz"}' 'foo | bar | baz | quiz'
O_SEPARATOR='tum' \
    x_testcase_output '{"foo", "bar", "baz", "quiz"}' 'footumbartumbaztumquiz'
O_ERROR='dammit…' O_SEPARATOR='•' \
    x_testcase_output '{"", "str", "", "", "str2", ""}' 'str•str2'
O_ERROR=':(' \
    x_testcase_output '""' ''
x_testcase_output '{}' ''
x_testcase_output '{nil}' ''
x_testcase_output '{"", "", nil, ""}' ''
