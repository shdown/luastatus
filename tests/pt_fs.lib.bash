main_fifo_file=./tmp-fifo-main
wakeup_fifo_file=./tmp-fifo-wakeup
globtest_dir=$(mktemp -d) || fail "Cannot create temporary directory."

preface='
local function _validate_t(t, ks)
    local function _validate_num(n, msg)
        assert(type(n) == "number", msg)
        assert(n >= 0, msg)
        assert(n ~= math.huge, msg)
    end
    for _, k in ipairs(ks) do
        local function _msg(fmt)
            return string.format(fmt, k)
        end
        local x = t[k]
        assert(x, _msg("no entry for key `%s`"))
        _validate_num(x.total, _msg("entry for key `%s`: total is invalid"))
        _validate_num(x.free, _msg("entry for key `%s`: free is invalid"))
        _validate_num(x.avail, _msg("entry for key `%s`: avail is invalid"))
        assert(x.free <= x.total, _msg("entry for key `%s`: free > total"))
        assert(x.avail <= x.total, _msg("entry for key `%s`: avail > total"))
        t[k] = nil
    end
    local k = next(t)
    if k then
        error(string.format("unexpected entry with key `%s`", k))
    end
end
'

testcase_begin 'fs/default'
add_fifo "$main_fifo_file"
write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
$preface
widget = {
    plugin = '$BUILD_DIR/plugins/fs/plugin-fs.so',
    cb = function(t)
        _validate_t(t, {})
        f:write('cb\n')
    end,
}
__EOF__
spawn_luastatus
exec 3<"$main_fifo_file"
expect_line 'init' <&3
measure_start
expect_line 'cb' <&3
measure_check_ms 0
exec 3<&-
testcase_end

testcase_begin 'fs/root'
add_fifo "$main_fifo_file"
write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
$preface
widget = {
    plugin = '$BUILD_DIR/plugins/fs/plugin-fs.so',
    opts = {
        paths = {'/'},
        period = 0.25,
    },
    cb = function(t)
        _validate_t(t, {'/'})
        f:write('cb ok\n')
    end,
}
__EOF__
spawn_luastatus
exec 3<"$main_fifo_file"
measure_start
expect_line 'init' <&3
measure_check_ms 0
expect_line 'cb ok' <&3
measure_check_ms 0
expect_line 'cb ok' <&3
measure_check_ms 250
expect_line 'cb ok' <&3
measure_check_ms 250
exec 3<&-
testcase_end

testcase_begin 'fs/glob_yesmatch'
add_fifo "$main_fifo_file"
for f in havoc alligator za all cucumber; do
    touch "$globtest_dir/$f" || fail "Cannot touch $globtest_dir/$f."
    add_file_to_remove "$globtest_dir/$f"
done
write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
$preface
widget = {
    plugin = '$BUILD_DIR/plugins/fs/plugin-fs.so',
    opts = {
        globs = {'$globtest_dir/*?a*'},
    },
    cb = function(t)
        _validate_t(t, {'$globtest_dir/havoc', '$globtest_dir/alligator', '$globtest_dir/za'})
        f:write('cb glob seems to work...\n')
    end,
}
__EOF__
spawn_luastatus
exec 3<"$main_fifo_file"
measure_start
expect_line 'init' <&3
expect_line 'cb glob seems to work...' <&3
exec 3<&-
testcase_end

testcase_begin 'fs/glob_nomatch'
add_fifo "$main_fifo_file"
write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
$preface
widget = {
    plugin = '$BUILD_DIR/plugins/fs/plugin-fs.so',
    opts = {
        globs = {'$globtest_dir/foo/bar/*', '$globtest_dir/*z*', '$globtest_dir/*'},
    },
    cb = function(t)
        _validate_t(t, {})
        f:write('fine\n')
    end,
}
__EOF__
spawn_luastatus
exec 3<"$main_fifo_file"
measure_start
expect_line 'init' <&3
expect_line 'fine' <&3
exec 3<&-
testcase_end

testcase_begin 'fs/bad_path'
add_fifo "$main_fifo_file"
write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
$preface
widget = {
    plugin = '$BUILD_DIR/plugins/fs/plugin-fs.so',
    opts = {
        paths = {'$globtest_dir/foo/bar'},
    },
    cb = function(t)
        _validate_t(t, {})
        f:write('good\n')
    end,
}
__EOF__
spawn_luastatus
exec 3<"$main_fifo_file"
measure_start
expect_line 'init' <&3
expect_line 'good' <&3
exec 3<&-
testcase_end

testcase_begin 'fs/wakeup_fifo'
add_fifo "$main_fifo_file"
add_fifo "$wakeup_fifo_file"
write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
$preface
widget = {
    plugin = '$BUILD_DIR/plugins/fs/plugin-fs.so',
    opts = {
        paths = {'/'},
        period = 0.5,
        fifo = '$wakeup_fifo_file',
    },
    cb = function(t)
        _validate_t(t, {'/'})
        f:write('cb called\n')
    end,
}
__EOF__
spawn_luastatus
exec 3<"$main_fifo_file"
measure_start
expect_line 'init' <&3
measure_check_ms 0
expect_line 'cb called' <&3
measure_check_ms 0
expect_line 'cb called' <&3
measure_check_ms 500
expect_line 'cb called' <&3
measure_check_ms 500
touch "$wakeup_fifo_file" || fail "Cannot touch FIFO $wakeup_fifo_file."
expect_line 'cb called' <&3
measure_check_ms 0
expect_line 'cb called' <&3
measure_check_ms 500
exec 3<&-
testcase_end

rmdir "$globtest_dir" || fail "Cannot rmdir $globtest_dir."
