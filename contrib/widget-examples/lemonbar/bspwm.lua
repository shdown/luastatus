dofile 'lib.lua'

widget = {
  plugin = 'pipe',
  opts = { 
    args = { 'bspc', 'control', '--subscribe' }, 
  },
  cb = function(t)
    return pager(t)
  end,
}
