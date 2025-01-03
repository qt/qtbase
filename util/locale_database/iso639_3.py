# Copyright (C) 2021 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

from dataclasses import dataclass
from typing import Dict, Optional


@dataclass
class LanguageCodeEntry:
    part3Code: str
    part2BCode: Optional[str]
    part2TCode: Optional[str]
    part1Code: Optional[str]

    def id(self) -> str:
        if self.part1Code:
            return self.part1Code
        if self.part2BCode:
            return self.part2BCode
        return self.part3Code

    def __repr__(self) -> str:
        parts = [f'{self.__class__.__name__}({self.id()!r}, part3Code={self.part3Code!r}']
        if self.part2BCode is not None and self.part2BCode != self.part3Code:
            parts.append(f', part2BCode={self.part2BCode!r}')
            if self.part2TCode != self.part2BCode:
                parts.append(f', part2TCode={self.part2TCode!r}')
        if self.part1Code is not None:
            parts.append(f', part1Code={self.part1Code!r}')
        parts.append(')')
        return ''.join(parts)


class LanguageCodeData:
    """
    Representation of ISO639-2 language code data.
    """
    def __init__(self, fileName: str):
        """
        Construct the object populating the data from the given file.
        """
        self.__codeMap: Dict[str, LanguageCodeEntry] = {}

        with open(fileName, 'r', encoding='utf-8') as stream:
            stream.readline() # skip the header
            for line in stream.readlines():
                part3Code, part2BCode, part2TCode, part1Code, _ = line.split('\t', 4)

                # sanity checks
                assert all(p.isascii() for p in (part3Code, part2BCode, part2TCode, part1Code)), \
                    f'Non-ascii characters in code names: {part3Code!r} {part2BCode!r} '\
                        f'{part2TCode!r} {part1Code!r}'

                assert len(part3Code) == 3, f'Invalid Part 3 code length for {part3Code!r}'
                assert not part1Code or len(part1Code) == 2, \
                    f'Invalid Part 1 code length for {part3Code!r}: {part1Code!r}'
                assert not part2BCode or len(part2BCode) == 3, \
                    f'Invalid Part 2B code length for {part3Code!r}: {part2BCode!r}'
                assert not part2TCode or len(part2TCode) == 3, \
                    f'Invalid Part 2T code length for {part3Code!r}: {part2TCode!r}'

                assert (part2BCode == '') == (part2TCode == ''), \
                    f'Only one Part 2 code is specified for {part3Code!r}: ' \
                    f'{part2BCode!r} vs {part2TCode!r}'
                assert not part2TCode or part2TCode == part3Code, \
                    f'Part 3 code {part3Code!r} does not match Part 2T code {part2TCode!r}'

                entry = LanguageCodeEntry(part3Code, part2BCode or None,
                    part2TCode or None, part1Code or None)

                self.__codeMap[entry.id()] = entry

    def query(self, code: str) -> Optional[LanguageCodeEntry]:
        """
        Lookup the entry with the given code and return it.

        The entries can be looked up by using either the Alpha2 code or the bibliographical
        Alpha3 code.
        """
        return self.__codeMap.get(code)
