#!/usr/bin/env python3

# Copyright (C) 2025  luastatus developers
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
import sys
import argparse


REGEX_STR_LIST = [
    r'\b(MLC_DECL|MLC_MODE|MLC_INIT|MLC_DEINIT|MLC_RETURN|MLC_PUSH_SCOPE)\("([^"]*)"\)',
    r'\b(MLC_POP_SCOPE)\(()\)',
]


class Settings:
    def __init__(self):
        self.allow_undeclared_structs = False


GLOBAL_SETTINGS = Settings()


class InternalError(BaseException):
    pass


class Annotation:
    def __init__(self, spelling, value, line_num, filename):
        self.spelling = spelling
        self.value = value
        self.line_num = line_num
        self.filename = filename


def say(*args):
    print(*args, file=sys.stderr)


def output_msg(msg_kind, msg, ann=None, mode='', add_newline=True):
    say(f'There is {msg_kind}')
    if ann is not None:
        say(f'  regarding annotation {ann.spelling}("{ann.value}")')
        say(f'  at file "{ann.filename}":{ann.line_num}')
    if mode:
        say(f'  appearing in mode "{mode}"', file=sys.stderr)

    say('---')

    msg = msg[:1].upper() + msg[1:]
    say(msg)

    say('---')

    if add_newline:
        say()


def err(msg, ann=None, mode=''):
    output_msg('ERROR', msg, ann=ann, mode=mode, add_newline=False)
    sys.exit(1)


def warn(msg, ann=None, mode=''):
    output_msg('WARNING', msg, ann=ann, mode=mode)


def get_annotations(filename):
    results = []
    def add_matches(matches, line_num):
        for m in matches:
            results.append(Annotation(
                spelling=m[0],
                value=m[1],
                line_num=line_num,
                filename=filename
            ))

    regexes = [re.compile(re_str) for re_str in REGEX_STR_LIST]
    with open(filename, 'r') as f:
        for line_num, line in enumerate(f, start=1):
            for regex in regexes:
                matches = regex.findall(line)
                add_matches(matches, line_num)

    return results


class Scope:
    def __init__(self, name, opening_ann):
        self.name = name
        self.opening_ann = opening_ann
        self.mode = ''
        self.declared = []
        self.inited = []
        self.deinited = {}


class StructScope(Scope):
    def __init__(self, opening_ann, struct_name, stage):
        super().__init__(f'struct/{struct_name}:{stage}', opening_ann)
        self.struct_name = struct_name
        self.stage = stage


class FuncScope(Scope):
    def __init__(self, opening_ann, func_name):
        super().__init__(f'func/{func_name}', opening_ann)
        self.func_name = func_name


class StructDeclDB:
    def __init__(self):
        self.data = {}

    def set_new(self, k, v):
        if k in self.data:
            return False
        self.data[k] = v
        return True

    def get_optional(self, k):
        return self.data.get(k)


STRUCT_DECL_DB = StructDeclDB()


def check_match(A, A_name, B, B_name, ann, mode=''):
    A_set = set()
    for a_elem in A:
        x = a_elem.value
        if x in A_set:
            err(f'{A_name} not unique (repeating "{x}")', ann=ann, mode=mode)
        A_set.add(x)

    B_set = set()
    for b_elem in B:
        y = b_elem.value
        if y in B_set:
            err(f'{B_name} not unique (repeating "{y}")')
        if y not in A_set:
            err(f'"{y}": {B_name}, but not {A_name}', ann=ann, mode=mode)
        B_set.add(y)

    for x in A_set:
        if x not in B_set:
            err(f'"{x}": {A_name}, but not {B_name}', ann=ann, mode=mode)


def iter_over_modes(D):
    always_yes = []
    custom_modes = list(D.keys())
    if '' in D:
        always_yes = D['']
        custom_modes.remove('')

    if custom_modes:
        for mode in custom_modes:
            yield mode, always_yes + D[mode]
    else:
        yield '', always_yes


def CMD_close(scope, ann):

    if isinstance(scope, FuncScope):
        for k, v in iter_over_modes(scope.deinited):
            check_match(
                scope.inited, 'inited', v, 'deinited',
                ann=ann,
                mode=k)

    elif isinstance(scope, StructScope):

        if scope.stage == 'decl':
            is_ok = STRUCT_DECL_DB.set_new(scope.struct_name, scope.declared)
            if not is_ok:
                err(f'struct "{scope.struct_name}" already declared', ann=ann)

        elif scope.stage == 'init':
            decl_list = STRUCT_DECL_DB.get_optional(scope.struct_name)
            if decl_list is not None:
                check_match(decl_list, 'declared', scope.inited, 'inited', ann=ann)
            else:
                if GLOBAL_SETTINGS.allow_undeclared_structs:
                    warn(f'No decl list for struct "{scope.struct_name}" available, using inited-list instead', ann=ann)
                    STRUCT_DECL_DB.set_new(scope.struct_name, scope.inited)
                else:
                    err(f'No decl list for struct "{scope.struct_name}" available', ann=ann)

        elif scope.stage == 'deinit':
            decl_list = STRUCT_DECL_DB.get_optional(scope.struct_name)
            if decl_list is None:
                err(f'no decl list for struct "{scope.struct_name}"', ann=ann)
            for k, v in iter_over_modes(scope.deinited):
                check_match(
                    decl_list, 'declared', v, 'deinited',
                    ann=ann, mode=k)
        else:
            raise InternalError(f'unknown scope.stage value "{scope.stage}"')

    else:
        raise InternalError('unknown scope class')


def CMD_set_mode(scope, ann):
    scope.mode = ann.value


def CMD_declare(scope, ann):
    if not isinstance(scope, StructScope):
        err('declarations are only possible in struct scope', ann=ann)
    if scope.stage != 'decl':
        err('declarations are only possible in "decl" stage', ann=ann)
    if scope.mode:
        err('only deinits are allowed under MLC_MODE(...)', ann=ann)
    scope.declared.append(ann)


def CMD_init(scope, ann):
    if isinstance(scope, StructScope):
        if scope.stage != 'init':
            err('for struct scopes, inits are only possible in "init" stage', ann=ann)
    if scope.mode:
        err('only deinits are allowed under MLC_MODE(...)', ann=ann)
    scope.inited.append(ann)


def CMD_deinit(scope, ann):
    if isinstance(scope, StructScope):
        if scope.stage != 'deinit':
            err('for struct scopes, deinits are only possible in "deinit" stage', ann=ann)
    deinited_list = scope.deinited.setdefault(scope.mode, [])
    deinited_list.append(ann)


def check_file(filename):
    scopes = []
    anns = list(get_annotations(filename))

    def check_scopes_nonempty(ann):
        if not scopes:
            err('there are no opened scopes', ann=ann)

    for ann in anns:

        if ann.spelling == 'MLC_PUSH_SCOPE':
            if ':' in ann.value:
                struct_name, stage = ann.value.split(':')
                if stage not in ['decl', 'init', 'deinit']:
                    err('invalid struct scope stage', ann=ann)
                scopes.append(StructScope(opening_ann=ann, struct_name=struct_name, stage=stage))
            else:
                scopes.append(FuncScope(opening_ann=ann, func_name=ann.value))

        elif ann.spelling == 'MLC_POP_SCOPE':
            check_scopes_nonempty(ann)
            CMD_close(scopes[-1], ann)
            scopes.pop()

        elif ann.spelling == 'MLC_MODE':
            check_scopes_nonempty(ann)
            CMD_set_mode(scopes[-1], ann)

        elif ann.spelling == 'MLC_DECL':
            check_scopes_nonempty(ann)
            CMD_declare(scopes[-1], ann)

        elif ann.spelling == 'MLC_INIT':
            check_scopes_nonempty(ann)
            CMD_init(scopes[-1], ann)

        elif ann.spelling in ['MLC_DEINIT', 'MLC_RETURN']:
            check_scopes_nonempty(ann)
            CMD_deinit(scopes[-1], ann)

        else:
            raise InternalError(f'unknown spelling "{ann.spelling}"')

    if scopes:
        err(f'scope "{scopes[-1].name}" was never popped off', ann=scopes[-1].opening_ann)

    return len(anns)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--allow-undecl', action='store_true', help='allow undeclared structs')
    ap.add_argument('filename', nargs='+')
    args = ap.parse_args()

    GLOBAL_SETTINGS.allow_undeclared_structs = args.allow_undecl

    total_annotations = 0
    for filename in args.filename:
        total_annotations += check_file(filename)

    say('Everything seems to be OK')
    say(f'Processed files: {len(args.filename)}')
    say(f'Processed annotations: {total_annotations}')


if __name__ == '__main__':
    main()
