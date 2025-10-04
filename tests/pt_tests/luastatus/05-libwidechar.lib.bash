pt_testcase_begin
pt_add_fifo "$main_fifo_file"

pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')

function my_to_str(x)
    if type(x) == 'number' then
        return string.format('%d', x)
    end
    if type(x) == 'boolean' then
        return x and 'TRUE' or 'FALSE'
    end
    if x == nil then
        return 'nil'
    end
    return '<<' .. x .. '>>'
end

function dump(x)
    f:write(my_to_str(x) .. '\\n')
end

function dump2(x, y)
    f:write(string.format('%s %s\\n', my_to_str(x), my_to_str(y)))
end

WIDTH = luastatus.libwidechar.width
TRUNC = luastatus.libwidechar.truncate_to_width
MKVALID = luastatus.libwidechar.make_valid_and_printable

STR = "Test"
dump(WIDTH(STR))
dump2(TRUNC(STR, 3))
dump(MKVALID(STR, "?"))

STR = "$(printf \\xFF) Test"
dump(WIDTH(STR))
dump2(TRUNC(STR, 3))
dump(MKVALID(STR, "?"))

STR = "ЮЩЛЫ"
dump(WIDTH(STR))
dump2(TRUNC(STR, 3))
dump(MKVALID(STR, "?"))

STR = "t$(printf \\x01)est"
dump(WIDTH(STR))
dump2(TRUNC(STR, 3))
dump(MKVALID(STR, "?"))

STR = "ABCDEFGHI"

dump(WIDTH(STR, 2))
dump(WIDTH(STR, 2, 3))

dump(TRUNC(STR, 1, 2))
dump(TRUNC(STR, 1, 3, 4))
dump(TRUNC(STR, 1, 3, 128))
dump(TRUNC(STR, 100, 3, 8))
dump(TRUNC(STR, 100, 3, 9))
dump(TRUNC(STR, 100, 3, 10))
dump(TRUNC(STR, 100, 3, 11))

dump(MKVALID(STR, '?', 3))
dump(MKVALID(STR, '?', 3, 5))

dump(luastatus.libwidechar.is_dummy_implementation())

__EOF__

pt_spawn_luastatus_directly -b "$mock_barlib"

exec {pfd}<"$main_fifo_file"

pt_expect_line '4' <&$pfd
pt_expect_line '<<Tes>> 3' <&$pfd
pt_expect_line '<<Test>>' <&$pfd

pt_expect_line 'nil' <&$pfd
pt_expect_line 'nil nil' <&$pfd
pt_expect_line '<<? Test>>' <&$pfd

pt_expect_line '4' <&$pfd
pt_expect_line '<<ЮЩЛ>> 3' <&$pfd
pt_expect_line '<<ЮЩЛЫ>>' <&$pfd

pt_expect_line 'nil' <&$pfd
pt_expect_line 'nil nil' <&$pfd
pt_expect_line '<<t?est>>' <&$pfd

pt_expect_line '8' <&$pfd
pt_expect_line '2' <&$pfd

pt_expect_line '<<B>>' <&$pfd
pt_expect_line '<<C>>' <&$pfd
pt_expect_line '<<C>>' <&$pfd
pt_expect_line '<<CDEFGH>>' <&$pfd
pt_expect_line '<<CDEFGHI>>' <&$pfd
pt_expect_line '<<CDEFGHI>>' <&$pfd
pt_expect_line '<<CDEFGHI>>' <&$pfd

pt_expect_line '<<CDEFGHI>>' <&$pfd
pt_expect_line '<<CDE>>' <&$pfd

pt_expect_line 'FALSE' <&$pfd

pt_close_fd "$pfd"

pt_testcase_end
