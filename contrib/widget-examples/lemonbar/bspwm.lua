lib = require 'lib'

f = io.popen('exec bspc control --subscribe', 'r')

widget = {
    plugin = 'timer',
    cb = function()
        return lib.pager(f:read('*line'))
    end,
}
