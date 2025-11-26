main_fifo_file=./tmp-main-fifo

hang_timeout=${PT_HANG_TIMEOUT:-3}

mock_barlib="$PT_BUILD_DIR"/tests/barlib-mock.so
mock_plugin="$PT_BUILD_DIR"/tests/plugin-mock.so

assert_exits_with_code() {
    local expected_c=$1
    shift
    pt_spawn_luastatus_directly "$@"
    local actual_c=0
    pt_wait_luastatus || actual_c=$?
    if (( expected_c != actual_c )); then
        pt_fail "Expected exit code $expected_c, found $actual_c."
    fi
}

assert_succeeds() {
    assert_exits_with_code 0 "$@"
}

assert_fails() {
    assert_exits_with_code 1 "$@"
}

assert_hangs() {
    pt_spawn_luastatus_directly "$@"
    sleep "$hang_timeout"
    pt_kill_thing luastatus
}

assert_works() {
    assert_hangs "$@"
    assert_succeeds -e "$@"
}

testcase_assert_fails() {
    pt_testcase_begin
    assert_fails "$@"
    pt_testcase_end
}

testcase_assert_succeeds() {
    pt_testcase_begin
    assert_succeeds "$@"
    pt_testcase_end
}

testcase_assert_works() {
    pt_testcase_begin
    assert_works "$@"
    pt_testcase_end
}
