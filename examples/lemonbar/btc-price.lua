-- Bitcoin price widget.
-- Updates on click and every 5 minutes.
-- Requires (luarocks packages):
--     * luasec
--     * lua-cjson
https = require 'ssl.https'
ltn12 = require 'ltn12'
cjson = require 'cjson'

-- All the arguments except for 'url' may be absent or nil; default method is GET.
-- Returns: code (integer), body (string), headers (table), status (string).
function request(url, headers, method, body)
    local out_body = {}
    local is_ok, code_or_errmsg, out_headers, status = https.request(
        {
            url = url,
            sink = ltn12.sink.table(out_body),
            redirect = false,
            cafile = '/etc/ssl/certs/ca-certificates.crt',
            verify = 'peer',
            method = method,
            headers = headers,
        },
        body)
    assert(is_ok, code_or_errmsg)
    return code_or_errmsg, table.concat(out_body), out_headers, status
end

-- Arguments are the same to those of 'request'.
-- Returns: body (string), headers (table).
function request_check_code(...)
    local code, body, headers, status = request(...)
    assert(code == 200, string.format('HTTP %s %s', code, status))
    return body, headers
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
    plugin = 'timer',
    opts = {
        period = 5 * 60,
        make_self_pipe = true,
    },
    cb = function()
        local is_ok, body = pcall(request_check_code, 'https://api.binance.com/api/v3/ticker/price?symbol=BTCUSDT')
        local text
        if is_ok then
            text = cjson.decode(body).price:match('[^.]+')
        else
            text = '......'
            luastatus.plugin.push_period(5) -- retry in 5 seconds
        end
        return stylize(
            {'%{F#586A4B}', '%{F#C0863F}'},
            '@1[@2$@1%1]@0',
            luastatus.barlib.escape(text)
        )
    end,
}
