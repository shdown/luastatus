lib = require 'lib'
utf8 = require 'utf8' -- git@github.com:Stepets/utf8.lua.git

widget = {
  plugin = 'xtitle',
  cb = function(t)
    t = t or ''
    t = (utf8.len(t) < 100) and t or (utf8.sub(t, 1, 97) .. '...')
    t = luastatus.barlib.escape(t)
    return '%{c}' .. colorize(t, '#ffe') .. '%{r}'
  end,
}
