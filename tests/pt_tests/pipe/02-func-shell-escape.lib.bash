pt_testcase_begin
pt_add_fifo "$main_fifo_file"
pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
x = dofile('$PT_SOURCE_DIR/plugins/pipe/pipe.lua')
local function dump(id, arg)
    local s = x.shell_escape(arg)
    assert(type(s) == 'string', 'shell_escape() return value is not string')
    f:write(string.format('%s => %s\n', id, s:gsub('\n', ';')))
end
dump('1', 'hello')
dump('2', 'hello world')
dump('3', 'ichi\t  nichi')
dump('4', 'one\ntwo')
dump('5', "it's fine")
dump('6', '')
dump('7', {})
dump('8', {'one'})
dump('9', {"one's own"})
dump('10', {'one', 'two', 'three'})
dump('11', {'one two', 'three'})
dump('12', {'one', 'two\n\tthree'})
__EOF__
pt_spawn_luastatus
exec {pfd}<"$main_fifo_file"
tab=$'\t'
pt_expect_line 'init' <&$pfd
pt_expect_line "1 => 'hello'" <&$pfd
pt_expect_line "2 => 'hello world'" <&$pfd
pt_expect_line "3 => 'ichi${tab}  nichi'" <&$pfd
pt_expect_line "4 => 'one;two'" <&$pfd
pt_expect_line "5 => 'it'\\''s fine'" <&$pfd
pt_expect_line "6 => ''" <&$pfd
pt_expect_line "7 => " <&$pfd
pt_expect_line "8 => 'one'" <&$pfd
pt_expect_line "9 => 'one'\\''s own'" <&$pfd
pt_expect_line "10 => 'one' 'two' 'three'" <&$pfd
pt_expect_line "11 => 'one two' 'three'" <&$pfd
pt_expect_line "12 => 'one' 'two;${tab}three'" <&$pfd
pt_close_fd "$pfd"
pt_testcase_end
