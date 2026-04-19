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

class WordIsTooLong(BaseException):
    pass


def _wrap_internal(sentences, width, sentence_spacing):
    lines = ['']

    def _add(prefix, suffix):

        to_append = (prefix + suffix) if lines[-1] else suffix
        if len(lines[-1]) + len(to_append) <= width:
            lines[-1] += to_append
        else:
            if len(suffix) > width:
                raise WordIsTooLong(f'word "{suffix}" is too long')
            lines.append(suffix)

    for i, sentence in enumerate(sentences):
        for word in sentence.content.split():
            _add(' ', word)
        if i != len(sentences) - 1:
            if sentence_spacing > 1:
                _add(' ' * (sentence_spacing - 1), '')

    return [line.rstrip() for line in lines]


def wrap_ascii(sentences, width, padding=0, sentence_spacing=1):
    lines = _wrap_internal(
        sentences,
        width=width - padding,
        sentence_spacing=sentence_spacing)
    padding_prefix = ' ' * padding
    return '\n'.join(padding_prefix + line for line in lines)
