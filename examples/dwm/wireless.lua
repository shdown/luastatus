local MIN_DBM, MAX_DBM = -90, -20
local NGAUGE = 5

local function round(x)
    return math.floor(x + 0.5)
end

local function mk_gauge(dbm)
    if dbm < MIN_DBM then dbm = MIN_DBM end
    if dbm > MAX_DBM then dbm = MAX_DBM end
    local nbright = round(NGAUGE * (dbm - MIN_DBM) / (MAX_DBM - MIN_DBM))
    return ('●'):rep(nbright) .. ('○'):rep(NGAUGE - nbright)
end

widget = {
    plugin = 'wireless',
    opts = {timeout = 10},
    cb = function(t)
        if t.what ~= 'update' then
            return nil
        end
        if not t.ssid or not t.signal_dbm then
            return nil
        end
        return t.ssid .. ' ' .. mk_gauge(t.signal_dbm)
    end,
}
