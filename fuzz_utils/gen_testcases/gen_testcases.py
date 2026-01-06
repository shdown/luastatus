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

import argparse
import sys
import random
import typing

import mod_generator
import mod_mutator
import mod_mutators


LENGTH_DEFAULT = 16


NUM_FILES_DEFAULT = 20


class ChunkSpec:
    def __init__(self, weight: int, chunk: bytes, is_homogeneous: bool) -> None:
        assert weight > 0
        assert chunk
        self.weight = weight
        self.chunk = chunk
        self.is_homogeneous = is_homogeneous


def make_chunk_spec_checked(weight: int, chunk: bytes, is_homogeneous: bool) -> ChunkSpec:
    if weight <= 0:
        raise ValueError('weight must be positive')
    if not chunk:
        raise ValueError('chunk is empty')
    return ChunkSpec(weight=weight, chunk=chunk, is_homogeneous=is_homogeneous)


def into_h_w_and_rest(s: str) -> typing.Tuple[bool, int, str]:
    is_homogeneous = False
    if s.startswith('h:'):
        is_homogeneous = True
        s = s[2:]

    weight_s, rest = s.split(':', maxsplit=1)
    weight = int(weight_s)
    return (is_homogeneous, weight, rest)


def parse_arg_ab(s: str) -> ChunkSpec:
    is_homogeneous, weight, content = into_h_w_and_rest(s)
    return make_chunk_spec_checked(
        weight=weight,
        chunk=content.encode(),
        is_homogeneous=is_homogeneous)


def parse_arg_ab_range(s: str) -> ChunkSpec:
    is_homogeneous, weight, content = into_h_w_and_rest(s)

    first_s, last_s = content.split('-')
    first, last = int(first_s), int(last_s)

    chunk = bytes(list(range(first, last + 1)))

    return make_chunk_spec_checked(
        weight=weight,
        chunk=chunk,
        is_homogeneous=is_homogeneous)


def parse_arg_length(s: str) -> typing.Tuple[int, int]:
    if '-' in s:
        n_min_s, n_max_s = s.split('-')
        n_min, n_max = int(n_min_s), int(n_max_s)
    else:
        n = int(s)
        n_min, n_max = n, n

    if (n_min > n_max) or (n_min < 0) or (n_max < 0):
        raise ValueError('invalid length specifier')

    return (n_min, n_max)


def parse_arg_num_files(s: str) -> int:
    n = int(s)
    if n < 3:
        raise ValueError('--num-files value must be at least 3')
    return n


def parse_arg_prefix(s: str) -> bytes:
    return s.encode()


def parse_arg_substrings(s: str) -> typing.List[bytes]:
    separator = s[0]
    parts = s[1:].split(separator)
    return [part.encode() for part in parts]


def parse_arg_extra_testcase(s: str) -> typing.Tuple[str, bytes]:
    file_suffix, content_s = s.split(':', maxsplit=1)
    return (file_suffix, content_s.encode())


def main() -> None:
    ap = argparse.ArgumentParser()

    ap.add_argument('dest_dir')

    def _add_list_arg(spelling: str, **kwargs: typing.Any) -> None:
        ap.add_argument(spelling, action='append', default=[], **kwargs)

    _add_list_arg('--a',       type=parse_arg_ab,       metavar='<CHUNK_SPEC>')
    _add_list_arg('--a-range', type=parse_arg_ab_range, metavar='<RANGE_CHUNK_SPEC>')
    _add_list_arg('--b',       type=parse_arg_ab,       metavar='<CHUNK_SPEC>')
    _add_list_arg('--b-range', type=parse_arg_ab_range, metavar='<RANGE_CHUNK_SPEC>')

    ap.add_argument(
        '--a-is-important',
        action='store_true')

    ap.add_argument(
        '--length',
        default=(LENGTH_DEFAULT, LENGTH_DEFAULT),
        type=parse_arg_length,
        metavar='<NUMBER>|<MIN>-<MAX>')

    ap.add_argument(
        '--num-files',
        default=NUM_FILES_DEFAULT,
        type=parse_arg_num_files,
        metavar='<NUMBER>')

    ap.add_argument(
        '--mut-prefix',
        type=parse_arg_prefix,
        required=False)

    _add_list_arg('--extra-testcase', type=parse_arg_extra_testcase)

    ap.add_argument(
        '--mut-substrings',
        type=parse_arg_substrings,
        required=False)

    ap.add_argument(
        '--file-prefix',
        default='')

    ap.add_argument(
        '--random-seed',
        type=int,
        required=False,
        metavar='<NUMBER>')

    args = ap.parse_args()

    if args.random_seed is not None:
        random.seed(args.random_seed, version=2)

    g = mod_generator.Generator()

    def _add_choices(chunk_specs: typing.List[ChunkSpec], key: str) -> None:
        for chunk_spec in chunk_specs:
            sheaf = mod_generator.Sheaf(
                chunk_spec.chunk,
                is_homogeneous=chunk_spec.is_homogeneous)
            g.add_sheaf_for_key(
                key=key,
                sheaf=sheaf,
                weight=chunk_spec.weight)

    _add_choices(args.a, key='A')
    _add_choices(args.a_range, key='A')
    _add_choices(args.b, key='B')
    _add_choices(args.b_range, key='B')

    if not (g.has_choices_for_key('A') and g.has_choices_for_key('B')):
        if not g.has_choices_for_key('A'):
            print('Ether "--a" or "--a-range" must be specified.', file=sys.stderr)
        else:
            print('Ether "--b" or "--b-range" must be specified.', file=sys.stderr)
        ap.print_help(sys.stderr)
        sys.exit(2)

    mutators: typing.List[mod_mutator.Mutator] = []
    if args.mut_substrings is not None:
        mutators.append(mod_mutators.SubstringMutator(args.mut_substrings))
    if args.mut_prefix is not None:
        mutators.append(mod_mutators.PrefixMutator(args.mut_prefix))

    max_variants = max((m.get_number_of_variants() for m in mutators), default=-1)
    if max_variants > args.num_files:
        print(
            'Specified mutators have too many variants (%d) for --num-files=%d' % (
                max_variants, args.num_files
            ),
            file=sys.stderr)
        sys.exit(2)

    def _gen_filename(suffix: str) -> str:
        return f'{args.dest_dir}/{args.file_prefix}testcase_{suffix}'

    length_min, length_max = args.length

    if args.a_is_important:
        whole_length = g.get_whole_length_for_key('A')
        if length_min < whole_length:
            print(
                'Minimal length (%d) is less than number of whole bytes in A (%d)' % (
                    length_min, whole_length
                ),
                file=sys.stderr)
            sys.exit(2)

    for i in range(args.num_files):
        with open(_gen_filename(f'{i:03d}'), 'wb') as f:

            length = random.randint(length_min, length_max)
            include_all_of_A = (i == 0) and args.a_is_important

            content = mod_mutator.generate_with_mutators(
                g=g,
                mutators=mutators,
                length=length,
                i=i,
                n=args.num_files,
                include_all_of_A=include_all_of_A)
            f.write(content)

    for file_suffix, content in args.extra_testcase:
        with open(_gen_filename(file_suffix), 'wb') as f:
            f.write(content)


if __name__ == '__main__':
    main()
