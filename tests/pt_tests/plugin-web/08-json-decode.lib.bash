pt_testcase_begin
pt_add_fifo "$main_fifo_file"
pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')

local function check_dict_impl(json_str, expected, is_marked)
    local res, err = luastatus.plugin.json_decode(json_str, is_marked)
    assert(type(res) == 'table')

    if expected.foo ~= nil then
        local res_key = next(res)
        assert(res_key == 'foo')
        assert(next(res, res_key) == nil)
        assert(res.foo == expected.foo)
    else
        assert(next(res) == nil)
    end

    local mt = getmetatable(res)
    if is_marked then
        assert(mt ~= nil)
        assert(mt.is_dict)
        assert(not mt.is_array)
    else
        assert(mt == nil)
    end
end

local function check_dict(json_str, expected)
    check_dict_impl(json_str, expected, false)
    check_dict_impl(json_str, expected, true)
end

local function check_array_impl(json_str, expected, is_marked)
    local res, err = luastatus.plugin.json_decode(json_str, is_marked)
    assert(type(res) == 'table')

    assert(#res == #expected)
    for i = 1, #res do
        assert(res[i] == expected[i])
    end

    local mt = getmetatable(res)
    if is_marked then
        assert(mt ~= nil)
        assert(mt.is_array)
        assert(not mt.is_dict)
    else
        assert(mt == nil)
    end
end

local function check_array(json_str, expected)
    check_array_impl(json_str, expected, false)
    check_array_impl(json_str, expected, true)
end

widget = {
    plugin = '$PT_BUILD_DIR/plugins/web/plugin-web.so',
    opts = {
        planner = function()
            while true do
                coroutine.yield({action = 'call_cb', what = 'test'})
                coroutine.yield({action = 'sleep', period = 1.0})
            end
        end,
    },
    cb = function(t)
        assert(t.what == 'test')

        check_dict('{"foo":"bar"}', {foo = "bar"})
        check_dict('{"foo":42}', {foo = 42})
        check_dict('{"foo":true}', {foo = true})
        check_dict('{"foo":false}', {foo = false})
        check_dict('{"foo":null}', {})

        check_array('[]', {})
        check_array('["foo"]', {"foo"})
        check_array('["foo", "bar"]', {"foo", "bar"})

        f:write('ok\n')
    end,
}
__EOF__
pt_spawn_luastatus
exec {pfd}<"$main_fifo_file"
pt_expect_line 'init' <&$pfd

pt_expect_line 'ok' <&$pfd

pt_close_fd "$pfd"
pt_testcase_end
