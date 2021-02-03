pt_testcase_begin
pt_write_widget_file <<__EOF__
os.exit(27)
__EOF__
assert_exits_with_code 27 -b "$mock_barlib"
pt_testcase_end

pt_testcase_begin
pt_write_widget_file <<__EOF__
os.exit()
__EOF__
assert_exits_with_code 0 -b "$mock_barlib"
pt_testcase_end
