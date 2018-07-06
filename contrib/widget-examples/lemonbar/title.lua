-- you need to install 'utf8' module (e.g. with luarocks) if using Lua <=5.2.
utf8 = require 'utf8'
lib = require 'lib'

widget = {
    plugin = 'xtitle',
    cb = function(t)
        t = t or ''
        t = (utf8.len(t) < 100) and t or (utf8.sub(t, 1, 97) .. '...')
        t = luastatus.barlib.escape(t)
        return '%{c}' .. colorize(t, '#ffe') .. '%{r}'
    end,
}
