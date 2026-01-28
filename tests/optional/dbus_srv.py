#!/usr/bin/env python3

# Copyright (C) 2026  luastatus developers
#
# This file is part of luastatus.
#
# luastatus is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# luastatus is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with luastatus.  If not, see <https://www.gnu.org/licenses/>.

# ================================================================================
# This is a simple D-Bus server. It is needed for testing plugin functions of the
# "dbus" plugin.
# ================================================================================

import sys

import gi.repository.GLib
import dbus
import dbus.service
import dbus.mainloop.glib


MY_INTERFACE = 'io.github.shdown.luastatus.test'


MY_BUS_NAME = 'io.github.shdown.luastatus.test'


MY_OBJECT_PATH = '/io/github/shdown/luastatus/test/MyObject'


PROP_INTERFACE = 'org.freedesktop.DBus.Properties'


def log(s):
    print(f'dbus_srv.py: {s}', file=sys.stderr)


class MyServerObject(dbus.service.Object):
    def __init__(self, bus, object_path):
        dbus.service.Object.__init__(self, bus, object_path, MY_BUS_NAME)
        self.prop = ''

    @dbus.service.method(MY_INTERFACE, in_signature='s', out_signature='s')
    def Upcase(self, msg):
        msg = str(msg)
        log(f'Called Upcase on "{msg}"')
        return str(msg).upper()

    @dbus.service.method(MY_INTERFACE, in_signature='', out_signature='i')
    def ReturnFortyTwo(self):
        log('Called ReturnFortyTwo')
        return 42

    @dbus.service.method(PROP_INTERFACE, in_signature='ss', out_signature='s')
    def Get(self, _, prop_name):
        prop_name = str(prop_name)
        log(f'Called Get (property): prop_name="{prop_name}"')
        if prop_name != 'MyProperty':
            raise ValueError('wrong property name')
        return self.prop

    @dbus.service.method(PROP_INTERFACE, in_signature='s', out_signature='a{sv}')
    def GetAll(self, _):
        log('Called GetAll (properties)')
        return {
            'MyProperty': self.prop,
        }

    @dbus.service.method(PROP_INTERFACE, in_signature='sss', out_signature='')
    def Set(self, _, prop_name, value):
        prop_name = str(prop_name)
        value = str(value)
        log(f'Called Set (property): prop_name="{prop_name}", value="{value}"')
        if prop_name != 'MyProperty':
            raise ValueError('wrong property name')
        self.prop = value


def main():
    args = sys.argv[1:]
    if len(args) != 1:
        print(f'USAGE: {sys.argv[0]} FIFO_FILE', file=sys.stderr)
        sys.exit(2)

    log('Opening FIFO...')
    with open(args[0], 'w') as fifo_file:
        log('OK, opened')

        dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)

        bus = dbus.SessionBus()
        bus_name = dbus.service.BusName(MY_BUS_NAME, bus, do_not_queue=True)

        log(f'Acquired bus name "{bus_name.get_name()}"')

        my_object = MyServerObject(bus, MY_OBJECT_PATH)

        mainloop = gi.repository.GLib.MainLoop()

        log('Starting up')
        print('running', file=fifo_file, flush=True)
        mainloop.run()


if __name__ == '__main__':
    main()
