pt_testcase_begin
pt_add_fifo "$main_fifo_file"

pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')

local function seq_gen(f, n)
    local res = {}
    for i = 1, n do
        res[i] = f()
    end
    return res
end

local function seq_eq(xs, ys)
    if #xs ~= #ys then
        return false
    end
    for i = 1, #xs do
        if xs[i] ~= ys[i] then
            return false
        end
    end
    return true
end

function test_seed(f)
    -- p(fail) < 3.1e-24

    local N = 4

    math.randomseed(2)
    local xs = seq_gen(f, N)
    math.randomseed(3)
    local ys = seq_gen(f, N)
    math.randomseed(4)
    local zs = seq_gen(f, N)

    assert(not seq_eq(xs, ys))
    assert(not seq_eq(ys, zs))
    assert(not seq_eq(zs, xs))
end

test_seed(function() return math.random(123, 1000000) end)

f:write('ok\n')
__EOF__

pt_spawn_luastatus_directly -b "$mock_barlib"

exec {pfd}<"$main_fifo_file"
pt_expect_line 'ok' <&$pfd
pt_close_fd "$pfd"

pt_testcase_end
