-- ======================================================
-- ================= sensor_list stuff  =================
-- ======================================================

local function sensor_list_new(kind)
    return {kind = kind, entries = nil}
end

local function sensor_list_is_null(SL)
    return SL.entries == nil
end

local function sensor_list_nullify(SL)
    SL.entries = nil
end

local function sensor_list_realize(SL, filter_func, cmd)
    local f = assert(io.popen(cmd))

    local lines = {}
    for line in f:lines() do
        lines[#lines + 1] = line
    end

    f:close()

    local entries = {}
    for _, line in ipairs(lines) do
        local name, sort_by, path = line:match('^(%S+)\t(%S+)\t(%S+)$')
        if name then
            if (not filter_func) or filter_func(SL.kind, name) then
                entries[#entries + 1] = {name = name, sort_by = sort_by, path = path}
            end
        end
    end

    -- numeric sort
    local function extract_first_number(s)
        return tonumber(s:match('[0-9]+') or '-1')
    end
    table.sort(entries, function(a, b)
        return extract_first_number(a.sort_by) < extract_first_number(b.sort_by)
    end)

    SL.entries = entries
end

local function sensor_list_fetch_data(SL, into)
    assert(SL.entries)
    for _, entry in ipairs(SL.entries) do
        local f = io.open(entry.path, 'r')
        if not f then
            return false
        end
        local value = f:read('*number')
        f:close()
        if not value then
            return false
        end
        value = value / 1000
        into[#into + 1] = {
            kind  = SL.kind,
            name  = entry.name,
            path  = entry.path,
            value = value,
        }
    end
    return true
end

-- ======================================================
-- ================= data stuff  ========================
-- ======================================================

local function escape_or_return_default(custom, default)
    if not custom then
        return default
    end
    if custom:find('\0') then
        error('custom path contains NUL character')
    end
    return string.format([['%s']], custom:gsub([[']], [['\'']]))
end

local function data_realize_sensor_lists(data)
    sensor_list_nullify(data.SL_thermal)
    sensor_list_nullify(data.SL_hwmon)

    local thermal_path_escaped = escape_or_return_default(data._thermal_path, '/sys/class/thermal')
    local hwmon_path_escaped = escape_or_return_default(data._hwmon_path, '/sys/class/hwmon')

    sensor_list_realize(data.SL_thermal, data.filter_func, [[
D=]] .. thermal_path_escaped .. [[;
cd -- "$D" || exit $?
for dir in thermal_zone*; do
    [ -e "$dir"/temp ] || continue
    printf "%s\t%s\t%s\n" "$dir" "$dir" "$D/$dir/temp"
done
]])

    sensor_list_realize(data.SL_hwmon, data.filter_func, [[
D=]] .. hwmon_path_escaped .. [[;
cd -- "$D" || exit $?
for dir in *; do
    [ -e "$dir"/name ] || continue
    IFS= read -r monitor_name < "$dir"/name || continue
    for f in "$dir"/temp*_input; do
        [ -e "$f" ] || continue
        printf "%s\t%s\t%s\n" "$monitor_name" "${f##*/}" "$D/$f"
    done
done
]])
end

local function data_is_ready(data)
    if sensor_list_is_null(data.SL_thermal) then
        return false
    end
    if sensor_list_is_null(data.SL_hwmon) then
        return false
    end
    return true
end

local function data_fetch_temps(data)
    local r = {}
    if not sensor_list_fetch_data(data.SL_thermal, r) then
        return nil
    end
    if not sensor_list_fetch_data(data.SL_hwmon, r) then
        return nil
    end
    return r
end

-- ======================================================
-- ================= plugin interface ===================
-- ======================================================

local P = {}

function P.get_temps(data)
    if not data.SL_thermal then
        data.SL_thermal = sensor_list_new('thermal')
        data.SL_hwmon = sensor_list_new('hwmon')
    end

    if data.please_reload or (not data_is_ready(data)) then
        data_realize_sensor_lists(data)
        data.please_reload = nil
    end

    local r = data_fetch_temps(data)
    if not r then
        data_realize_sensor_lists(data)
        return nil
    end
    return r
end

function P.widget(tbl, data)
    data = data or {}

    if tbl.filter_func then
        data.filter_func = tbl.filter_func
    end

    return {
        plugin = 'timer',
        opts = tbl.timer_opts,
        cb = function()
            return tbl.cb(P.get_temps(data))
        end,
        event = tbl.event,
    }
end

return P
