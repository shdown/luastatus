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
import wrap_ascii

import argparse


class Resolver:
    def __init__(self):
        self.map = {}

    def add_mapping(self, key, value):
        self.map[key] = value.split()

    def resolve(self, key):
        return self.map[key]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--lua-pkg-suffix', required=True)
    ap.add_argument('cfg_file', nargs='+', default=[])
    args = ap.parse_args()

    pkgs = cfg_parser.parse_cfg_files(args.cfg_file)

    resolver = Resolver()

    resolver.add_mapping('lib_alsa', 'libasound2-dev')
    resolver.add_mapping('lib_glib', 'libglib2.0-dev')
    resolver.add_mapping('lib_gio', 'libgio-2.0-dev')
    resolver.add_mapping('lib_curl', 'libcurl4-openssl-dev')
    resolver.add_mapping('lib_cjson', 'libcjson-dev')
    resolver.add_mapping('lib_yajl', 'libyajl-dev')
    resolver.add_mapping('lib_nl', 'libnl-3-dev')
    resolver.add_mapping('lib_nl_genl', 'libnl-genl-3-dev')
    resolver.add_mapping('lib_pulse', 'libpulse-dev')
    resolver.add_mapping('lib_udev', 'libudev-dev')
    resolver.add_mapping('lib_x11', 'libx11-dev')
    resolver.add_mapping('lib_xcb', 'libxcb1-dev')
    resolver.add_mapping('lib_xcb_ewmh', 'libxcb-ewmh-dev')
    resolver.add_mapping('lib_xcb_icccm', 'libxcb-icccm4-dev')
    resolver.add_mapping('lib_xcb_event', 'libxcb-util-dev')
    resolver.add_mapping('linux_headers', 'linux-libc-dev')

    resolver.add_mapping('lib_lua', f'liblua{args.lua_pkg_suffix}-dev')

    build_deps = []

    for pkg in pkgs:
        for dep in pkg.depends:
            if not (dep.is_shared_library or dep.is_dev):
                continue

            if not isinstance(dep, models.NormalDependency):
                raise ValueError('only normal dependencies on build-deps are supported')

            for debian_pkg_name in resolver.resolve(dep.generic_name):
                build_deps.append(debian_pkg_name)

    build_deps = list(set(build_deps))
    build_deps.sort()

    for debian_pkg_name in build_deps:
        print(f' {debian_pkg_name},')


if __name__ == '__main__':
    main()
