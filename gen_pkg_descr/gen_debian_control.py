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
        self.map[key] = value

    def resolve(self, key):
        return self.map[key]


def make_pkg_name(pkg):
    if pkg.kind == '%':
        return pkg.name
    else:
        return f'luastatus-{pkg.kind}-{pkg.name}'


class DebianPkgList:
    def __init__(self):
        self.pkg_names = []
        self.has_shared_libraries = False

    def add_dependency(self, pkg_name):
        self.pkg_names.append(pkg_name)

    def add_shared_library(self):
        self.has_shared_libraries = True


def handle_dependency(resolver, DPL, dep, is_suggestion):
    if isinstance(dep, models.AnyOfDependency):
        proxy_DPL = DebianPkgList()

        for sub_dep in dep.normal_deps:
            handle_dependency(resolver, proxy_DPL, sub_dep, is_suggestion=is_suggestion)

        if proxy_DPL.pkg_names:
            DPL.add_dependency(' | '.join(proxy_DPL.pkg_names))

    elif isinstance(dep, models.NormalDependency):
        if dep.is_shared_library:
            DPL.add_shared_library()
        elif not dep.is_dev:
            pkg_name = resolver.resolve(dep.generic_name)
            if pkg_name is None:
                if not is_suggestion:
                    raise ValueError(
                        f'generic name "{dep.generic_name}" for a ' +
                        'required dependency cannot be resolved')
            else:
                DPL.add_dependency(pkg_name)

    else:
        raise ValueError('unexpected type(dep)')


def print_dependencies(resolver, field_name, deps, is_suggestion):
    DPL = DebianPkgList()
    for dep in deps:
        handle_dependency(resolver, DPL, dep, is_suggestion=is_suggestion)

    result = []
    if DPL.has_shared_libraries:
        result.append('${shlibs:Depends}')
        result.append('${misc:Depends}')

    result.extend(DPL.pkg_names)

    if result:
        print(field_name + ': ' + ', '.join(result))
    else:
        print(field_name + ':')


def replace_in_sentences(sentences, old_str, new_str):
    return [
        models.Sentence(sentence.content.replace(old_str, new_str))
        for sentence in sentences
    ]


def add_mappings_to_resolver(resolver, lua_ver):
    resolver.add_mapping('pkg_i3wm', 'i3-wm')
    resolver.add_mapping('pkg_dwm', 'dwm')
    resolver.add_mapping('pkg_lemonbar', 'lemonbar')
    resolver.add_mapping('pkg_dzen2', 'dzen2')
    resolver.add_mapping('pkg_xmobar', 'xmobar')
    resolver.add_mapping('pkg_yabar', None)
    resolver.add_mapping('pkg_dvtm', 'dvtm')

    resolver.add_mapping('lualib_lua_socket', f'lua{lua_ver}-socket')
    resolver.add_mapping('lualib_lua_sec', f'lua{lua_ver}-sec')

    resolver.add_mapping('pkg_pulseaudio', 'pulseaudio')
    resolver.add_mapping('pkg_systemd', 'systemd')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--lua-ver', required=True)
    ap.add_argument('cfg_file', nargs='+', default=[])
    args = ap.parse_args()

    pkgs = cfg_parser.parse_cfg_files(args.cfg_file)

    plugins_in_main_pkg = ', '.join(
        pkg.name
        for pkg in pkgs
        if pkg.kind == 'plugin' and pkg.goes_into_main_pkg
    )

    resolver = Resolver()
    add_mappings_to_resolver(resolver, lua_ver=args.lua_ver)

    for pkg in pkgs:
        if pkg.goes_into_main_pkg:
            continue

        print()
        print(f'Package: {make_pkg_name(pkg)}')
        print(f'Architecture: {"all" if pkg.is_source_only else "any"}')

        print_dependencies(resolver, 'Depends', pkg.depends, is_suggestion=False)
        print('Recommends:')
        print_dependencies(resolver, 'Suggests', pkg.suggests, is_suggestion=True)

        print(f'Description: {pkg.title}')
        for i, paragraph in enumerate(pkg.description.paragraphs):
            sentences = replace_in_sentences(
                paragraph.sentences,
                '${__main_pkg_plugins__}',
                plugins_in_main_pkg
            )
            text = wrap_ascii.wrap_ascii(
                sentences=sentences,
                width=80,
                padding=1,
                sentence_spacing=2
            )
            if i:
                print(' .')
            print(text)


if __name__ == '__main__':
    main()
