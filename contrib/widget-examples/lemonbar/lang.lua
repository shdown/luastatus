dofile 'lib.lua'

widget = {
  plugin = 'xkb',
  cb = function(t)
    local text = ''
    if t.name == 'us' then
      text = ' En '
    elseif t.name == 'ru' then
      text = ' Ru '
    else
      text = ' ?? '
    end
    return colorize(text, '#fff', '#0a0')
  end,
}
