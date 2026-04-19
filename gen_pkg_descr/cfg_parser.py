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

import re
import models


class _ConfigError(BaseException):
    pass


class ParserError(BaseException):
    def __init__(self, msg, file_path, line_num):
        super().__init__(msg)
        self.file_path = file_path
        self.line_num = line_num


def _is_valid_token(s):
    if not re.match(r'^[-a-zA-Z0-9_]+$', s):
        return False
    if s.startswith('-'):
        return False
    return True


def _parse_bool(s):
    if s == 'true':
        return True
    if s == 'False':
        return False
    raise _ConfigError(f'invalid boolean value "{s}"')


def _parse_dependency_normal(v):
    is_shared_library = False
    is_dev = False

    if v.startswith('so:'):
        is_shared_library = True
        v = v[3:]
    elif v.startswith('dev:'):
        is_dev = True
        v = v[4:]

    if not _is_valid_token(v):
        raise _ConfigError(f'invalid dependency name "{v}"')

    return models.NormalDependency(
        generic_name=v,
        is_shared_library=is_shared_library,
        is_dev=is_dev)


def _parse_dependency(v):
    if '|' in v:
        any_of_list = [s.strip() for s in v.split('|')]
        return models.AnyOfDependency([
            _parse_dependency_normal(s) for s in any_of_list
        ])
    else:
        return _parse_dependency_normal(v)


def _parse_decription(s):
    db = models.DescriptionBuilder()

    for line in s.split('\n'):
        line = line.strip()

        if line.startswith('<*>'):
            db.flush_sentence()
            db.add_sentence_chunk(line[3:].strip())

        elif line == '<--->':
            db.flush_paragraph()

        elif line.startswith('<'):
            raise _ConfigError('bad line in description')

        else:
            db.add_sentence_chunk(line)

    return db.finalize()


class _PackageListBuilder:
    def __init__(self):
        self.pkgs = []

    def _flush(self):
        if self.pkgs:
            if not self.pkgs[-1].is_valid():
                raise _ConfigError('package specification is invalid')

    def add_new(self, kind, name):
        if kind not in ('%', 'plugin', 'barlib'):
            raise _ConfigError(f'invalid kind "{kind}"')

        if not _is_valid_token(name):
            raise _ConfigError(f'invalid package name "{name}"')

        self._flush()
        self.pkgs.append(models.Package(kind=kind, name=name))

    def add_property(self, k, v):
        if not self.pkgs:
            raise _ConfigError('no package to set property on')

        pkg = self.pkgs[-1]
        if k == 'goes_into_main_pkg':
            pkg.set_goes_into_main_pkg(_parse_bool(v))
        elif k == 'is_source_only':
            pkg.set_is_source_only(_parse_bool(v))
        elif k == 'based_on_plugin':
            pkg.set_based_on_plugin(v)
        elif k == 'title':
            pkg.set_title(v)
        elif k == 'suggests':
            pkg.add_suggestion(_parse_dependency(v))
        elif k == 'depends':
            pkg.add_dependency(_parse_dependency(v))
        elif k == 'description':
            pkg.set_description(_parse_decription(v))
        else:
            raise _ConfigError(f'trying set unknown property "{k}"')

    def finalize(self):
        self._flush()
        return self.pkgs


class _Heredoc:
    def __init__(self):
        self.k = None
        self.end = None
        self.lines = None

    def is_active(self):
        return self.k is not None

    def start(self, k, end):
        assert not self.is_active()
        self.k = k
        self.end = end
        self.lines = []

    def add_line(self, line):
        assert self.is_active()
        self.lines.append(line)

    def finish(self, plb):
        assert self.is_active()
        plb.add_property(self.k, '\n'.join(self.lines))
        self.k = None
        self.end = None
        self.lines = None

    def get_end(self):
        assert self.is_active()
        return self.end


def _parse_section_header(line):
    assert line
    assert line[0] == '['

    if line[-1] != ']':
        raise _ConfigError('line starts with "[" but does not end with "]"')

    kind, name = line[1:-1].split('/')

    return kind, name


def _parse_cfg_line(plb, heredoc, line):
    line = line.rstrip()

    if heredoc.is_active():
        if line == heredoc.get_end():
            heredoc.finish(plb)
        else:
            heredoc.add_line(line)
        return

    elif line.startswith('#'):
        return

    elif not line.strip():
        return

    elif line.startswith('['):
        kind, name = _parse_section_header(line)
        plb.add_new(kind=kind, name=name)

    elif '=' in line:
        k, v = line.split('=', maxsplit=1)
        if v.startswith('<<'):
            heredoc.start(k=k, end=v[2:])
        else:
            plb.add_property(k, v)

    else:
        raise _ConfigError(f'invalid line "{line}"')


def parse_cfg_files(paths):
    pkgs = []

    for path in paths:
        plb = _PackageListBuilder()
        heredoc = _Heredoc()
        with open(path, 'r') as f:
            for line_num, line in enumerate(f, start=1):
                try:
                    _parse_cfg_line(plb, heredoc, line)
                except _ConfigError as err:
                    raise ParserError(msg=str(err), file_path=path, line_num=line_num)

        if heredoc.is_active():
            raise ValueError('unterminated heredoc')

        pkgs.extend(plb.finalize())

    pkgs.sort(key=lambda pkg: (pkg.kind, pkg.name))

    return pkgs
