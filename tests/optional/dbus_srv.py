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

    @dbus.service.method(MY_INTERFACE, in_signature='as', out_signature='a{ss}')
    def ConvertArrayToDictHexify(self, arr):
        log('Called ConvertArrayToDictHexify')
        def _hexify(s):
            return s.encode('utf8').hex().upper()
        return {
            _hexify(str(i)): _hexify(s)
            for i, s in enumerate(arr)
        }

    @dbus.service.method(MY_INTERFACE, in_signature='a{ss}', out_signature='s')
    def ConvertDictToString(self, d):
        log('Called ConvertDictToString')
        return ','.join(f'{key}:{value}' for key, value in sorted(d.items()))

    @dbus.service.method(MY_INTERFACE, in_signature='', out_signature='s')
    def RecvTuple0(self):
        log('Called RecvTuple0')
        return 'Empty'

    @dbus.service.method(MY_INTERFACE, in_signature='s', out_signature='s')
    def RecvTuple1(self, arg):
        log('Called RecvTuple1')
        return arg

    @dbus.service.method(MY_INTERFACE, in_signature='ss', out_signature='s')
    def RecvTuple2(self, arg1, arg2):
        log('Called RecvTuple2')
        return arg1 + arg2

    @dbus.service.method(MY_INTERFACE, in_signature='sss', out_signature='s')
    def RecvTuple3(self, arg1, arg2, arg3):
        log('Called RecvTuple3')
        return arg1 + arg2 + arg3

    @dbus.service.method(MY_INTERFACE, in_signature='v', out_signature='s')
    def RecvVariant(self, x):
        log(f'Called RecvVariant, type(x)={type(x)}')

        TABLE = [
            (dbus.Boolean, 'bool'),
            (dbus.Double, 'double'),
            (dbus.Byte, 'byte'),
            (dbus.String, 'string'),
            (dbus.ObjectPath, 'object_path'),
            (dbus.Signature, 'signature'),

            (dbus.Int16, 'i16'),
            (dbus.Int32, 'i32'),
            (dbus.Int64, 'i64'),

            (dbus.UInt16, 'u16'),
            (dbus.UInt32, 'u32'),
            (dbus.UInt64, 'u64'),
        ]

        if hasattr(x, 'variant_level'):
            if x.variant_level > 0:
                return 'variant'

        for dbus_type, retval in TABLE:
            if isinstance(x, dbus_type):
                return retval

        raise ValueError('Got Variant wrapping an unsupported type')

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
