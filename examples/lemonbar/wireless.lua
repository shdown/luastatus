local ORIGIN = '•'  -- '●' '○' '◌' '◍' '⬤'
local RAY = '}'     -- '›' '〉' '❭' '❯' '❱' '⟩' '⟫' '》'
local MIN_DBM, MAX_DBM = -90, -20
local NGAUGE = 5
local COLOR_DIM = '#709080'

local function round(x)
    return math.floor(x + 0.5)
end

local function colorize_as_dim(s)
    s = luastatus.barlib.escape(s)
    return '%{F' .. COLOR_DIM .. '}' .. s .. '%{F-}'
end

local function make_wifi_gauge(dbm)
    if dbm < MIN_DBM then dbm = MIN_DBM end
    if dbm > MAX_DBM then dbm = MAX_DBM end
    local nbright = round(NGAUGE * (1 - 0.7 * (MAX_DBM - dbm) / (MAX_DBM - MIN_DBM)))
    return ORIGIN .. RAY:rep(nbright) .. colorize_as_dim(RAY:rep(NGAUGE - nbright))
end

local function sorted_keys(tbl)
    local keys = {}
    for k, _ in pairs(tbl) do
        keys[#keys + 1] = k
    end
    table.sort(keys)
    return keys
end

widget = {
    plugin = 'network-linux',
    opts = {
        wireless = true,
        timeout = 10,
    },
    cb = function(t)
        if not t then
            return nil
        end

        -- Sort for determinism
        local ifaces = sorted_keys(t)

        local r = {}
        for _, iface in ipairs(ifaces) do
            local params = t[iface]
            if params.wireless then
                if params.wireless.ssid then
                    r[#r + 1] = colorize_as_dim(params.wireless.ssid)
                end
                if params.wireless.signal_dbm then
                    r[#r + 1] = make_wifi_gauge(params.wireless.signal_dbm)
                end
            elseif iface ~= 'lo' and (params.ipv4 or params.ipv6) then
                r[#r + 1] = colorize_as_dim(string.format('[%s]', iface))
            end
        end
        return r
    end,
}
