paths = {}
do
    -- Replace "*" with "[^0]*" in the first glob if your zeroeth thermal sensor is virtual (and
    -- thus useless):
    local f = io.popen([[ sh -c '
shopt -s nullglob
for file in /sys/class/thermal/thermal_zone*/temp
do
    echo "$file"
done
for dir in /sys/class/hwmon/*
do
    monitor_name="$(< "$dir"/name)"
    # You may have more than one hardware monitor enabled
    # If so, disable ones that are not needed
    if [ $monitor_name = "coretemp" ] ||
       [ $monitor_name = "fam15h_power" ] ||
       [ $monitor_name = "k10temp" ]
    then
        echo "$dir"/temp*_input
    fi
done
']])
    for p in f:lines() do
        table.insert(paths, p)
    end
    f:close()
end

widget = {
    plugin = 'timer',
    opts = {period = 2},
    cb = function()
        local res = {}
        for _, p in ipairs(paths) do
            local f = assert(io.open(p, 'r'))
            local temp = f:read('*number') / 1000
            table.insert(res, string.format('%.0f°', temp))
            f:close()
        end
        return res
    end,
}
