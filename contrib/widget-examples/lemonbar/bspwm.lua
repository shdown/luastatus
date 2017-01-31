lib = require 'lib'

widget = {
  plugin = 'pipe',
  opts = {
    args = {'bspc', 'control', '--subscribe'},
  },
  cb = function(t)
    return lib.pager(t)
  end,
}
