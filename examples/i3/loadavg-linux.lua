local function ncpus_try_cmdline(cmd)
    local f = assert(io.popen(cmd .. ' 2>/dev/null', 'r'))
    local n = f:read('*number')
    f:close()
    return n
end

ncpus = nil
if not ncpus then
    ncpus = ncpus_try_cmdline('getconf _NPROCESSORS_ONLN')
end
if not ncpus then
    ncpus = ncpus_try_cmdline('getconf NPROCESSORS_ONLN')
end
if not ncpus then
    ncpus = ncpus_try_cmdline('nproc --all')
end
if not ncpus then
    ncpus = ncpus_try_cmdline('LC_ALL=C grep -c "^processor\\s" /proc/cpuinfo')
end
assert(ncpus, 'Cannot fetch number of CPUs')

local function avg2str(x)
    assert(x >= 0)
    if x >= 1000 then
        return '↑↑↑'
    end
    return string.format('%3.0f', x)
end

widget = {
    plugin = 'timer',
    opts = {
        greet = true,
    },
    cb = function(t)
        local f = io.open('/proc/loadavg', 'r')
        local avg1, avg5, avg15 = f:read('*number', '*number', '*number')
        f:close()

        return {full_text = string.format(
            '[%s%% %s%% %s%%]',
            avg2str(avg1  / ncpus * 100),
            avg2str(avg5  / ncpus * 100),
            avg2str(avg15 / ncpus * 100)
        )}
    end,
}
