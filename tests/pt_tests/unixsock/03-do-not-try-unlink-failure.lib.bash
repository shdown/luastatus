pt_testcase_begin
true > "$socket_file" || pt_fail "Cannot create regular file $socket_file."
pt_add_file_to_remove "$socket_file"
pt_write_widget_file <<__EOF__
widget = {
    plugin = '$PT_BUILD_DIR/plugins/unixsock/plugin-unixsock.so',
    opts = {
        path = '$socket_file',
        try_unlink = false,
    },
    cb = function(t) end,
}
__EOF__
pt_spawn_luastatus -e
pt_wait_luastatus || pt_fail "luastatus exited with non-zero code"
pt_testcase_end
