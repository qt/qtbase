#!/usr/bin/env python3
# Copyright (C) 2021 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
"""Script to generate C++ code from CLDR data in QLocaleXML form

See ``cldr2qlocalexml.py`` for how to generate the QLocaleXML data itself.
Pass the output file from that as first parameter to this script; pass the ISO
639-3 data file as second parameter. You can optionally pass the root of the
qtbase check-out as third parameter; it defaults to the root of the qtbase
check-out containing this script.

The ISO 639-3 data file can be downloaded from the SIL website:

    https://iso639-3.sil.org/sites/iso639-3/files/downloads/iso-639-3.tab
"""

import datetime
import argparse
from pathlib import Path
from typing import Callable, Iterator, Optional, TextIO

from qlocalexml import Locale, QLocaleXmlReader
from localetools import *
from iso639_3 import LanguageCodeData, LanguageCodeEntry
from zonedata import utcIdList, windowsIdList


# Sanity check the zone data:

# Offsets of the windows tables, in minutes, where whole numbers:
winOff = set(m for m, s in (divmod(v, 60) for k, v in windowsIdList) if s == 0)
# The UTC±HH:mm forms of the non-zero offsets:
winUtc = set(f'UTC-{h:02}:{m:02}'
             for h, m in (divmod(-o, 60) for o in winOff if o < 0)
             ).union(f'UTC+{h:02}:{m:02}'
                     for h, m in (divmod(o, 60) for o in winOff if o > 0))
# All such offsets should be represented by entries in utcIdList:
newUtc = winUtc.difference(utcIdList)
assert not newUtc, (
    'Please add missing UTC-offset zones to to zonedata.utcIdList', newUtc)


class LocaleKeySorter:
    """Sort-ordering representation of a locale key.

    This is for passing to a sorting algorithm as key-function, that
    it applies to each entry in the list to decide which belong
    earlier. It adds an entry to the (language, script, territory)
    triple, just before script, that sorts earlier if the territory is
    the default for the given language and script, later otherwise.
    """

    # TODO: study the relationship between this and CLDR's likely
    # sub-tags algorithm. Work out how locale sort-order impacts
    # QLocale's likely sub-tag matching algorithms. Make sure this is
    # sorting in an order compatible with those algorithms.

    def __init__(self, defaults: Iterator[tuple[tuple[int, int], int]]) -> None:
        self.map: dict[tuple[int, int], int] = dict(defaults)
    def foreign(self, key: tuple[int, int, int]) -> bool:
        default: int | None = self.map.get(key[:2])
        return default is None or default != key[2]
    def __call__(self, key: tuple[int, int, int]) -> tuple[int, bool, int, int]:
        # TODO: should we compare territory before or after script ?
        return (key[0], self.foreign(key)) + key[1:]

class ByteArrayData:
    # Only for use with ASCII data, e.g. IANA IDs.
    def __init__(self) -> None:
        self.data: list[str] = []
        self.hash: dict[str, int] = {}

    def append(self, s: str) -> int:
        assert s.isascii(), s
        s += '\0'
        if s in self.hash:
            return self.hash[s]

        index: int = len(self.data)
        if index > 0xffff:
            raise Error(f'Index ({index}) outside the uint16 range !')
        self.hash[s] = index
        self.data += unicode2hex(s)
        return index

    def write(self, out: Callable[[str], int], name: str) -> None:
        out(f'\nstatic inline constexpr char {name}[] = {{\n')
        out(wrap_list(self.data, 16)) # 16 == 100 // len('0xhh, ')
        # All data is ASCII, so only two-digit hex is ever needed.
        out('\n};\n')

class StringDataToken:
    def __init__(self, index: int, length: int, lenbits: int, indbits: int) -> None:
        if index >= (1 << indbits):
            raise ValueError(f'Start-index ({index}) exceeds the {indbits}-bit range!')
        if length >= (1 << lenbits):
            raise ValueError(f'Data size ({length}) exceeds the {lenbits}-bit range!')

        self.index = index
        self.length = length

class StringData:
    def __init__(self, name: str, lenbits: int = 8, indbits: int = 16) -> None:
        self.data: list[str] = []
        self.hash: dict[str, StringDataToken] = {}
        self.name = name
        self.text = '' # Used in quick-search for matches in data
        self.__bits: tuple[int, int] = lenbits, indbits

    def append(self, s: str) -> StringDataToken:
        try:
            token: StringDataToken = self.hash[s]
        except KeyError:
            token: StringDataToken = self.__store(s)
            self.hash[s] = token
        return token

    def __store(self, s: str) -> StringDataToken:
        """Add string s to known data.

        Seeks to avoid duplication, where possible.
        For example, short-forms may be prefixes of long-forms.
        """
        if not s:
            return StringDataToken(0, 0, *self.__bits)
        ucs2: list[str] = unicode2hex(s)
        try:
            index: int = self.text.index(s) - 1
            matched = 0
            while matched < len(ucs2):
                index, matched = self.data.index(ucs2[0], index + 1), 1
                if index + len(ucs2) >= len(self.data):
                    raise ValueError # not found after all !
                while matched < len(ucs2) and self.data[index + matched] == ucs2[matched]:
                    matched += 1
        except ValueError:
            index = len(self.data)
            self.data += ucs2
            self.text += s

        assert index >= 0
        try:
            return StringDataToken(index, len(ucs2), *self.__bits)
        except ValueError as e:
            e.args += (self.name, s)
            raise

    def write(self, out: Callable[[str], int]) -> None:
        indbits: int = self.__bits[1]
        if len(self.data) >= (1 << indbits):
            raise ValueError(f'Data is too big ({len(self.data)}) '
                             f'for {indbits}-bit index to its end!',
                             self.name)
        out(f"\nstatic inline constexpr char16_t {self.name}[] = {{\n")
        out(wrap_list(self.data, 12)) # 12 == 100 // len('0xhhhh, ')
        out("\n};\n")

def currencyIsoCodeData(s: str) -> str:
    if s:
        return '{' + ",".join(str(ord(x)) for x in s) + '}'
    return "{0,0,0}"

class LocaleSourceEditor (SourceFileEditor):
    def __init__(self, path: Path, temp: Path, version: str) -> None:
        super().__init__(path, temp)
        self.version = version

    def onEnter(self) -> None:
        super().onEnter()
        self.writer.write(f"""
/*
    This part of the file was generated on {datetime.date.today()} from the
    Common Locale Data Repository v{self.version}

    http://www.unicode.org/cldr/

    Do not edit this section: instead regenerate it using
    cldr2qlocalexml.py and qlocalexml2cpp.py on updated (or
    edited) CLDR data; see qtbase/util/locale_database/.
*/

""")

class TimeZoneDataWriter (LocaleSourceEditor):
    def __init__(self, path: Path, temp: Path, version: str) -> None:
        super().__init__(path, temp, version)
        self.__ianaTable = ByteArrayData() # Single IANA IDs
        self.__ianaListTable = ByteArrayData() # Space-joined lists of IDs
        self.__windowsTable = ByteArrayData() # Windows names for zones
        self.__windowsList = sorted(windowsIdList,
                                    key=lambda p: p[0].lower())
        self.windowsKey = {name: (key, off) for key, (name, off)
                           in enumerate(self.__windowsList, 1)}

    def utcTable(self) -> None:
        offsetMap: dict[int, tuple[str, ...]] = {}
        out: Callable[[str], int] = self.writer.write
        for name in utcIdList:
            offset: int = self.__offsetOf(name)
            offsetMap[offset] = offsetMap.get(offset, ()) + (name,)

        # Write UTC ID key table
        out('// IANA ID Index, UTC Offset\n')
        out('static inline constexpr UtcData utcDataTable[] = {\n')
        for offset in sorted(offsetMap.keys()): # Sort so C++ can binary-chop.
            names: tuple[str, ...] = offsetMap[offset]
            joined: int = self.__ianaListTable.append(' '.join(names))
            out(f'    {{ {joined:6d},{offset:6d} }}, // {names[0]}\n')
        out('};\n')

    def aliasToIana(self, pairs: Iterator[tuple[str, str]]) -> None:
        out: Callable[[str], int] = self.writer.write
        store: Callable[[str], int] = self.__ianaTable.append

        out('// Alias ID Index, Alias ID Index\n')
        out('static inline constexpr AliasData aliasMappingTable[] = {\n')
        for name, iana in pairs: # They're ready-sorted
            if name != iana:
                out(f'    {{ {store(name):6d},{store(iana):6d} }},'
                    f' // {name} -> {iana}\n')
        out('};\n\n')

    def msToIana(self, pairs: Iterator[tuple[str, str]]) -> None:
        out: Callable[[str], int] = self.writer.write
        winStore: Callable[[str], int] = self.__windowsTable.append
        ianaStore: Callable[[str], int] = self.__ianaListTable.append
        alias: dict[str, str] = dict(pairs) # {MS name: IANA ID}

        out('// Windows ID Key, Windows ID Index, IANA ID Index, UTC Offset\n')
        out('static inline constexpr WindowsData windowsDataTable[] = {\n')
        # Sorted by Windows ID key:

        for index, (name, offset) in enumerate(self.__windowsList, 1):
            out(f'    {{ {index:6d},{winStore(name):6d},'
                f'{ianaStore(alias[name]):6d},{offset:6d} }}, // {name}\n')
        out('};\n\n')

    def msLandIanas(self, triples: Iterator[tuple[str, str, str]]) -> None:
        # triples (MS name, territory code, IANA list)
        out: Callable[[str], int] = self.writer.write
        store: Callable[[str], int] = self.__ianaListTable.append
        from enumdata import territory_map
        landKey: dict[str, tuple[int, str]] = {code: (i, name) for i, (name, code)
                                               in territory_map.items()}
        seq: list[tuple[int, int, str, str, str]] = sorted(
            (self.windowsKey[name][0], landKey[land][0], name, landKey[land][1], ianas)
                for name, land, ianas in triples)

        out('// Windows ID Key, Territory Enum, IANA ID Index\n')
        out('static inline constexpr ZoneData zoneDataTable[] = {\n')
        # Sorted by (Windows ID Key, territory enum)
        for winId, landId, name, land, ianas in seq:
            out(f'    {{ {winId:6d},{landId:6d},{store(ianas):6d} }},'
                f' // {name} / {land}\n')
        out('};\n\n')

    def writeTables(self) -> None:
        self.__windowsTable.write(self.writer.write, 'windowsIdData')
        # TODO: these are misnamed, entries in the first are lists,
        # those in the next are single IANA IDs
        self.__ianaListTable.write(self.writer.write, 'ianaIdData')
        self.__ianaTable.write(self.writer.write, 'aliasIdData')

    # Implementation details:
    @staticmethod
    def __offsetOf(utcName: str) -> int:
        "Maps a UTC±HH:mm name to its offset in seconds"
        assert utcName.startswith('UTC')
        if len(utcName) == 3:
            return 0
        assert utcName[3] in '+-', utcName
        sign = -1 if utcName[3] == '-' else 1
        assert len(utcName) == 9 and utcName[6] == ':', utcName
        hour, mins = int(utcName[4:6]), int(utcName[-2:])
        return sign * (hour * 60 + mins) * 60

class LocaleDataWriter (LocaleSourceEditor):
    def likelySubtags(self, likely: Iterator[tuple[str, tuple, str, tuple]]) -> None:
        # First sort likely, so that we can use binary search in C++
        # code. Although the entries are (lang, script, region), sort
        # as (lang, region, script) and sort 0 after all non-zero
        # values. This ensures that, when several mappings partially
        # match a requested locale, the one we should prefer to use
        # appears first.
        huge = 0x10000 # > any ushort; all tag values are ushort
        def keyLikely(entry):
            have = entry[1] # Numeric id triple
            return have[0] or huge, have[2] or huge, have[1] or huge # language, region, script
        likely = sorted(likely, key=keyLikely)

        i = 0
        self.writer.write('static inline constexpr QLocaleId likely_subtags[] = {\n')
        # have and give are both triplets of ints
        for had, have, got, give in likely:
            i += 1
            self.writer.write('    {{ {:3d}, {:3d}, {:3d} }}'.format(*have))
            self.writer.write(', {{ {:3d}, {:3d}, {:3d} }}'.format(*give))
            self.writer.write(' ' if i == len(likely) else ',')
            self.writer.write(f' // {had} -> {got}\n')
        self.writer.write('};\n\n')

    def localeIndex(self, indices: Iterator[tuple[int, str]]) -> None:
        self.writer.write('static inline constexpr quint16 locale_index[] = {\n')
        for index, name in indices:
            self.writer.write(f'{index:6d}, // {name}\n')
        self.writer.write('     0 // trailing 0\n')
        self.writer.write('};\n\n')

    def localeData(self, locales: dict[tuple[int, int, int], Locale],
                   names: list[tuple[int, int, int]]) -> None:
        list_pattern_part_data = StringData('list_pattern_part_data')
        single_character_data = StringData('single_character_data')
        date_format_data = StringData('date_format_data')
        time_format_data = StringData('time_format_data')
        days_data = StringData('days_data')
        am_data = StringData('am_data')
        pm_data = StringData('pm_data')
        byte_unit_data = StringData('byte_unit_data')
        currency_symbol_data = StringData('currency_symbol_data')
        currency_display_name_data = StringData('currency_display_name_data')
        currency_format_data = StringData('currency_format_data')
        endonyms_data = StringData('endonyms_data')

        # Locale data
        self.writer.write('static inline constexpr QLocaleData locale_data[] = {\n')
        # Table headings: keep each label centred in its field, matching line_format:
        self.writer.write('   // '
                          # Width 6 + comma
                          ' lang  ' # IDs
                          'script '
                          '  terr '

                          # Range entries (all start-indices, then all sizes)
                          # Width 5 + comma
                          'lStrt ' # List pattern
                          'lpMid '
                          'lpEnd '
                          'lPair '
                          'lDelm ' # List delimiter
                          # Representing numbers
                          ' dec  '
                          'group '
                          'prcnt '
                          ' zero '
                          'minus '
                          'plus  '
                          ' exp  '
                          # Quotation marks
                          'qtOpn '
                          'qtEnd '
                          'altQO '
                          'altQE '
                          'lDFmt ' # Date format
                          'sDFmt '
                          'lTFmt ' # Time format
                          'sTFmt '
                          'slDay ' # Day names
                          'lDays '
                          'ssDys '
                          'sDays '
                          'snDay '
                          'nDays '
                          '  am  ' # am/pm indicators
                          '  pm  '
                          ' byte '
                          'siQnt '
                          'iecQn '
                          'crSym ' # Currency formatting
                          'crDsp '
                          'crFmt '
                          'crFNg '
                          'ntLng ' # Name of language in itself, and of territory
                          'ntTer '
                          # Width 3 + comma for each size; no header
                          + '    ' * 37 +

                          # Strays (char array, bit-fields):
                          # Width 10 + 2 spaces + comma
                          '   currISO   '
                          # Width 6 + comma
                          'curDgt ' # Currency digits
                          'curRnd ' # Currencty rounding (unused: QTBUG-81343)
                          'dow1st ' # First day of week
                          ' wknd+ ' # Week-end start/end days
                          ' wknd- '
                          'grpTop '
                          'grpMid '
                          'grpEnd'
                          # No trailing space on last entry (be sure to
                          # pad before adding anything after it).
                          '\n')

        formatLine = ''.join((
            '    {{ ',
            # Locale-identifier
            '{:6d},' * 3,
            # List patterns, date/time formats, day names, am/pm
            # SI/IEC byte-unit abbreviations
            # Currency and endonyms
            # Range starts
            '{:5d},' * 37,
            # Range sizes
            '{:3d},' * 37,

            # Currency ISO code
            ' {:>10s}, ',
            # Currency formatting
            '{:6d},{:6d}',
            # Day of week and week-end
            ',{:6d}' * 3,
            # Number group sizes
            ',{:6d}' * 3,
            ' }}')).format
        for key in names:
            locale: Locale = locales[key]
            # Sequence of StringDataToken:
            ranges: tuple[StringDataToken, ...] = (
                      tuple(list_pattern_part_data.append(p) for p in # 5 entries:
                            (locale.listPatternPartStart, locale.listPatternPartMiddle,
                             locale.listPatternPartEnd, locale.listPatternPartTwo,
                             locale.listDelim)) +
                      tuple(single_character_data.append(p) for p in # 11 entries
                            (locale.decimal, locale.group, locale.percent, locale.zero,
                             locale.minus, locale.plus, locale.exp,
                             locale.quotationStart, locale.quotationEnd,
                             locale.alternateQuotationStart, locale.alternateQuotationEnd)) +
                      tuple(date_format_data.append(f) for f in # 2 entries:
                             (locale.longDateFormat, locale.shortDateFormat)) +
                      tuple(time_format_data.append(f) for f in # 2 entries:
                            (locale.longTimeFormat, locale.shortTimeFormat)) +
                      tuple(days_data.append(d) for d in # 6 entries:
                            (locale.standaloneLongDays, locale.longDays,
                             locale.standaloneShortDays, locale.shortDays,
                             locale.standaloneNarrowDays, locale.narrowDays)) +
                      (am_data.append(locale.am), pm_data.append(locale.pm)) + # 2 entries
                      tuple(byte_unit_data.append(b) for b in # 3 entries:
                            (locale.byte_unit,
                             locale.byte_si_quantified,
                             locale.byte_iec_quantified)) +
                      (currency_symbol_data.append(locale.currencySymbol),
                       currency_display_name_data.append(locale.currencyDisplayName),
                       currency_format_data.append(locale.currencyFormat),
                       currency_format_data.append(locale.currencyNegativeFormat),
                       endonyms_data.append(locale.languageEndonym),
                       endonyms_data.append(locale.territoryEndonym)) # 6 entries
                      ) # Total: 37 entries
            assert len(ranges) == 37

            self.writer.write(formatLine(*(
                        key +
                        tuple(r.index for r in ranges) +
                        tuple(r.length for r in ranges) +
                        (currencyIsoCodeData(locale.currencyIsoCode),
                         locale.currencyDigits,
                         locale.currencyRounding, # unused (QTBUG-81343)
                         locale.firstDayOfWeek, locale.weekendStart, locale.weekendEnd,
                         locale.groupTop, locale.groupHigher, locale.groupLeast) ))
                              + f', // {locale.language}/{locale.script}/{locale.territory}\n')
        self.writer.write(formatLine(*( # All zeros, matching the format:
                    (0,) * 3 + (0,) * 37 * 2
                    + (currencyIsoCodeData(0),)
                    + (0,) * 8 ))
                          + ' // trailing zeros\n')
        self.writer.write('};\n')

        # StringData tables:
        for data in (list_pattern_part_data, single_character_data,
                     date_format_data, time_format_data, days_data,
                     byte_unit_data, am_data, pm_data, currency_symbol_data,
                     currency_display_name_data, currency_format_data,
                     endonyms_data):
            data.write(self.writer.write)

    @staticmethod
    def __writeNameData(out, book: dict[int, tuple[str, str, str]], form: str) -> None:
        out(f'static inline constexpr char {form}_name_list[] =\n')
        out('"Default\\0"\n')
        for key, value in book.items():
            if key == 0:
                continue
            enum, name = value[0], value[-1]
            if names_clash(name, enum):
                out(f'"{name}\\0" // {enum}\n')
            else:
                out(f'"{name}\\0"\n') # Automagically utf-8 encoded
        out(';\n\n')

        out(f'static inline constexpr quint16 {form}_name_index[] = {{\n')
        out(f'     0, // Any{form.capitalize()}\n')
        index = 8
        for key, value in book.items():
            if key == 0:
                continue
            out(f'{index:6d}, // {value[0]}\n')
            index += len(value[-1].encode('utf-8')) + 1
        out('};\n\n')

    @staticmethod
    def __writeCodeList(out, book: dict[int, tuple[str, str, str]], form: str, width: int) -> None:
        out(f'static inline constexpr unsigned char {form}_code_list[] =\n')
        for key, value in book.items():
            code = value[1]
            code += r'\0' * max(width - len(code), 0)
            out(f'"{code}" // {value[0]}\n')
        out(';\n\n')

    def languageNames(self, languages: dict[int, tuple[str, str, str]]) -> None:
        self.__writeNameData(self.writer.write, languages, 'language')

    def scriptNames(self, scripts):
        self.__writeNameData(self.writer.write, scripts, 'script')

    def territoryNames(self, territories):
        self.__writeNameData(self.writer.write, territories, 'territory')

    # TODO: unify these next three into the previous three; kept
    # separate for now to verify we're not changing data.

    def languageCodes(self, languages: dict[int, tuple[str, str, str]],
                      code_data: LanguageCodeData) -> None:
        out: Callable[[str], int] = self.writer.write

        out(f'constexpr std::array<LanguageCodeEntry, {len(languages)}> languageCodeList {{\n')

        def q(val: Optional[str], size: int) -> str:
            """Quote the value and adjust the result for tabular view."""
            s: str = '' if val is None else ', '.join(f"'{c}'" for c in val)
            return f'{{{s}}}' if size == 0 else f'{{{s}}},'.ljust(size * 5 + 2)

        for key, value in languages.items():
            code: str = value[1]
            if key < 2:
                result: LanguageCodeEntry = code_data.query('und')
            else:
                result: LanguageCodeEntry = code_data.query(code)
                assert code == result.id()
            assert result is not None

            codeString: str = q(result.part1Code, 2)
            codeString += q(result.part2BCode, 3)
            codeString += q(result.part2TCode, 3)
            codeString += q(result.part3Code, 0)
            out(f'    LanguageCodeEntry {{{codeString}}}, // {value[0]}\n')

        out('};\n\n')

    def scriptCodes(self, scripts: dict[int, tuple[str, str, str]]) -> None:
        self.__writeCodeList(self.writer.write, scripts, 'script', 4)

    # TODO: unify with territoryNames()
    def territoryCodes(self, territories: dict[int, tuple[str, str, str]]) -> None:
        self.__writeCodeList(self.writer.write, territories, 'territory', 3)

class CalendarDataWriter (LocaleSourceEditor):
    formatCalendar = (
        '      {{'
        + ','.join(('{:6d}',) * 3 + ('{:5d}',) * 6 + ('{:3d}',) * 6)
        + ' }},').format
    def write(self, calendar: str, locales: dict[tuple[int, int, int], Locale],
              names: list[tuple[int, int, int]]) -> None:
        months_data = StringData('months_data', 16)

        self.writer.write('static inline constexpr QCalendarLocale locale_data[] = {\n')
        self.writer.write(
            '     //'
            # IDs, width 7 (6 + comma)
            ' lang  '
            ' script'
            ' terr  '
            # Month-name start-indices, width 6 (5 + comma)
            'sLong '
            ' long '
            'sShrt '
            'short '
            'sNarw '
            'narow '
            #  No individual headers for the sizes.
            'Sizes...'
            '\n')
        for key in names:
            locale: Locale = locales[key]
            # Sequence of StringDataToken:
            try:
                # Twelve long month names can add up to more than 256 (e.g. kde_TZ: 264)
                ranges = tuple(months_data.append(m[calendar]) for m in
                               (locale.standaloneLongMonths, locale.longMonths,
                                locale.standaloneShortMonths, locale.shortMonths,
                                locale.standaloneNarrowMonths, locale.narrowMonths))
            except ValueError as e:
                e.args += (locale.language, locale.script, locale.territory)
                raise

            self.writer.write(
                self.formatCalendar(*(
                        key +
                        tuple(r.index for r in ranges) +
                        tuple(r.length for r in ranges) ))
                + f'// {locale.language}/{locale.script}/{locale.territory}\n')
        self.writer.write(self.formatCalendar(*( (0,) * (3 + 6 * 2) ))
                          + '// trailing zeros\n')
        self.writer.write('};\n')
        months_data.write(self.writer.write)


class TestLocaleWriter (LocaleSourceEditor):
    def localeList(self, locales: list[tuple[int, int, int]]) -> None:
        self.writer.write('const LocaleListItem g_locale_list[] = {\n')
        from enumdata import language_map, territory_map
        # TODO: update testlocales/ to include script.
        # For now, only mention each (lang, land) pair once:
        pairs = set((lang, land) for lang, script, land in locales)
        for lang, script, land in locales:
            if (lang, land) in pairs:
                pairs.discard((lang, land))
                langName = language_map[lang][0]
                landName = territory_map[land][0]
                self.writer.write(f'    {{ {lang:6d},{land:6d} }}, // {langName}/{landName}\n')
        self.writer.write('};\n\n')


class LocaleHeaderWriter (SourceFileEditor):
    def __init__(self, path: Path, temp: Path, enumify: Callable[[str, str], str]) -> None:
        super().__init__(path, temp)
        self.__enumify = enumify

    def languages(self, languages: dict[int, tuple[str, str, str]]) -> None:
        self.__enum('Language', languages, self.__language)
        self.writer.write('\n')

    def territories(self, territories: dict[int, tuple[str, str, str]]) -> None:
        self.writer.write("    // ### Qt 7: Rename to Territory\n")
        self.__enum('Country', territories, self.__territory, 'Territory')

    def scripts(self, scripts: dict[int, tuple[str, str, str]]) -> None:
        self.__enum('Script', scripts, self.__script)
        self.writer.write('\n')

    # Implementation details
    from enumdata import (language_aliases as __language,
                          territory_aliases as __territory,
                          script_aliases as __script)

    def __enum(self, name: str, book: dict[int, tuple[str, str, str]],
               alias: dict[str, str], suffix: str = None) -> None:
        assert book

        if suffix is None:
            suffix = name

        out: Callable[[str], int] = self.writer.write
        enumify: Callable[[str, str], str] = self.__enumify
        out(f'    enum {name} : ushort {{\n')
        for key, value in book.items():
            member = enumify(value[0], suffix)
            out(f'        {member} = {key},\n')

        out('\n        '
            + ',\n        '.join(f'{k} = {v}' for k, v in sorted(alias.items()))
            + f',\n\n        Last{suffix} = {member}')

        # for "LastCountry = LastTerritory"
        # ### Qt 7: Remove
        if suffix != name:
            out(f',\n        Last{name} = Last{suffix}')

        out('\n    };\n')


def main(argv: list[str], out: TextIO, err: TextIO) -> int:
    """Updates QLocale's CLDR data from a QLocaleXML file.

    Takes sys.argv, sys.stdout, sys.stderr (or equivalents) as
    arguments. In argv[1:] it expects the QLocaleXML file as first
    parameter and the ISO 639-3 data table as second
    parameter. Accepts the root of the qtbase checkout as third
    parameter (default is inferred from this script's path) and a
    --calendars option to select which calendars to support (all
    available by default).

    Updates various src/corelib/t*/q*_data_p.h files within the qtbase
    checkout to contain data extracted from the QLocaleXML file."""
    calendars_map: dict[str, str] = {
        # CLDR name: Qt file name fragment
        'gregorian': 'roman',
        'persian': 'jalali',
        'islamic': 'hijri',
    }
    all_calendars = list(calendars_map.keys())

    parser = argparse.ArgumentParser(
        prog=Path(argv[0]).name,
        description='Generate C++ code from CLDR data in QLocaleXML form.',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('input_file', help='input XML file name',
                        metavar='input-file.xml')
    parser.add_argument('iso_path', help='path to the ISO 639-3 data file',
                        metavar='iso-639-3.tab')
    parser.add_argument('qtbase_path', help='path to the root of the qtbase source tree',
                        nargs='?', default=qtbase_root)
    parser.add_argument('--calendars', help='select calendars to emit data for',
                        nargs='+', metavar='CALENDAR',
                        choices=all_calendars, default=all_calendars)
    args: argparse.Namespace = parser.parse_args(argv[1:])

    qlocalexml: str = args.input_file
    qtsrcdir = Path(args.qtbase_path)
    calendars: dict[str, str] = {cal: calendars_map[cal] for cal in args.calendars}

    if not (qtsrcdir.is_dir()
            and all(qtsrcdir.joinpath('src/corelib/text', leaf).is_file()
                    for leaf in ('qlocale_data_p.h', 'qlocale.h', 'qlocale.qdoc'))):
        parser.error(f'Missing expected files under qtbase source root {qtsrcdir}')

    reader = QLocaleXmlReader(qlocalexml)
    locale_map = dict(reader.loadLocaleMap(calendars, err.write))
    locale_keys: list[tuple[int, int, int]] = sorted(locale_map.keys(),
                                                     key=LocaleKeySorter(reader.defaultMap()))

    code_data = LanguageCodeData(args.iso_path)

    try:
        with LocaleDataWriter(qtsrcdir.joinpath('src/corelib/text/qlocale_data_p.h'),
                              qtsrcdir, reader.cldrVersion) as writer:
            writer.likelySubtags(reader.likelyMap())
            writer.localeIndex(reader.languageIndices(tuple(k[0] for k in locale_map)))
            writer.localeData(locale_map, locale_keys)
            writer.writer.write('\n')
            writer.languageNames(reader.languages)
            writer.scriptNames(reader.scripts)
            writer.territoryNames(reader.territories)
            # TODO: merge the next three into the previous three
            writer.languageCodes(reader.languages, code_data)
            writer.scriptCodes(reader.scripts)
            writer.territoryCodes(reader.territories)
    except Exception as e:
        err.write(f'\nError updating locale data: {e}\n')
        return 1

    # Generate calendar data
    for calendar, stem in calendars.items():
        try:
            with CalendarDataWriter(
                    qtsrcdir.joinpath(f'src/corelib/time/q{stem}calendar_data_p.h'),
                    qtsrcdir, reader.cldrVersion) as writer:
                writer.write(calendar, locale_map, locale_keys)
        except Exception as e:
            err.write(f'\nError updating {calendar} locale data: {e}\n')

    # qlocale.h
    try:
        with LocaleHeaderWriter(qtsrcdir.joinpath('src/corelib/text/qlocale.h'),
                                qtsrcdir, reader.enumify) as writer:
            writer.languages(reader.languages)
            writer.scripts(reader.scripts)
            writer.territories(reader.territories)
    except Exception as e:
        err.write(f'\nError updating qlocale.h: {e}\n')

    # qlocale.qdoc
    try:
        with Transcriber(qtsrcdir.joinpath('src/corelib/text/qlocale.qdoc'), qtsrcdir) as qdoc:
            DOCSTRING = "    QLocale's data is based on Common Locale Data Repository "
            for line in qdoc.reader:
                if DOCSTRING in line:
                    qdoc.writer.write(f'{DOCSTRING}v{reader.cldrVersion}.\n')
                else:
                    qdoc.writer.write(line)
    except Exception as e:
        err.write(f'\nError updating qlocale.h: {e}\n')
        return 1

    # Locale-independent timezone data
    try:
        with TimeZoneDataWriter(qtsrcdir.joinpath(
                'src/corelib/time/qtimezoneprivate_data_p.h'),
                                qtsrcdir, reader.cldrVersion) as writer:
            writer.aliasToIana(reader.aliasToIana())
            writer.msLandIanas(reader.msLandIanas())
            writer.msToIana(reader.msToIana())
            writer.utcTable()
            writer.writeTables()
    except Exception as e:
        err.write(f'\nError updating qtimezoneprivate_data_p.h: {e}\n')
        return 1

    # ./testlocales/localemodel.cpp
    try:
        path = 'util/locale_database/testlocales/localemodel.cpp'
        with TestLocaleWriter(qtsrcdir.joinpath(path), qtsrcdir,
                              reader.cldrVersion) as test:
            test.localeList(locale_keys)
    except Exception as e:
        err.write(f'\nError updating localemodel.cpp: {e}\n')

    return 0

if __name__ == "__main__":
    import sys
    sys.exit(main(sys.argv, sys.stdout, sys.stderr))
