function colorize (text, fg, bg, u)
  text = ( fg and '%{F' .. fg .. '}' .. text .. '%{F-}' ) or text
  text = ( bg and '%{B' .. bg .. '}' .. text .. '%{B-}' ) or text
  text = ( u and '%{U' .. u .. '}' .. text .. '%{U-}' ) or text
  return text
end

function pager (status)
  local colors = {
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

  local text = ''
  local status = status:sub(2, #status)
  for block in status:gmatch("[^:]+") do
    text = text .. 
      colorize(
        string.format(' %s ', block:sub(2,#block)), 
        colors[block:sub(1,1)].fg, 
        colors[block:sub(1,1)].bg
      )

  end

  return text
end
