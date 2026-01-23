pt_require_tools mktemp

pt_testcase_begin
pt_add_fifo "$main_fifo_file"
globtest_dir=$(mktemp -d) || pt_fail "'mktemp -d' failed"
pt_add_dir_to_remove "$globtest_dir"
for f in 1_one 2_two 3_three; do
    pt_check touch "$globtest_dir/$f"
    pt_add_file_to_remove "$globtest_dir/$f"
done
pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')
f:write('init\n')
$preface

local i = 1
local function check_eq(s1, s2)
    if s1 ~= s2 then
        error(string.format('check_eq: "%s" ~= "%s"', s1, s2))
    end
end
local function add(k, expected_result)
    local path = '$globtest_dir/' .. k
    local pcall_ok, result = pcall(luastatus.plugin.add_dyn_path, path)
    if pcall_ok then
        check_eq(result and 'true' or 'false', expected_result)
    else
        check_eq(expected_result, 'error')
    end
end
local function remove(k, expected_result)
    local path = '$globtest_dir/' .. k
    local result = luastatus.plugin.remove_dyn_path(path)
    check_eq(result and 'true' or 'false', expected_result)
end
local function check_keys(keys)

    if i == 1 then
        check_eq(keys, '')
        add('1_one', 'true')
        add('1_one', 'false')
        return true
    end

    if i == 2 then
        check_eq(keys, '1_one')
        add('2_two', 'true')
        add('2_two', 'false')
        return true
    end
    if i == 3 then
        check_eq(keys, '1_one 2_two')
        add('3_three', 'true')
        add('3_three', 'false')
        return true
    end
    if i == 4 then
        check_eq(keys, '1_one 2_two 3_three')
        remove('1_one', 'true')
        remove('1_one', 'false')
        return true
    end
    if i == 5 then
        check_eq(keys, '2_two 3_three')
        remove('2_two', 'true')
        remove('2_two', 'false')
        return true
    end
    if i == 6 then
        check_eq(keys, '3_three')
        remove('3_three', 'true')
        remove('3_three', 'false')
        return true
    end
    if i == 7 then
        check_eq(keys, '')
        return true
    end
    if i == 8 then
        local max = luastatus.plugin.get_max_dyn_paths()

        for j = 1, max do
            add(string.format('junk_%d', j), 'true')
        end

        add('more_junk', 'error')

        for j = 1, max do
            remove(string.format('junk_%d', j), 'true')
        end

        return true
    end

    return false
end
local function basename(s)
    return assert(s:match('[^/]+$'))
end

widget = {
    plugin = '$PT_BUILD_DIR/plugins/fs/plugin-fs.so',
    opts = {
        enable_dyn_paths = true,
        period = 0.1,
    },
    cb = function(t)
        local keys_tbl = {}
        for k, _ in pairs(t) do
            table.insert(keys_tbl, basename(k))
        end
        table.sort(keys_tbl)
        local keys = table.concat(keys_tbl, ' ')
        if not check_keys(keys) then
            return
        end
        f:write(string.format('ok %d\n', i))
        i = i + 1
    end,
}
__EOF__
pt_spawn_luastatus
exec {pfd}<"$main_fifo_file"
pt_expect_line 'init' <&$pfd

for (( i = 1; i <= 8; ++i )); do
    pt_expect_line "ok $i" <&$pfd
done

pt_close_fd "$pfd"
pt_testcase_end
