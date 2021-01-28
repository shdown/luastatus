main_fifo_file=./tmp-fifo-main
wakeup_fifo_file=./tmp-fifo-wakeup
globtest_dir=$(mktemp -d) || pt_fail "Cannot create temporary directory."

preface='
local function _validate_t(t, ks)
    local function _assert_for_k(cond, k)
        if not cond then
            error(string.format("assertion failed for entry with key <%s> (see the stacktrace)", k))
        end
    end
    local function _validate_num_for_k(n, k)
        _assert_for_k(type(n) == "number", k)
        _assert_for_k(n >= 0, k)
        _assert_for_k(n ~= math.huge, k)
    end
    for _, k in ipairs(ks) do
        local x = t[k]
        _assert_for_k(x ~= nil, k)
        _validate_num_for_k(x.total, k)
        _validate_num_for_k(x.free, k)
        _validate_num_for_k(x.avail, k)
        _assert_for_k(x.free <= x.total, k)
        _assert_for_k(x.avail <= x.total, k)
        t[k] = nil
    end
    local k = next(t)
    if k ~= nil then
        error(string.format("unexpected entry with key <%s>", k))
    end
end
'

fs_cleanup() {
    rmdir "$globtest_dir" || pt_fail "Cannot rmdir $globtest_dir."
}
pt_push_cleanup_after_suite fs_cleanup
