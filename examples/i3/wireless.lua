local ORIGIN = '•'  -- '●' '○' '◌' '◍' '⬤'
local RAY = '}'     -- '›' '〉' '❭' '❯' '❱' '⟩' '⟫' '》'
local MIN_DBM, MAX_DBM = -90, -20
local NGAUGE = 5

local function round(x)
    return math.floor(x + 0.5)
end

local function mk_gauge(dbm)
    if dbm < MIN_DBM then dbm = MIN_DBM end
    if dbm > MAX_DBM then dbm = MAX_DBM end
    local nbright = round(NGAUGE * (1 - 0.7 * (MAX_DBM - dbm) / (MAX_DBM - MIN_DBM)))
    return {
        full_text = ORIGIN .. RAY:rep(nbright) .. '<span color="#709080">' .. RAY:rep(NGAUGE - nbright) .. '</span>',
        markup = 'pango',
    }
end

widget = {
    plugin = 'wireless',
    opts = {timeout = 10},
    cb = function(t)
        if t.what ~= 'update' then
            return nil
        end
        return {
            t.ssid and {full_text = t.ssid, color = '#709080'},
            t.signal_dbm and mk_gauge(t.signal_dbm),
        }
    end,
}
