pt_testcase_begin
pt_add_fifo "$main_fifo_file"

pt_write_widget_file <<__EOF__
f = assert(io.open('$main_fifo_file', 'w'))
f:setvbuf('line')

function test_fp_gen()
    local seen = {}
    local num_uniq = 0

    for i = 1, 64*1024 do
        local x = math.random()
        assert(x >= 0 and x < 1)
        if not seen[x] then
            seen[x] = true
            num_uniq = num_uniq + 1
        end
    end

    assert(num_uniq > 8*1024)
end
test_fp_gen()

function test_int_gen_uniq(lbound, rbound, f)
    local seen = {}
    local num_uniq = 0

    for i = 1, 64*1024 do
        local x = f()
        assert(x >= lbound and x <= rbound)
        if not seen[x] then
            seen[x] = true
            num_uniq = num_uniq + 1
        end
    end

    assert(num_uniq >= 10)
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

local function seq_shift(xs, new_v)
    local n = #xs
    for i = 1, n - 1 do
        xs[i] = xs[i + 1]
    end
    xs[n] = new_v
end

function test_int_gen_period(lbound, rbound, f)
    local N = 3

    local xs = {}
    for i = 1, N do
        xs[i] = f()
    end

    local ys = {}
    for i = 1, N do
        ys[i] = f()
    end

    for i = 1, 1024 do
        assert(not seq_eq(xs, ys))
        seq_shift(ys, f())
    end
end

function test_int_gen_quality(lbound, rbound, f)
    local N = 64*1024

    local first_third = lbound + math.floor((rbound - lbound + 1) / 3)

    local q = 0
    for i = 1, N do
        if f() < first_third then
            q = q + 1
        end
    end

    assert(q >= N / 4)
    assert(q <= N / 2)
end

function test_int_gen(lbound, rbound, f)
    test_int_gen_uniq(lbound, rbound, f)
    test_int_gen_period(lbound, rbound, f)
    test_int_gen_quality(lbound, rbound, f)
end

test_int_gen(1, 12345, function() return math.random(12345) end)

test_int_gen(123, 12345, function() return math.random(123, 12345) end)

test_int_gen(123, 12345, function()
    local r = math.random()
    return 123 + math.floor(r * (12345 - 123 + 1))
end)

f:write('ok\n')
__EOF__

pt_spawn_luastatus_directly -b "$mock_barlib"

exec {pfd}<"$main_fifo_file"
pt_expect_line 'ok' <&$pfd
pt_close_fd "$pfd"

pt_testcase_end
