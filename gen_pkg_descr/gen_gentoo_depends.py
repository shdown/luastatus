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

import cfg_parser
import models

import argparse


class Resolver:
    def __init__(self):
        self.map = {}

    def add_mapping(self, key, value):
        self.map[key] = value

    def add_mapping_multi(self, keys, value):
        for key in keys:
            self.add_mapping(key, value)

    def resolve(self, key):
        return self.map[key]



def add_mappings_to_resolver(resolver):
    resolver.add_mapping(
        'lib_lua',
        [
            'lua_targets_lua5-1? ( dev-lang/lua:5.1 )',
            'lua_targets_lua5-3? ( dev-lang/lua:5.3 )',
            'lua_targets_lua5-4? ( dev-lang/lua:5.4 )',
            'lua_targets_luajit? ( dev-lang/luajit )',
        ]
    )

    resolver.add_mapping('lib_yajl', 'dev-libs/yajl')

    resolver.add_mapping('lib_alsa', 'media-libs/alsa-lib')
    resolver.add_mapping_multi(['lib_glib', 'lib_gio'], 'dev-libs/glib')

    resolver.add_mapping('linux_headers', 'sys-kernel/linux-headers')

    resolver.add_mapping_multi(['lib_nl', 'lib_nl_genl'], 'dev-libs/libnl')

    resolver.add_mapping('lib_pulse', 'media-libs/libpulse')
    resolver.add_mapping('lib_udev', 'virtual/libudev')
    resolver.add_mapping('lib_curl', 'net-misc/curl')
    resolver.add_mapping('lib_cjson', 'dev-libs/cJSON')
    resolver.add_mapping('lib_x11', 'x11-libs/libX11')

    resolver.add_mapping('lib_xcb', 'x11-libs/libxcb')
    resolver.add_mapping_multi(['lib_xcb_ewmh', 'lib_xcb_icccm'], 'x11-libs/xcb-util-wm')
    resolver.add_mapping('lib_xcb_event', 'x11-libs/xcb-util')

    resolver.add_mapping(
        'lualib_lua_socket',
        [
            'lua_targets_luajit? ( dev-lua/luasocket[lua_targets_luajit] )',
            'lua_targets_lua5-1? ( dev-lua/luasocket[lua_targets_lua5-1] )',
            'lua_targets_lua5-3? ( dev-lua/luasocket[lua_targets_lua5-3] )',
            'lua_targets_lua5-4? ( dev-lua/luasocket[lua_targets_lua5-4] )',
        ]
    )

    resolver.add_mapping(
        'lualib_lua_sec',
        [
            'lua_targets_luajit? ( dev-lua/luasec[lua_targets_luajit] )',
            'lua_targets_lua5-1? ( dev-lua/luasec[lua_targets_lua5-1] )',
            'lua_targets_lua5-3? ( dev-lua/luasec[lua_targets_lua5-3] )',
            'lua_targets_lua5-4? ( dev-lua/luasec[lua_targets_lua5-4] )',
        ]
    )

    resolver.add_mapping('pkg_systemd', 'sys-apps/systemd')
    resolver.add_mapping('pkg_pulseaudio', 'media-sound/pulseaudio-daemon')


def pkg_name_to_snake_case(name):
    return name.replace('-', '_')


def print_rdependency(useflag, pkg, resolver):
    if not pkg.depends:
        return

    chunks = []
    if useflag is not None:
        chunks.append(f'{useflag}? ( ')

    simple_deps = set()

    for dep in pkg.depends:
        if not isinstance(dep, models.NormalDependency):
            raise ValueError('any-of dependencies are not supported yet')

        content = resolver.resolve(dep.generic_name)
        if isinstance(content, str):
            simple_deps.add(content)
        elif isinstance(content, list):
            for line in content:
                chunks.append('\n  ' + line.strip())
        else:
            raise ValueError('unexpected type(content)')

    chunks.append(' '.join(simple_deps))

    if useflag is not None:
        chunks.append(' )')

    result = ''.join(chunks)
    for line in result.split('\n'):
        line = line.rstrip()
        if line:
            print(' ' + line)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('cfg_file', nargs='+', default=[])
    args = ap.parse_args()

    pkgs = cfg_parser.parse_cfg_files(args.cfg_file)

    resolver = Resolver()
    add_mappings_to_resolver(resolver)

    print('DEPEND="')
    print(' doc? ( dev-python/docutils )')
    print('"')

    print('RDEPEND="')

    for pkg in pkgs:
        if pkg.kind == 'plugin':
            useflag = '${PN}_plugins_' + pkg_name_to_snake_case(pkg.name)
        elif pkg.kind == 'barlib':
            useflag = '${PN}_barlibs_' + pkg_name_to_snake_case(pkg.name)
        elif pkg.kind == '%':
            useflag = None
        else:
            raise ValueError('unexpected pkg.kind')

        print_rdependency(useflag, pkg, resolver)

    print('"')


if __name__ == '__main__':
    main()

