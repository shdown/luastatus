testcase_assert_fails
testcase_assert_fails /dev/null
testcase_assert_fails /dev/nil
testcase_assert_fails -e
testcase_assert_fails -TheseFlagsDoNotExist
testcase_assert_fails -l ''
testcase_assert_fails -l nosuchloglevel
testcase_assert_fails -l '•÷¢˝Q⅓'
testcase_assert_fails -l
testcase_assert_fails -l -l
testcase_assert_fails -ы
testcase_assert_fails -v
testcase_assert_fails -b

testcase_assert_fails -b '§n”™°£'
testcase_assert_fails -b '/'
testcase_assert_fails -b 'nosuchbarlibforsure'

testcase_assert_succeeds -b "$mock_barlib" -eeeeeeee -e

testcase_assert_works -b "$mock_barlib"
testcase_assert_works -b "$mock_barlib" .
testcase_assert_works -b "$mock_barlib" . . . . .
testcase_assert_works -b "$mock_barlib" /dev/null
testcase_assert_works -b "$mock_barlib" /dev/null /dev/null /dev/null

pt_testcase_begin
pt_write_widget_file <<__EOF__
luastatus = nil
__EOF__
assert_works -b "$mock_barlib"
pt_testcase_end

pt_testcase_begin
pt_write_widget_file <<__EOF__
widget = setmetatable(
    {
        plugin = setmetatable({}, {
            __tostring = function() error("hi there (__tostring)") end,
        }),
        cb = function() end,
    }, {
        __index = function() error("hi there (__index)") end,
        __pairs = function() error("hi there (__pairs)") end,
    }
)
__EOF__
assert_works -b "$mock_barlib"
pt_testcase_end

pt_testcase_begin
pt_write_widget_file <<__EOF__
widget = {
    plugin = '$mock_plugin',
    cb = function()
    end,
}
__EOF__
assert_works -b "$mock_barlib"
pt_testcase_end
