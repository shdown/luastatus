paths = {}
do
    -- Replace "*" with "[^0]*" in the first glob if your zeroeth thermal sensor is virtual (and
    -- thus useless):
    local f = assert(io.popen([[
for file in /sys/class/thermal/thermal_zone*/temp
do
    [ -e "$file" ] || break
    printf "%s\n" "$file"
done
for dir in /sys/class/hwmon/*
do
    [ -e "$dir" ] || break
    IFS= read -r monitor_name < "$dir"/name
    # You may have more than one hardware monitor enabled
    # If so, disable ones that are not needed
    case "$monitor_name" in
    coretemp|fam15h_power|k10temp)
        printf "%s\n" "$dir"/temp*_input
    esac
done
]]))
    for p in f:lines() do
        table.insert(paths, p)
    end
    f:close()

    -- numeric sort
    local function extract_first_number(s)
        return tonumber(s:match('[0-9]+') or '-1')
    end
    table.sort(paths, function(a, b)
        return extract_first_number(a) < extract_first_number(b)
    end)
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
