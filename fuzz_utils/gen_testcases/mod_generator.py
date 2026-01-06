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


def _clamp(x: int, min_value: int, max_value: int) -> int:
    assert min_value <= max_value
    if x < min_value:
        return min_value
    if x > max_value:
        return max_value
    return x


class Sheaf:
    def __init__(self, b: bytes, is_homogeneous: bool) -> None:
        assert b
        self.b = b
        self.is_homogeneous = is_homogeneous

    def get_length(self) -> int:
        return len(self.b)

    def random_byte(self) -> bytes:
        i = random.randint(0, len(self.b) - 1)
        return self.b[i:i + 1]

    def all_bytes(self) -> typing.Iterator[bytes]:
        for i in range(len(self.b)):
            yield self.b[i:i + 1]


class WeightedSheafList:
    def __init__(self) -> None:
        self.sheaves: typing.List[Sheaf] = []
        self.weights: typing.List[int] = []

    def add_sheaf(self, sheaf: Sheaf, weight: int) -> None:
        assert weight > 0
        self.sheaves.append(sheaf)
        self.weights.append(weight)

    def is_nonempty(self) -> bool:
        return bool(self.sheaves)

    def random_sheaf(self) -> Sheaf:
        assert self.is_nonempty()
        res, = random.choices(self.sheaves, weights=self.weights)
        return res

    def get_whole_length(self) -> int:
        res = 0
        for sheaf in self.sheaves:
            if sheaf.is_homogeneous:
                res += 1
            else:
                res += sheaf.get_length()
        return res

    def make_whole_iterator(self) -> typing.Iterator[bytes]:
        for sheaf in self.sheaves:
            if sheaf.is_homogeneous:
                yield sheaf.random_byte()
            else:
                for byte in sheaf.all_bytes():
                    yield byte
        while True:
            yield self.random_sheaf().random_byte()


class Generator:
    def __init__(self) -> None:
        self.wsls = {'A': WeightedSheafList(), 'B': WeightedSheafList()}

    def add_sheaf_for_key(self, key: str, sheaf: Sheaf, weight: int) -> None:
        self.wsls[key].add_sheaf(sheaf, weight=weight)

    def has_choices_for_key(self, key: str) -> bool:
        return self.wsls[key].is_nonempty()

    def get_whole_length_for_key(self, key: str) -> int:
        return self.wsls[key].get_whole_length()

    def random_bytes(self, length: int, i: int, n: int, include_all_of_A: bool) -> bytes:
        assert length >= 0
        assert n > 0
        assert 0 <= i < n

        ratio = i / (n - 1)

        num_B = _clamp(round(ratio * length), min_value=0, max_value=length)
        num_A = length - num_B

        formula = list('A' * num_A + 'B' * num_B)
        random.shuffle(formula)

        A_iter = None
        if include_all_of_A:
            assert self.wsls['A'].get_whole_length() <= length
            A_iter = self.wsls['A'].make_whole_iterator()

        chunks = []
        for letter in formula:
            if letter == 'A' and A_iter is not None:
                chunks.append(next(A_iter))
            else:
                wsl = self.wsls[letter]
                sheaf = wsl.random_sheaf()
                chunks.append(sheaf.random_byte())
        return b''.join(chunks)
