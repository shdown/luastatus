stage_dir=$(mktemp -d) || pt_fail "Cannot create temporary directory."
stage_subdir=$stage_dir/subdir
actor_file_1=$stage_dir/foo1
actor_file_2=$stage_dir/foo2

pt_testcase_begin
pt_add_fifo "$main_fifo_file"
pt_add_file_to_remove "$actor_file_1"
pt_add_file_to_remove "$actor_file_2"
pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
$preface
_cookies = {}
_cookies_cnt = 0
function _fmt_cookie(c)
    assert(type(c) == 'number', 'cookie is not a number')
    if c == 0 then
        return '(none)'
    end
    local r = _cookies[c]
    if r == nil then
        _cookies_cnt = _cookies_cnt + 1
        r = string.format('(cookie #%d)', _cookies_cnt)
        _cookies[c] = r
    end
    return r
end
widget = {
    plugin = '$PT_BUILD_DIR/plugins/inotify/plugin-inotify.so',
    opts = {
        watch = {['$stage_dir'] = {'close_write', 'delete', 'moved_from', 'moved_to'}},
        greet = true,
    },
    cb = function(t)
        if t.what == 'hello' then
            f:write('cb hello\n')
        else
            assert(t.what == 'event', 'unexpected t.what')
            f:write(string.format('cb event mask=%s cookie=%s name=%s\n', _fmt_mask(t.mask), _fmt_cookie(t.cookie), t.name or '(nil)'))
        end
    end,
}
__EOF__
pt_spawn_luastatus
exec 3<"$main_fifo_file"

pt_expect_line 'init' <&3
pt_expect_line 'cb hello' <&3

echo hello >> "$actor_file_1" || pt_fail "Cannot write to $actor_file_1."
pt_expect_line 'cb event mask=close_write cookie=(none) name=foo1' <&3

mv -f "$actor_file_1" "$actor_file_2" || pt_fail "Cannot rename $actor_file_1 to $actor_file_2."
pt_expect_line 'cb event mask=moved_from cookie=(cookie #1) name=foo1' <&3
pt_expect_line 'cb event mask=moved_to cookie=(cookie #1) name=foo2' <&3

rm -f "$actor_file_2" || pt_fail "Cannot rm $actor_file_2."
pt_expect_line 'cb event mask=delete cookie=(none) name=foo2' <&3

mkdir "$stage_subdir" || pt_fail "Cannot mkdir $stage_subdir."
rmdir "$stage_subdir" || pt_fail "Cannot rmdir $stage_subdir."
pt_expect_line 'cb event mask=delete,isdir cookie=(none) name=subdir' <&3

rmdir "$stage_dir" || pt_fail "Cannot rmdir $stage_dir."
pt_expect_line 'cb event mask=ignored cookie=(none) name=(nil)' <&3

exec 3<&-
pt_testcase_end
