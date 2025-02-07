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
from typing import AnyStr, Callable, Iterator, Optional, TextIO

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

    def lookup(self, s: str) -> int:
        return self.append(s, False)

    def append(self, s: str, create: bool = True) -> int:
        assert s.isascii(), s
        s += '\0'
        if s in self.hash:
            return self.hash[s]
        if not create:
            raise Error(f'Entry "{s[:-1]}" missing from reused table !')

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

# Would tables benefit from pre-population, one script at a time ?
# That might improve the chances of match-ups in store.
class StringData:
    def __init__(self, name: str, lenbits: int = 8, indbits: int = 16) -> None:
        self.data: list[str] = []
        self.hash: dict[str, StringDataToken] = {}
        self.name = name
        self.text = '' # Used in quick-search for matches in data
        self.__bits: tuple[int, int] = lenbits, indbits

    def end(self) -> StringDataToken:
        return StringDataToken(len(self.data), 0, *self.__bits)

    def append(self, s: str) -> StringDataToken:
        try:
            token: StringDataToken = self.hash[s]
        except KeyError:
            token: StringDataToken = self.__store(s)
            self.hash[s] = token
        return token

    # The longMetaZoneName table grows to c. 0xe061c bytes, making the
    # searching here rather expensive.
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
        self.__metaIdData = ByteArrayData() # Metazone names

        self.__windowsList: list[tuple[str, int]] = sorted(windowsIdList,
                                                           key=lambda p: p[0].lower())
        self.windowsKey: dict[str, tuple[int, int]] = {name: (key, off) for key, (name, off)
                                                       in enumerate(self.__windowsList, 1)}

        from enumdata import territory_map
        self.__landKey: dict[str, tuple[int, str]] = {code: (i, name) for i, (name, code)
                                                      in territory_map.items()}

    def utcTable(self) -> None:
        offsetMap: dict[int, tuple[str, ...]] = {}
        out: Callable[[str], int] = self.writer.write
        for name in utcIdList:
            offset: int = self.__offsetOf(name)
            offsetMap[offset] = offsetMap.get(offset, ()) + (name,)

        # Write UTC ID key table
        out('\n// IANA List Index, UTC Offset\n')
        out('static inline constexpr UtcData utcDataTable[] = {\n')
        for offset in sorted(offsetMap.keys()): # Sort so C++ can binary-chop.
            names: tuple[str, ...] = offsetMap[offset]
            joined: int = self.__ianaListTable.append(' '.join(names))
            out(f'    {{ {joined:6d},{offset:6d} }}, // {names[0]}\n')
        out('};\n')

    def aliasToIana(self, pairs: Iterator[tuple[str, str]]) -> None:
        out: Callable[[str], int] = self.writer.write
        store: Callable[[str], int] = self.__ianaTable.append

        out('// IANA ID indices of alias and IANA ID\n')
        out('static inline constexpr AliasData aliasMappingTable[] = {\n')
        for name, iana in pairs: # They're ready-sorted
            assert name != iana, (name, iana) # Filtered out in QLocaleXmlWriter
            out(f'    {{ {store(name):6d},{store(iana):6d} }},'
                f' // {name} -> {iana}\n')
        out('};\n\n')

    def territoryZone(self, pairs: Iterator[tuple[str, str]]) -> None:
        self.__beginNonIcuFeatureTZL()

        out: Callable[[str], int] = self.writer.write
        store: Callable[[str], int] = self.__ianaTable.append
        landKey: dict[str, tuple[int, str]] = self.__landKey
        seq: list[tuple[int, str, str]] = sorted((landKey[code][0], iana, landKey[code][1])
                                                 for code, iana in pairs)
        # Write territory-to-zone table
        out('\n// QLocale::Territory value, IANA ID Index\n')
        out('static inline constexpr TerritoryZone territoryZoneMap[] = {\n')
        # Sorted by QLocale::Territory value:
        for land, iana, terra in seq:
            out(f'    {{ {land:6d},{store(iana):6d} }}, // {terra} -> {iana}\n')
        out('};\n')

        # self.__endNonIcuFeatureTZL()

    def metaLandZone(self, quads: Iterator[tuple[str, int, str, str]]) -> None:
        # self.__beginNonIcuFeatureTZL()
        out: Callable[[str], int] = self.writer.write
        metaStore: Callable[[str], int] = self.__metaIdData.append
        ianaStore: Callable[[str], int] = self.__ianaTable.append
        landKey: dict[str, tuple[int, str]] = self.__landKey
        seq: list[tuple[int, int, str, str, str]] = sorted(
            (metaKey, landKey[land][0], meta, landKey[land][1], iana)
            for meta, metaKey, land, iana in quads)

        # Write (metazone, territory, zone) table
        out('\n// MetaZone Key, MetaZone Name Index, '
            'QLocale::Territory value, IANA ID Index\n')
        out('static inline constexpr MetaZoneData metaZoneTable[] = {\n')
        # Sorted by metaKey, then by QLocale::Territory within each metazone:
        for mkey, land, meta, terra, iana in seq:
            out(f'    {{ {mkey:6d},{metaStore(meta):6d},{land:6d},{ianaStore(iana):6d} }},'
                f' // {meta}/{terra} -> {iana}\n')
        out('};\n')

        # self.__endNonIcuFeatureTZL()

    def zoneMetaStory(self, quads: Iterator[tuple[str, int, int, int]]) -> None:
        # self.__beginNonIcuFeatureTZL()

        out: Callable[[str], int] = self.writer.write
        store: Callable[[str], int] = self.__ianaTable.append

        # Write (zone, metazone key, begin, end) table:
        out('\n// IANA ID Index, MetaZone Key, interval start, end\n')
        out('static inline constexpr ZoneMetaHistory zoneHistoryTable[] = {\n')
        # Sorted by IANA ID; each story comes pre-sorted on (start, stop)
        for iana, start, stop, mkey in quads:
            out(f'    {{ {store(iana):6d},{mkey:6d},{start:10d},{stop:10d} }},\n')
        out('};\n')

        self.__endNonIcuFeatureTZL()

    def msToIana(self, pairs: Iterator[tuple[str, str]]) -> None:
        out: Callable[[str], int] = self.writer.write
        winStore: Callable[[str], int] = self.__windowsTable.append
        ianaStore: Callable[[str], int] = self.__ianaTable.append
        alias: dict[str, str] = dict(pairs) # {MS name: IANA ID}
        assert all(not any(x.isspace() for x in iana) for iana in alias.values())

        out('\n// Windows ID Key, Windows ID Index, IANA ID Index, UTC Offset\n')
        out('static inline constexpr WindowsData windowsDataTable[] = {\n')
        # Sorted by Windows ID key:

        for index, (name, offset) in enumerate(self.__windowsList, 1):
            out(f'    {{ {index:6d},{winStore(name):6d},'
                f'{ianaStore(alias[name]):6d},{offset:6d} }}, // {name}\n')
        out('};\n')

    def msLandIanas(self, triples: Iterator[tuple[str, str, str]]) -> None:
        # triples (MS name, territory code, IANA list)
        out: Callable[[str], int] = self.writer.write
        store: Callable[[str], int] = self.__ianaListTable.append
        landKey: dict[str, tuple[int, str]] = self.__landKey
        seq: list[tuple[int, int, str, str, str]] = sorted(
            (self.windowsKey[name][0], landKey[land][0], name, landKey[land][1], ianas)
                for name, land, ianas in triples)

        out('// Windows ID Key, Territory Enum, IANA List Index\n')
        out('static inline constexpr ZoneData zoneDataTable[] = {\n')
        # Sorted by (Windows ID Key, territory enum)
        for winId, landId, name, land, ianas in seq:
            out(f'    {{ {winId:6d},{landId:6d},{store(ianas):6d} }},'
                f' // {name} / {land}\n')
        out('};\n')

    def nameTables(self, locales: Iterator[Locale]) -> tuple[ByteArrayData, ByteArrayData]:
        """Ensure all zone and metazone names used by locales are known

        Must call before writeTables(), to ensure zone and metazone naming
        include all entries, rather than only those implicated in the
        locale-independent data. Returns the ByteArrayData() objects for IANA
        ID and metazone names, whose lookup() can be used to map names to table
        indices when writing the locale-dependent data.

        Because the locale-dependent additions made here happen after the
        carefully ordered entries from the locale-independent data (generated
        along with their sorted tables), the added entries don't follow the
        same sort-order. Fortunately LocaleZoneDataWriter's tables sorted by
        metaIdKey are OK with using the key regardless of the lexical ordering
        of the IDs, and the IANA-sorted things can be sorted on the actual IANA
        ID, rather than index in the (now haphazardly sorted) table."""
        for locale in locales:
            for k in locale.zoneNaming.keys():
                self.__ianaTable.append(k)
            for k in locale.metaNaming.keys():
                self.__metaIdData.append(k)
        return self.__ianaTable, self.__metaIdData

    def writeTables(self) -> None:
        self.__windowsTable.write(self.writer.write, 'windowsIdData')
        self.__ianaListTable.write(self.writer.write, 'ianaListData')
        self.__ianaTable.write(self.writer.write, 'ianaIdData')
        self.__beginNonIcuFeatureTZL()
        self.__metaIdData.write(self.writer.write, 'metaIdData')
        self.__endNonIcuFeatureTZL()
        self.writer.write('\n')

    # Implementation details:
    def __beginNonIcuFeatureTZL(self) -> None:
        self.writer.write('\n#if QT_CONFIG(timezone_locale) && !QT_CONFIG(icu)\n')
    def __endNonIcuFeatureTZL(self) -> None:
        self.writer.write('\n#endif // timezone_locale but not ICU\n')

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


class LocaleZoneDataWriter (LocaleSourceEditor):
    def __init__(self, path: Path, temp: Path, version: str,
                 ianaNames: ByteArrayData, metaNames: ByteArrayData) -> None:
        super().__init__(path, temp, version)
        self.__iana = ianaNames
        self.__meta = metaNames
        self.__hourFormatTable = StringData('hourFormatTable') # "±HH:mm" Some have single H.
        self.__gmtFormatTable = StringData('gmtFormatTable') # "GMT%0"
        # Could be split in three - generic, standard, daylight-saving - if too big:
        self.__regionFormatTable = StringData('regionFormatTable') # "%0 (Summer|Standard)? Time"
        self.__fallbackFormatTable = StringData('fallbackFormatTable')
        self.__exemplarCityTable = StringData('exemplarCityTable', indbits = 32)
        self.__shortZoneNameTable = StringData('shortZoneNameTable')
        self.__longZoneNameTable = StringData('longZoneNameTable')
        self.__shortMetaZoneNameTable = StringData('shortMetaZoneNameTable')
        # Would splitting this table up by (gen, std, dst) miss
        # chances to avoid duplication ? It should speed append().
        self.__longMetaZoneNameTable = StringData('longMetaZoneNameTable', indbits = 32)



    def localeData(self, locales: dict[tuple[int, int, int], Locale],
                   names: list[tuple[int, int, int]]) -> None:
        assert len(names) == len(locales), 'Names should just be a sorted list of locale.keys()'
        # Tables need a terminal row whose localeIndex is len(names) for the
        # sake of an assertion in QTZL.cpp's findTableEntryFor(); the end() of
        # each locale's range of rows must be a valid row.
        out: Callable[[str], int] = self.writer.write

        out('// Sorted by locale index, then iana name\n')
        out('static inline constexpr LocaleZoneExemplar localeZoneExemplarTable[] = {\n')
        out('    // locInd, ianaInd, xcty{ind, sz}\n')
        store: Callable[[str], StringDataToken] = self.__exemplarCityTable.append
        formatLine: Callable[[*tuple[int, ...]], str] = ''.join((
            '    {{ ',
            '{:4d},{:5d},', # Sort keys
            '{:8d},{:3d},', # Index and size
            ' }}')).format
        index = 0
        for locInd, key in enumerate(names):
            locale: Locale = locales[key]
            locale.exemplarStart = index
            for name, data in sorted(locale.zoneNaming.items()):
                eg: StringDataToken = store(data.get('exemplarCity', ''))
                if not eg.length:
                    continue # No exemplar city given, skip this row.
                out(formatLine(locInd, self.__iana.lookup(name), eg.index, eg.length)
                    + (f', // {name} {locale.language}/{locale.script}/{locale.territory}\n'
                       if index == locale.exemplarStart else f', // {name}\n')
                    )
                index += 1
        out(formatLine(len(names), 0, 0, 0) + ' // Terminal row\n'
            + '}; // Exemplar city table\n')
        if index >= (1 << 32):
            raise Error(f'Exemplar table has too many ({index}) entries')
        exemplarRowCount: int = index

        out('\n// Sorted by locale index, then iana name\n')
        out('static inline constexpr LocaleZoneNames localeZoneNameTable[] = {\n')
        out('    // locInd, ianaInd, (lngGen, lngStd, lngDst,'
            ' srtGen, srtStd, srtDst){ind, sz}\n')
        longStore: Callable[[str], StringDataToken] = self.__longZoneNameTable.append
        shortStore: Callable[[str], StringDataToken] = self.__shortZoneNameTable.append
        formatLine = ''.join((
            '    {{ ',
            '{:4d},{:5d},', # Sort keys
            '{:8d},' * 3, '{:5d},' * 3, # Range starts
            '{:3d},' * 6, # Range sizes
            ' }}')).format
        index = 0
        for locInd, key in enumerate(names):
            locale = locales[key]
            locale.zoneStart = index
            for name, data in sorted(locale.zoneNaming.items()):
                ranges: tuple[StringDataToken, ...] = ( tuple(longStore(z)
                                 for z in data.get('long', ('', '', '')))
                           + tuple(shortStore(z)
                                   for z in data.get('short', ('', '', '')))
                          ) # 6 entries; 3 32-bit, 3 16-bit
                if not any(r.length for r in ranges):
                    continue # No names given, don't generate a row
                out(formatLine(*((locInd, self.__iana.lookup(name))
                                 + tuple(r.index for r in ranges)
                                 + tuple(r.length for r in ranges)
                                 ))
                    + (f', // {name} {locale.language}/{locale.script}/{locale.territory}\n'
                       if index == locale.zoneStart else f', // {name}\n')
                    )
                index += 1
        out(formatLine(*((len(names), 0) + (0, 0) * 6)) + ' // Terminal row\n'
            + '}; // Zone naming table\n')
        if index >= (1 << 16):
            raise Error(f'Zone naming table has too many ({index}) entries')
        localeNameCount: int = index

        # Only a small proportion (about 1 in 18) of metazones have short
        # names, so splitting their names across two tables keeps the (many)
        # rows of the long name table short, making the duplication of sort
        # keys in the (much smaller) short name table still work out as a
        # saving.

        out('\n// Sorted by locale index, then meta key\n')
        out('static inline constexpr LocaleMetaZoneLongNames localeMetaZoneLongNameTable[] = {\n')
        out('    // locInd, metaKey, (generic, standard, DST){ind, sz}\n')
        store = self.__longMetaZoneNameTable.append
        formatLine = ''.join((
            '    {{ ',
            '{:4d},{:5d},', # Sort keys
            '{:8d},' * 3, # Range starts
            '{:3d},' * 3, # Range sizes
            ' }}')).format
        index = 0
        for locInd, key in enumerate(names):
            locale = locales[key]
            locale.metaZoneLongStart = index
            # Map metazone names to indices in metazone table:
            for key, meta, data in sorted(
                    (self.__meta.lookup(k), k, v)
                    for k, v in locale.metaNaming.items()):
                ranges: tuple[StringDataToken, ...] = tuple(store(z)
                               for z in data.get('long', ('', '', ''))
                               ) # 3 entries, all 32-bit
                if not any(r.length for r in ranges):
                    continue # No names given, don't generate a row
                out(formatLine(*((locInd, key)
                                 + tuple(r.index for r in ranges)
                                 + tuple(r.length for r in ranges)
                                 ))
                    + (f', // {meta} {locale.language}/{locale.script}/{locale.territory}\n'
                       if index == locale.metaZoneLongStart else f', // {meta}\n')
                    )
                index += 1
        out(formatLine(*((len(names), 0) + (0, 0) * 3)) + ' // Terminal row\n'
            + '}; // Metazone long name table\n')
        if index >= (1 << 32):
            raise Error(f'Metazone long name table has too many ({index}) entries')
        metaLongCount: int = index

        out('\n// Sorted by locale index, then meta key\n')
        out('static inline constexpr LocaleMetaZoneShortNames localeMetaZoneShortNameTable[] = {\n')
        out('    // locInd, metaKey, (generic, standard, DST){ind, sz}\n')
        store = self.__shortMetaZoneNameTable.append
        formatLine = ''.join((
            '    {{ ',
            '{:4d},{:5d},', # Sort keys
            '{:5d},' * 3, # Range starts
            '{:3d},' * 3, # Range sizes
            ' }}')).format
        index = 0
        for locInd, key in enumerate(names):
            locale = locales[key]
            locale.metaZoneShortStart = index
            # Map metazone names to indices in metazone table:
            for key, meta, data in sorted(
                    (self.__meta.lookup(k), k, v)
                    for k, v in locale.metaNaming.items()):
                ranges: tuple[StringDataToken, ...] = tuple(store(z)
                               for z in data.get('short', ('', '', ''))
                               ) # 3 entries, all 16-bit
                if not any(r.length for r in ranges):
                    continue # No names given, don't generate a row
                out(formatLine(*((locInd, key)
                                 + tuple(r.index for r in ranges)
                                 + tuple(r.length for r in ranges)
                                 ))
                    + (f', // {meta} {locale.language}/{locale.script}/{locale.territory}\n'
                       if index == locale.metaZoneShortStart else f', // {meta}\n')
                    )
                index += 1
        out(formatLine(*((len(names), 0) + (0, 0) * 3)) + ' // Terminal Row\n'
            + '}; // Metazone short name table\n')
        if index >= (1 << 16):
            raise Error(f'Metazone short name table has too many ({index}) entries')
        metaShortCount: int = index

        out('\n// Indexing matches that of locale_data in qlocale_data_p.h\n')
        out('static inline constexpr LocaleZoneData localeZoneData[] = {\n')
        out('    // LOCALE_TAGS(lng,scp,ter) xct1st, zn1st, ml1st, ms1st, '
            '(+hr, -hr, gmt, flbk, rgen, rstd, rdst){ind,sz}\n')
        hour: StringData = self.__hourFormatTable
        gmt: StringData = self.__gmtFormatTable
        region: StringData = self.__regionFormatTable
        fall: StringData = self.__fallbackFormatTable
        formatLine = ''.join((
            '    {{ ',
            'LOCALE_TAGS({:3d},{:3d},{:3d})', # key: language, script, territory
            '{:6d},' * 4, # exemplarStart, metaZone{Long,Short}Start, zoneStart
            '{:5d},' * 7, # Range starts
            '{:3d},' * 7, # Range sizes
            ' }}')).format
        for key in names:
            locale = locales[key]
            ranges: tuple[StringDataToken, ...] = (
                (hour.append(locale.positiveOffsetFormat),
                 hour.append(locale.negativeOffsetFormat),
                 gmt.append(locale.gmtOffsetFormat),
                 fall.append(locale.fallbackZoneFormat))
                + tuple(region.append(r) for r in locale.regionFormats)
                ) # 7 entries
            out(formatLine(*(
                key
                + (locale.exemplarStart, locale.metaZoneLongStart,
                   locale.metaZoneShortStart, locale.zoneStart)
                + tuple(r.index for r in ranges)
                + tuple(r.length for r in ranges)
            ))
                + f', // {locale.language}/{locale.script}/{locale.territory}\n')
        ranges: tuple[StringDataToken, ...] = 2 * (hour.end(),) + (
            gmt.end(), fall.end()) + 3 * (region.end(),)
        out(formatLine(*(
            (0, 0, 0, exemplarRowCount, metaLongCount,
             metaShortCount, localeNameCount)
            + tuple(r.index for r in ranges)
            + tuple(r.length for r in ranges)
        ))
            + ' // Terminal row\n')
        out('}; // Locale/zone data\n')

    def writeTables(self) -> None:
        for data in (self.__hourFormatTable,
                     self.__gmtFormatTable,
                     self.__regionFormatTable,
                     self.__fallbackFormatTable,
                     self.__exemplarCityTable,
                     self.__shortZoneNameTable,
                     self.__longZoneNameTable,
                     self.__shortMetaZoneNameTable,
                     self.__longMetaZoneNameTable):
            data.write(self.writer.write)
        self.writer.write('\n')


class LocaleDataWriter (LocaleSourceEditor):
    def likelySubtags(self, likely: Iterator[tuple[str, tuple, str, tuple]]) -> None:
        # Sort order of likely is taken care of upstream.
        self.writer.write('static inline constexpr QLocaleId likely_subtags[] = {\n')
        # have and give are both triplets of ints
        for had, have, got, give in likely:
            self.writer.write('    {{ {:3d}, {:3d}, {:3d} }}'.format(*have))
            self.writer.write(', {{ {:3d}, {:3d}, {:3d} }},'.format(*give))
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

    def languageNaming(self, languages: dict[int, tuple[str, str, str]],
                       code_data: LanguageCodeData) -> None:
        self.__writeNameData(self.writer.write, languages, 'language')
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

    def scriptNaming(self, scripts: dict[int, tuple[str, str, str]]) -> None:
        self.__writeNameData(self.writer.write, scripts, 'script')
        self.__writeCodeList(self.writer.write, scripts, 'script', 4)

    def territoryNaming(self, territories: dict[int, tuple[str, str, str]]) -> None:
        self.__writeNameData(self.writer.write, territories, 'territory')
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
    parser.add_argument('-v', '--verbose', help='more verbose output',
                        action='count', default=0)
    parser.add_argument('-q', '--quiet', help='less output',
                        dest='verbose', action='store_const', const=-1)
    args: argparse.Namespace = parser.parse_args(argv[1:])
    mutter: Callable[[AnyStr], int] = (lambda *x: 0) if args.verbose < 0 else out.write

    qlocalexml: str = args.input_file
    qtsrcdir = Path(args.qtbase_path)
    calendars: dict[str, str] = {cal: calendars_map[cal] for cal in args.calendars}

    if not (qtsrcdir.is_dir()
            and all(qtsrcdir.joinpath('src/corelib/text', leaf).is_file()
                    for leaf in ('qlocale_data_p.h', 'qlocale.h', 'qlocale.qdoc'))):
        parser.error(f'Missing expected files under qtbase source root {qtsrcdir}')

    reader = QLocaleXmlReader(qlocalexml)
    locale_map = dict(reader.loadLocaleMap(calendars, err.write))
    reader.pruneZoneNaming(locale_map, mutter)
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
            writer.languageNaming(reader.languages, code_data)
            writer.scriptNaming(reader.scripts)
            writer.territoryNaming(reader.territories)
    except Exception as e:
        err.write(f'\nError updating locale data: {e}\n')
        if args.verbose > 0:
            raise
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
            if args.verbose > 0:
                raise
            return 1

    # qlocale.h
    try:
        with LocaleHeaderWriter(qtsrcdir.joinpath('src/corelib/text/qlocale.h'),
                                qtsrcdir, reader.enumify) as writer:
            writer.languages(reader.languages)
            writer.scripts(reader.scripts)
            writer.territories(reader.territories)
    except Exception as e:
        err.write(f'\nError updating qlocale.h: {e}\n')
        if args.verbose > 0:
            raise
        return 1

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
        if args.verbose > 0:
            raise
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
            writer.territoryZone(reader.territoryZone())
            writer.metaLandZone(reader.metaLandZone())
            writer.zoneMetaStory(reader.zoneMetaStory())
            ianaNames, metaNames = writer.nameTables(locale_map.values())
            writer.writeTables()
    except Exception as e:
        err.write(f'\nError updating qtimezoneprivate_data_p.h: {e}\n')
        if args.verbose > 0:
            raise
        return 1

    # Locale-dependent timezone data
    try:
        with LocaleZoneDataWriter(
                qtsrcdir.joinpath('src/corelib/time/qtimezonelocale_data_p.h'),
                qtsrcdir, reader.cldrVersion, ianaNames, metaNames) as writer:
            writer.localeData(locale_map, locale_keys)
            writer.writeTables()
    except Exception as e:
        err.write(f'\nError updating qtimezonelocale_data_p.h: {e}\n')
        if args.verbose > 0:
            raise
        return 1

    # ./testlocales/localemodel.cpp
    try:
        path = 'util/locale_database/testlocales/localemodel.cpp'
        with TestLocaleWriter(qtsrcdir.joinpath(path), qtsrcdir,
                              reader.cldrVersion) as test:
            test.localeList(locale_keys)
    except Exception as e:
        err.write(f'\nError updating localemodel.cpp: {e}\n')
        if args.verbose > 0:
            raise
        return 1

    return 0

if __name__ == "__main__":
    import sys
    sys.exit(main(sys.argv, sys.stdout, sys.stderr))
