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

import typing

import mod_generator


class Mutator:
    def mutate(self, b: bytes, variant: int) -> bytes:
        raise NotImplementedError()

    def get_number_of_variants(self) -> int:
        raise NotImplementedError()


def generate_with_mutators(
        g: mod_generator.Generator,
        mutators: typing.List[Mutator],
        length: int,
        i: int,
        n: int,
        include_all_of_A: bool
    ) -> bytes:

    assert n > 0
    assert 0 <= i < n

    res = g.random_bytes(length=length, i=i, n=n, include_all_of_A=include_all_of_A)
    for m in mutators:
        variant = i % m.get_number_of_variants()
        res = m.mutate(res, variant=variant)
    return res
