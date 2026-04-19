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

class AbstractDependency:
    pass


class NormalDependency(AbstractDependency):
    def __init__(self, generic_name, is_shared_library, is_dev):
        self.generic_name = generic_name
        self.is_shared_library = is_shared_library
        self.is_dev = is_dev


class AnyOfDependency(AbstractDependency):
    def __init__(self, normal_deps):
        self.normal_deps = normal_deps


class Package:
    def __init__(self, kind, name):
        self.kind = kind
        self.name = name
        self.goes_into_main_pkg = False
        self.is_source_only = False
        self.title = None
        self.description = None
        self.based_on_plugin = None
        self.suggests = []
        self.depends = []

    def is_valid(self):
        if self.goes_into_main_pkg:
            return True
        if (self.title is not None) and (self.description is not None):
            return True
        return False

    def set_goes_into_main_pkg(self, goes_into_main_pkg):
        self.goes_into_main_pkg = goes_into_main_pkg

    def set_is_source_only(self, is_source_only):
        self.is_source_only = is_source_only

    def set_title(self, title):
        self.title = title

    def set_description(self, description):
        self.description = description

    def set_based_on_plugin(self, based_on_plugin):
        self.based_on_plugin = based_on_plugin

    def add_suggestion(self, dep):
        self.suggests.append(dep)

    def add_dependency(self, dep):
        self.depends.append(dep)


class Sentence:
    def __init__(self, content):
        self.content = content


class Paragraph:
    def __init__(self, sentences):
        self.sentences = sentences


class Description:
    def __init__(self, paragraphs):
        self.paragraphs = paragraphs


class SentenceBuilder:
    def __init__(self):
        self.chunks = []

    def add_chunk(self, chunk):
        chunk = chunk.strip()
        if not chunk:
            return
        self.chunks.append(chunk)

    def finalize(self):
        result = ' '.join(self.chunks)
        if result:
            return Sentence(result)
        else:
            return None


class ParagraphBuilder:
    def __init__(self):
        self.sbs = [SentenceBuilder()]

    def flush_sentence(self):
        self.sbs.append(SentenceBuilder())

    def add_sentence_chunk(self, chunk):
        self.sbs[-1].add_chunk(chunk)

    def finalize(self):
        result = []
        for sb in self.sbs:
            sentence = sb.finalize()
            if sentence is not None:
                result.append(sentence)
        if result:
            return Paragraph(result)
        else:
            return None


class DescriptionBuilder:
    def __init__(self):
        self.pbs = [ParagraphBuilder()]

    def flush_sentence(self):
        self.pbs[-1].flush_sentence()

    def flush_paragraph(self):
        self.pbs.append(ParagraphBuilder())

    def add_sentence_chunk(self, chunk):
        self.pbs[-1].add_sentence_chunk(chunk)

    def finalize(self):
        result = []
        for pb in self.pbs:
            paragraph = pb.finalize()
            if paragraph is not None:
                result.append(paragraph)
        return Description(result)
