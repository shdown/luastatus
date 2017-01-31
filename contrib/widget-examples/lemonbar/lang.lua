lib = require "lib"

widget = {
  plugin = 'xkb',
  cb = function(t)
    local text = ({['us'] = ' En ', ['ru'] = ' Ru '})[t.name] or ' ?? '
    return lib.colorize(text, '#fff', '#0a0')
  end,
}
