-- Bitcoin price widget.
-- Updates on click and every 5 minutes.

local custom_sleep_amt = nil

local function check_error(t)
    if t.error then
        -- Low-level libcurl error
        return t.error
    end
    if t.status < 200 or t.status > 299 then
        -- Bad HTTP status
        return string.format('HTTP status %d', t.status)
    end
    -- Everything's OK
    return nil
end

local function stylize(styles, fmt, ...)
    local pos_args = {...}
    local res, _ = fmt:gsub('([@%%])([0-9])', function(sigil, digit)
        local i = tonumber(digit)
        if sigil == '@' then
            if i == 0 then
                return '%{F-}'
            end
            return styles[i]
        else
            assert(i ~= 0)
            return pos_args[i]
        end
    end)
    return res
end

widget = {
    plugin = 'web',
    opts = {
        planner = function()
            while true do
                coroutine.yield({action = 'request', params = {
                    url = 'https://api.binance.com/api/v3/ticker/price?symbol=BTCUSDT',
                    timeout = 5,
                }})
                local period = custom_sleep_amt or (5 * 60)
                custom_sleep_amt = nil
                coroutine.yield({action = 'sleep', period = period})
            end
        end,
        make_self_pipe = true,
    },
    cb = function(t)
        local text
        local err_msg = check_error(t)
        if err_msg then
            print(string.format('WARNING: luastatus: btc-price widget: %s'), err_msg)
            text = '......'
            custom_sleep_amt = 5
        else
            local obj = assert(luastatus.plugin.json_decode(t.body))
            text = obj.price:match('[^.]+')
        end
        return stylize(
            {'%{F#586A4B}', '%{F#C0863F}'},
            '@1[@2$@1%1]@0',
            luastatus.barlib.escape(text)
        )
    end,
    event = function(t)
        if t.button == 1 then
            luastatus.plugin.wake_up()
        end
    end,
}
