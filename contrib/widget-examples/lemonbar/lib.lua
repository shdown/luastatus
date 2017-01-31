function colorize(text, fg, bg, u)
  text = (fg and string.format('%%{F%s}%s%%{F-}', fg, text)) or text
  text = (bg and string.format('%%{B%s}%s%%{B-}', bg, text)) or text
  text = (u and string.format('%%{U%s}%s%%{U-}', u, text)) or text
  return text
end

colors = {
  M = { fg='#000000', bg='#ff5900' },
  m = { fg='#ff5900', bg='#000000' },
  O = { fg='#fdf6e3', bg='#808080' },
  o = { fg='#fdf6e3', bg='#000000' },
  F = { fg='#002b36', bg='#808080' },
  f = { fg='#586e57', bg='#000000' },
  U = { fg='#000000', bg='#dc322f' },
  u = { fg='#000000', bg='#dc322f' },
  L = { fg='#ffffff', bg='#000000' },
}

function pager(status)
  local text = ''
  for block in status:sub(2):gmatch("[^:]+") do
    text = text .. 
      colorize(
        string.format(' %s ', block:sub(2)), 
        colors[block:sub(1,1)].fg, 
        colors[block:sub(1,1)].bg
      )
  end
  return text
end

local lib = {}
lib.colorize = colorize
lib.pager = pager
return lib
