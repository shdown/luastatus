-- Bitcoin price widget.
-- Updates on click and every 5 minutes.
-- Requires:
--     * luasec
--     * this JSON library (you can simply copy json.lua to your widgets directory): https://github.com/rxi/json.lua
local https = require 'ssl.https'
local ltn12 = require 'ltn12'
local json = require 'json'

-- All the arguments except for 'url' may be absent or nil; default method is GET.
function request(url, headers, method, body)
    local resp_body = {}
    local is_ok, code_or_errmsg, resp_headers, _ = https.request(
        {
            url = url,
            sink = ltn12.sink.table(resp_body),
            redirect = false,
            cafile = '/etc/ssl/certs/ca-certificates.crt',
            verify = 'peer',
            method = method,
            headers = headers,
        },
        body)
    assert(is_ok, code_or_errmsg)
    return code_or_errmsg, table.concat(resp_body), resp_headers
end

function request_assert_code(...)
    local code, body, headers = request(...)
    assert(code == 200, 'HTTP ' .. code)
    return body, headers
end

fifo_path = os.getenv('HOME') .. '/.luastatus-btc-pipe'
assert(os.execute('f=' .. fifo_path .. '; set -e; rm -f $f; mkfifo -m600 $f'))
upd_self_cmd = 'touch ' .. fifo_path

widget = {
    plugin = 'timer',
    opts = {
        period = 5 * 60,
        fifo = fifo_path,
    },
    cb = function()
        local is_ok, body = pcall(request_assert_code, 'https://api.coindesk.com/v1/bpi/currentprice/USD.json')
        local text
        if is_ok then
            text = json.decode(body).bpi.USD.rate:match('[^.]+')
        else
            text = '......'
            os.execute('{ sleep 2; exec ' .. upd_self_cmd .. '; }&')
        end
        return {
            full_text = string.format('[<span color="#C0863F">$</span>%s]', text),
            color = '#586A4B',
            markup = 'pango',
        }
    end,
    event = function(t)
        if t.button == 1 then
            os.execute('exec ' .. upd_self_cmd .. '&')
        end
    end,
}
