dofile 'lib.lua'

widget = {
  plugin = 'xtitle',
  cb = function(t)
    t = t or ''
    -- need utf8 lib
    t = (#t < 100 and t) or string.sub(t, 1, 100) .. '...'
    return '%{c}' .. colorize(t, '#ffe') .. '%{r}'
  end,
}
