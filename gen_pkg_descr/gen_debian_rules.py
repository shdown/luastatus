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

import models
import cfg_parser

import argparse
import sys


def mk_sh_list(pkgs, kind, in_main_pkg):
    result = []
    for pkg in pkgs:
        if pkg.kind == kind and pkg.goes_into_main_pkg == in_main_pkg:
            result.append(pkg.name)
    return ' '.join(result)


def mk_cmake_dflag_list(pkgs, lua_lib):
    result = [f'-DWITH_LUA_LIBRARY={lua_lib}']
    for pkg in pkgs:
        if pkg.kind != '%':
            kind_up = pkg.kind.upper()
            name_up = pkg.name.replace('-', '_').upper()
            result.append(f'-DBUILD_{kind_up}_{name_up}=ON')
    return ' '.join(result)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--lua-lib', required=True)
    ap.add_argument('--input-file', type=open, default=sys.stdin)
    ap.add_argument('cfg_file', nargs='+', default=[])
    args = ap.parse_args()

    pkgs = cfg_parser.parse_cfg_files(args.cfg_file)

    replacements = {
        'barlibs:main': mk_sh_list(pkgs, 'barlib', in_main_pkg=True),
        'barlibs:ext':  mk_sh_list(pkgs, 'barlib', in_main_pkg=False),

        'plugins:main': mk_sh_list(pkgs, 'plugin', in_main_pkg=True),
        'plugins:ext':  mk_sh_list(pkgs, 'plugin', in_main_pkg=False),

        'cmake_dflags': mk_cmake_dflag_list(pkgs, lua_lib=args.lua_lib),
    }

    for line in args.input_file:
        line = line.rstrip('\n')
        for k, v in replacements.items():
            line = line.replace('${__' + k + '__}', v)
        print(line)


if __name__ == '__main__':
    main()
