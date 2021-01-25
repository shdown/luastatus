main_fifo_file=./tmp-fifo-main

testcase_begin 'file-contents-linux/simple'
add_fifo "$main_fifo_file"
myfile=$(mktemp) || fail "mktemp failed."
add_file_to_remove "$myfile"
write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
x = dofile('$SOURCE_DIR/plugins/file-contents-linux/file-contents-linux.lua')
widget = x.widget{
    filename = '$myfile',
    cb = function(myfile)
        local s = assert(myfile:read('*all'))
        f:write('update ' .. s:gsub('\n', ';') .. '\n')
    end,
}
widget.plugin = ('$BUILD_DIR/plugins/{}/plugin-{}.so'):gsub('{}', widget.plugin)
__EOF__
spawn_luastatus
exec 3<"$main_fifo_file"
expect_line 'init' <&3
expect_line 'update ' <&3

echo hello > "$myfile" || fail "Cannot write to $myfile."
read_line <&3
expect_line 'update hello;' <&3

echo bye >> "$myfile" || fail "Cannot write to $myfile."
read_line <&3
expect_line 'update hello;bye;' <&3

echo -n nonl > "$myfile" || fail "Cannot write to $myfile."
read_line <&3
expect_line 'update nonl' <&3

exec 3<&-
testcase_end
