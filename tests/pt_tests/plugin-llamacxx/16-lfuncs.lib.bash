pt_testcase_begin

pt_add_fifo "$main_fifo_file"
pt_add_fifo "$R_fifo_file"

pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')

TESTCASES_1Q = {
    '',
    'test',
    'with double"quotes',
    'with back\`quote',
    "the so-called 'single quotes' are here",
    "I borrowed my dad's ring",
    "'",
    "4'441''453114324032'2352154310",
}
TESTCASES_2Q = {
    '',
    'Another test',
    "with single'quote",
    'with back\`quote',
    'the so-called "double quotes" are here',
    'abc"def',
    '"',
    '"2420"340214243130103031"1"35"',
}
TESTCASES_JSON = {
    '',
    'normal stuff',
    '"double-quoted stuff"',
    'stuff with \\\\backslash',
    '/?/stuff\\\\with"all_kinds\x01of\x1fsymbols^',
}
TESTCASE_JSON_BLOW_UP = 'a string with \\0 character'

widget = {
    plugin = '$PT_BUILD_DIR/plugins/llamacxx/plugin-llamacxx.so',
    opts = {
        greet = true,
        connection = {
            port = 0,
        },
        prompt = function()
            return 'What is the purpose of life?'
        end,
    },
    cb = function(t)
        if t.what ~= 'hello' then
            return
        end
        for _, s in ipairs(TESTCASES_1Q) do
            f:write('1q:' .. luastatus.plugin.escape_single_quoted(s) .. '\n')
        end
        for _, s in ipairs(TESTCASES_2Q) do
            f:write('2q:' .. luastatus.plugin.escape_double_quoted(s) .. '\n')
        end
        for _, s in ipairs(TESTCASES_JSON) do
            f:write('json:' .. luastatus.plugin.json_escape(s) .. '\n')
        end

        local returned_normally = pcall(function()
            return luastatus.plugin.json_escape(TESTCASE_JSON_BLOW_UP)
        end)
        assert(not returned_normally)
        f:write('json error ok\n')

        f:write('end\n')
    end,
}
__EOF__
pt_spawn_luastatus

exec {pfd}<"$main_fifo_file"

pt_expect_line 'init' <&$pfd

pt_expect_line '1q:' <&$pfd
pt_expect_line '1q:test' <&$pfd
pt_expect_line '1q:with double"quotes' <&$pfd
pt_expect_line '1q:with back`quote' <&$pfd
pt_expect_line '1q:the so-called `single quotes` are here' <&$pfd
pt_expect_line '1q:I borrowed my dad`s ring' <&$pfd
pt_expect_line '1q:`' <&$pfd
pt_expect_line '1q:4`441``453114324032`2352154310' <&$pfd

pt_expect_line '2q:' <&$pfd
pt_expect_line '2q:Another test' <&$pfd
pt_expect_line "2q:with single'quote" <&$pfd
pt_expect_line '2q:with back`quote' <&$pfd
pt_expect_line "2q:the so-called ''double quotes'' are here" <&$pfd
pt_expect_line "2q:abc''def" <&$pfd
pt_expect_line "2q:''" <&$pfd
pt_expect_line "2q:''2420''340214243130103031''1''35''" <&$pfd

pt_expect_line 'json:' <&$pfd
pt_expect_line 'json:normal stuff' <&$pfd
pt_expect_line 'json:\u0022double-quoted stuff\u0022' <&$pfd
pt_expect_line 'json:stuff with \u005cbackslash' <&$pfd
pt_expect_line 'json:\u002f?\u002fstuff\u005cwith\u0022all_kinds\u0001of\u001fsymbols^' <&$pfd

pt_expect_line 'json error ok' <&$pfd

pt_expect_line 'end' <&$pfd

pt_close_fd "$pfd"
pt_testcase_end
