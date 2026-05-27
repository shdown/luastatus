pt_testcase_begin
pt_write_widget_file <<__EOF__

function assert_throws(f)
    local is_ok, _ = pcall(f)
    assert(not is_ok)
end

assert_throws(function() os.setlocale()    end)
assert_throws(function() os.setlocale('C') end)

os.exit(0)
__EOF__
assert_exits_with_code 0 -b "$mock_barlib"
pt_testcase_end
