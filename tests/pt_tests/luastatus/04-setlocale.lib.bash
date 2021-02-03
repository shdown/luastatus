pt_testcase_begin
pt_write_widget_file <<__EOF__
assert(os.setlocale() == nil)
assert(os.setlocale('C') == nil)
os.exit(0)
__EOF__
assert_exits_with_code 0 -b "$mock_barlib"
pt_testcase_end
