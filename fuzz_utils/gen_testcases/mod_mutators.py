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

import random
import typing

import mod_mutator


class PrefixMutator(mod_mutator.Mutator):
    def __init__(self, prefix: bytes) -> None:
        self.prefix = prefix

    def mutate(self, b: bytes, variant: int) -> bytes:
        return self.prefix[:variant] + b

    def get_number_of_variants(self) -> int:
        return len(self.prefix) + 1


class SubstringMutator(mod_mutator.Mutator):
    def __init__(self, substr_variants: typing.List[bytes]) -> None:
        self.substr_variants = substr_variants

    def mutate(self, b: bytes, variant: int) -> bytes:
        pos = random.randint(0, len(b))
        substr = self.substr_variants[variant]
        return b[:pos] + substr + b[pos:]

    def get_number_of_variants(self) -> int:
        return len(self.substr_variants)
