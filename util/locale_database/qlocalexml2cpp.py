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
from typing import Optional

from qlocalexml import QLocaleXmlReader
from localetools import unicode2hex, wrap_list, Error, Transcriber, SourceFileEditor, qtbase_root
from iso639_3 import LanguageCodeData

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

    def __init__(self, defaults):
        self.map = dict(defaults)
    def foreign(self, key):
        default = self.map.get(key[:2])
        return default is None or default != key[2]
    def __call__(self, key):
        # TODO: should we compare territory before or after script ?
        return (key[0], self.foreign(key)) + key[1:]

class StringDataToken:
    def __init__(self, index, length, bits):
        if index > 0xffff:
            raise ValueError(f'Start-index ({index}) exceeds the uint16 range!')
        if length >= (1 << bits):
            raise ValueError(f'Data size ({length}) exceeds the {bits}-bit range!')

        self.index = index
        self.length = length

class StringData:
    def __init__(self, name):
        self.data = []
        self.hash = {}
        self.name = name
        self.text = '' # Used in quick-search for matches in data

    def append(self, s, bits = 8):
        try:
            token = self.hash[s]
        except KeyError:
            token = self.__store(s, bits)
            self.hash[s] = token
        return token

    def __store(self, s, bits):
        """Add string s to known data.

        Seeks to avoid duplication, where possible.
        For example, short-forms may be prefixes of long-forms.
        """
        if not s:
            return StringDataToken(0, 0, bits)
        ucs2 = unicode2hex(s)
        try:
            index = self.text.index(s) - 1
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
            return StringDataToken(index, len(ucs2), bits)
        except ValueError as e:
            e.args += (self.name, s)
            raise

    def write(self, fd):
        if len(self.data) > 0xffff:
            raise ValueError(f'Data is too big ({len(self.data)}) for quint16 index to its end!',
                             self.name)
        fd.write(f"\nstatic constexpr char16_t {self.name}[] = {{\n")
        fd.write(wrap_list(self.data))
        fd.write("\n};\n")

def currencyIsoCodeData(s):
    if s:
        return '{' + ",".join(str(ord(x)) for x in s) + '}'
    return "{0,0,0}"

class LocaleSourceEditor (SourceFileEditor):
    def __init__(self, path: Path, temp: Path, version: str):
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

class LocaleDataWriter (LocaleSourceEditor):
    def likelySubtags(self, likely):
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
        self.writer.write('static constexpr QLocaleId likely_subtags[] = {\n')
        for had, have, got, give in likely:
            i += 1
            self.writer.write('    {{ {:3d}, {:3d}, {:3d} }}'.format(*have))
            self.writer.write(', {{ {:3d}, {:3d}, {:3d} }}'.format(*give))
            self.writer.write(' ' if i == len(likely) else ',')
            self.writer.write(f' // {had} -> {got}\n')
        self.writer.write('};\n\n')

    def localeIndex(self, indices):
        self.writer.write('static constexpr quint16 locale_index[] = {\n')
        for index, name in indices:
            self.writer.write(f'{index:6d}, // {name}\n')
        self.writer.write('     0 // trailing 0\n')
        self.writer.write('};\n\n')

    def localeData(self, locales, names):
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
        self.writer.write('static constexpr QLocaleData locale_data[] = {\n')
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
            locale = locales[key]
            # Sequence of StringDataToken:
            ranges = (tuple(list_pattern_part_data.append(p) for p in # 5 entries:
                            (locale.listPatternPartStart, locale.listPatternPartMiddle,
                             locale.listPatternPartEnd, locale.listPatternPartTwo,
                             locale.listDelim)) +
                      tuple(single_character_data.append(p) for p in # 11 entries
                            (locale.decimal, locale.group, locale.percent, locale.zero,
                             locale.minus, locale.plus, locale.exp,
                             locale.quotationStart, locale.quotationEnd,
                             locale.alternateQuotationStart, locale.alternateQuotationEnd)) +
                      tuple (date_format_data.append(f) for f in # 2 entries:
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
            data.write(self.writer)

    @staticmethod
    def __writeNameData(out, book, form):
        out(f'static constexpr char {form}_name_list[] =\n')
        out('"Default\\0"\n')
        for key, value in book.items():
            if key == 0:
                continue
            out(f'"{value[0]}\\0"\n')
        out(';\n\n')

        out(f'static constexpr quint16 {form}_name_index[] = {{\n')
        out(f'     0, // Any{form.capitalize()}\n')
        index = 8
        for key, value in book.items():
            if key == 0:
                continue
            name = value[0]
            out(f'{index:6d}, // {name}\n')
            index += len(name) + 1
        out('};\n\n')

    @staticmethod
    def __writeCodeList(out, book, form, width):
        out(f'static constexpr unsigned char {form}_code_list[] =\n')
        for key, value in book.items():
            code = value[1]
            code += r'\0' * max(width - len(code), 0)
            out(f'"{code}" // {value[0]}\n')
        out(';\n\n')

    def languageNames(self, languages):
        self.__writeNameData(self.writer.write, languages, 'language')

    def scriptNames(self, scripts):
        self.__writeNameData(self.writer.write, scripts, 'script')

    def territoryNames(self, territories):
        self.__writeNameData(self.writer.write, territories, 'territory')

    # TODO: unify these next three into the previous three; kept
    # separate for now to verify we're not changing data.

    def languageCodes(self, languages, code_data: LanguageCodeData):
        out = self.writer.write

        out(f'constexpr std::array<LanguageCodeEntry, {len(languages)}> languageCodeList {{\n')

        def q(val: Optional[str], size: int) -> str:
            """Quote the value and adjust the result for tabular view."""
            s = '' if val is None else ', '.join(f"'{c}'" for c in val)
            return f'{{{s}}}' if size == 0 else f'{{{s}}},'.ljust(size * 5 + 2)

        for key, value in languages.items():
            code = value[1]
            if key < 2:
                result = code_data.query('und')
            else:
                result = code_data.query(code)
                assert code == result.id()
            assert result is not None

            codeString = q(result.part1Code, 2)
            codeString += q(result.part2BCode, 3)
            codeString += q(result.part2TCode, 3)
            codeString += q(result.part3Code, 0)
            out(f'    LanguageCodeEntry {{{codeString}}}, // {value[0]}\n')

        out('};\n\n')

    def scriptCodes(self, scripts):
        self.__writeCodeList(self.writer.write, scripts, 'script', 4)

    def territoryCodes(self, territories): # TODO: unify with territoryNames()
        self.__writeCodeList(self.writer.write, territories, 'territory', 3)

class CalendarDataWriter (LocaleSourceEditor):
    formatCalendar = (
        '      {{'
        + ','.join(('{:6d}',) * 3 + ('{:5d}',) * 6 + ('{:3d}',) * 6)
        + ' }},').format
    def write(self, calendar, locales, names):
        months_data = StringData('months_data')

        self.writer.write('static constexpr QCalendarLocale locale_data[] = {\n')
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
            locale = locales[key]
            # Sequence of StringDataToken:
            try:
                # Twelve long month names can add up to more than 256 (e.g. kde_TZ: 264)
                ranges = (tuple(months_data.append(m[calendar], 16) for m in
                                (locale.standaloneLongMonths, locale.longMonths)) +
                          tuple(months_data.append(m[calendar]) for m in
                                (locale.standaloneShortMonths, locale.shortMonths,
                                 locale.standaloneNarrowMonths, locale.narrowMonths)))
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
        months_data.write(self.writer)

class LocaleHeaderWriter (SourceFileEditor):
    def __init__(self, path, temp, dupes):
        super().__init__(path, temp)
        self.__dupes = dupes

    def languages(self, languages):
        self.__enum('Language', languages, self.__language)
        self.writer.write('\n')

    def territories(self, territories):
        self.writer.write("    // ### Qt 7: Rename to Territory\n")
        self.__enum('Country', territories, self.__territory, 'Territory')

    def scripts(self, scripts):
        self.__enum('Script', scripts, self.__script)
        self.writer.write('\n')

    # Implementation details
    from enumdata import (language_aliases as __language,
                          territory_aliases as __territory,
                          script_aliases as __script)

    def __enum(self, name, book, alias, suffix = None):
        assert book

        if suffix is None:
            suffix = name

        out, dupes = self.writer.write, self.__dupes
        out(f'    enum {name} : ushort {{\n')
        for key, value in book.items():
            member = value[0].replace('-', ' ')
            if name == 'Script':
                # Don't .capitalize() as some names are already camel-case (see enumdata.py):
                member = ''.join(word[0].upper() + word[1:] for word in member.split())
                if not member.endswith('Script'):
                    member += 'Script'
                if member in dupes:
                    raise Error(f'The script name "{member}" is messy')
            else:
                member = ''.join(member.split())
                member = member + suffix if member in dupes else member
            out(f'        {member} = {key},\n')

        out('\n        '
            + ',\n        '.join(f'{k} = {v}' for k, v in sorted(alias.items()))
            + f',\n\n        Last{suffix} = {member}')

        # for "LastCountry = LastTerritory"
        # ### Qt 7: Remove
        if suffix != name:
            out(f',\n        Last{name} = Last{suffix}')

        out('\n    };\n')


def main(out, err):
    calendars_map = {
        # CLDR name: Qt file name fragment
        'gregorian': 'roman',
        'persian': 'jalali',
        'islamic': 'hijri',
        # 'hebrew': 'hebrew'
    }
    all_calendars = list(calendars_map.keys())

    parser = argparse.ArgumentParser(
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
    args = parser.parse_args()

    qlocalexml = args.input_file
    qtsrcdir = Path(args.qtbase_path)
    calendars = {cal: calendars_map[cal] for cal in args.calendars}

    if not (qtsrcdir.is_dir()
            and all(qtsrcdir.joinpath('src/corelib/text', leaf).is_file()
                    for leaf in ('qlocale_data_p.h', 'qlocale.h', 'qlocale.qdoc'))):
        parser.error(f'Missing expected files under qtbase source root {qtsrcdir}')

    reader = QLocaleXmlReader(qlocalexml)
    locale_map = dict(reader.loadLocaleMap(calendars, err.write))
    locale_keys = sorted(locale_map.keys(), key=LocaleKeySorter(reader.defaultMap()))

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
                                qtsrcdir, reader.dupes) as writer:
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

    return 0

if __name__ == "__main__":
    import sys
    sys.exit(main(sys.stdout, sys.stderr))
