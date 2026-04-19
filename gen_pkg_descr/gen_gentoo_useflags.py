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

import argparse


def pkg_name_to_snake_case(name):
    return name.replace('-', '_')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('cfg_file', nargs='+', default=[])
    args = ap.parse_args()

    pkgs = cfg_parser.parse_cfg_files(args.cfg_file)

    print('BARLIBS="')
    for pkg in pkgs:
        if pkg.kind == 'barlib':
            print(' ${PN}_barlibs_' + pkg_name_to_snake_case(pkg.name))
    print('"')

    print('PLUGINS="')
    for pkg in pkgs:
        if pkg.kind == 'plugin':
            sigil = '+' if pkg.goes_into_main_pkg else ' '
            print(' %s${PN}_plugins_%s' % (sigil, pkg_name_to_snake_case(pkg.name)))
    print('"')

    print('REQUIRED_USE="')
    for pkg in pkgs:
        if pkg.kind == 'plugin' and (pkg.based_on_plugin is not None):
            derived_plugin_useflag = '${PN}_plugins_' + pkg_name_to_snake_case(pkg.name)
            proper_plugin_useflag = '${PN}_plugins_' + pkg_name_to_snake_case(pkg.based_on_plugin)
            print(f' {derived_plugin_useflag}? ( {proper_plugin_useflag} )')

    print(' ^^ ( lua_targets_lua5-1 lua_targets_lua5-3 lua_targets_lua5-4 lua_targets_luajit )')

    print('"')


if __name__ == '__main__':
    main()
