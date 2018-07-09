lib = require 'lib'

widget = luastatus.require_plugin('pipe').widget{
    command = 'exec bspc control --subscribe',
    cb = lib.pager,
}
