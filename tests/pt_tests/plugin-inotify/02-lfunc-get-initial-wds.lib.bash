pt_require_tools mktemp

pt_testcase_begin
pt_add_fifo "$main_fifo_file"
myfile1=$(mktemp)
myfile2=$(mktemp)
pt_add_file_to_remove "$myfile1"
pt_add_file_to_remove "$myfile2"
pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
$preface
widget = {
    plugin = '$PT_BUILD_DIR/plugins/inotify/plugin-inotify.so',
    opts = {
        watch = {
            ['$myfile1'] = {'close_write', 'delete_self'},
            ['$myfile2'] = {'delete_self'},
        },
        greet = true,
    },
    cb = function(t)
        if t.what == 'hello' then
            f:write('cb hello\n')
        else
            assert(t.what == 'event', 'unexpected t.what')
            assert(type(t.wd) == 'number', 't.wd is not a number')
            local file_ids = {
                ['$myfile1'] = '1',
                ['$myfile2'] = '2',
            }
            local wds = luastatus.plugin.get_initial_wds()
            local file_id
            for k, v in pairs(wds) do
                local id = file_ids[k]
                assert(id ~= nil, string.format('unexpected key in initial wds: "%s"', k))
                if v == t.wd then
                    file_id = id
                end
            end
            assert(file_id ~= nil, 't.wd is not present in initial wds')
            f:write('cb event file' .. file_id .. ' mask=' .. _fmt_mask(t.mask) .. '\n')
        end
    end,
}
__EOF__
pt_spawn_luastatus
exec {pfd}<"$main_fifo_file"
pt_expect_line 'init' <&$pfd
pt_expect_line 'cb hello' <&$pfd
echo hello >> "$myfile2"
echo hello >> "$myfile1"
pt_expect_line 'cb event file1 mask=close_write' <&$pfd
rm -f "$myfile2"
pt_expect_line 'cb event file2 mask=delete_self' <&$pfd
pt_expect_line 'cb event file2 mask=ignored' <&$pfd
rm -f "$myfile1"
pt_expect_line 'cb event file1 mask=delete_self' <&$pfd
pt_expect_line 'cb event file1 mask=ignored' <&$pfd
pt_close_fd "$pfd"
pt_testcase_end
