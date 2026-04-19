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

    print(' local with_lualib')
    print(' if use lua_targets_luajit; then with_lualib=luajit; fi')
    print(' if use lua_targets_lua5-1; then with_lualib=lua5.1; fi')
    print(' if use lua_targets_lua5-3; then with_lualib=lua5.3; fi')
    print(' if use lua_targets_lua5-4; then with_lualib=lua5.4; fi')

    print(' local mycmakeargs=(')
    print('  -DWITH_LUA_LIBRARY=$with_lualib')
    print('  -DBUILD_DOCS=$(usex doc)')

    for pkg in pkgs:
        normalized_name = pkg_name_to_snake_case(pkg.name)

        if pkg.kind == 'plugin':
            useflag = '${PN}_plugins_' + normalized_name
            cmakeflag = '-DBUILD_PLUGIN_' + normalized_name.upper()
        elif pkg.kind == 'barlib':
            useflag = '${PN}_barlibs_' + normalized_name
            cmakeflag = '-DBUILD_BARLIB_' + normalized_name.upper()
        elif pkg.kind == '%':
            continue
        else:
            raise ValueError('unexpected pkg.kind')

        print(f'  {cmakeflag}=$(usex {useflag})')

    print(' )')


if __name__ == '__main__':
    main()
