-- you need to install 'lua-curl' and 'gonapps-url-encoder' modules (e.g. with luarocks) 
-- you can look up all available flags here: https://github.com/chubin/wttr.in#one-line-output

local curl = require('cURL')
local encoder = require('gonapps.url.encoder')

local base_url = 'wttr.in'
local lang = 'en'
local location = ''
local PERIOD = 900

function get_weather(format)
    -- encoding is needed to allow usage of special use characters
    format = encoder.encode(format)
    curl.easy{
        url = string.format('%s.%s/%s?format=%s', lang, base_url, location, format),
        writefunction = function (str)
            res = str:gsub("\n","")
        end
    }:perform()
    :close()
    return res
end

widget = {
    plugin = 'timer',
    opts = {period = PERIOD},
    cb = function()
        return {full_text = get_weather('%l: %C %t(%f)')}
    end,
}
