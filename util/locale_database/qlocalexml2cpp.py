#!/usr/bin/env python2
#############################################################################
##
## Copyright (C) 2020 The Qt Company Ltd.
## Contact: https://www.qt.io/licensing/
##
## This file is part of the test suite of the Qt Toolkit.
##
## $QT_BEGIN_LICENSE:GPL-EXCEPT$
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and The Qt Company. For licensing terms
## and conditions see https://www.qt.io/terms-conditions. For further
## information use the contact form at https://www.qt.io/contact-us.
##
## GNU General Public License Usage
## Alternatively, this file may be used under the terms of the GNU
## General Public License version 3 as published by the Free Software
## Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
## included in the packaging of this file. Please review the following
## information to ensure the GNU General Public License requirements will
## be met: https://www.gnu.org/licenses/gpl-3.0.html.
##
## $QT_END_LICENSE$
##
#############################################################################
"""Script to generate C++ code from CLDR data in qLocaleXML form

See ``cldr2qlocalexml.py`` for how to generate the qLocaleXML data itself.
Pass the output file from that as first parameter to this script; pass
the root of the qtbase check-out as second parameter.
"""

import os
import datetime

from qlocalexml import QLocaleXmlReader
from localetools import unicode2hex, wrap_list, Error, Transcriber, SourceFileEditor

def compareLocaleKeys(key1, key2):
    if key1 == key2:
        return 0

    if key1[0] != key2[0]: # First sort by language:
        return key1[0] - key2[0]

    defaults = compareLocaleKeys.default_map
    # maps {(language, script): country} by ID
    try:
        country = defaults[key1[:2]]
    except KeyError:
        pass
    else:
        if key1[2] == country:
            return -1
        if key2[2] == country:
            return 1

    if key1[1] == key2[1]:
        return key1[2] - key2[2]

    try:
        country = defaults[key2[:2]]
    except KeyError:
        pass
    else:
        if key2[2] == country:
            return 1
        if key1[2] == country:
            return -1

    return key1[1] - key2[1]


class StringDataToken:
    def __init__(self, index, length, bits):
        if index > 0xffff:
            raise ValueError('Start-index ({}) exceeds the uint16 range!'.format(index))
        if length >= (1 << bits):
            raise ValueError('Data size ({}) exceeds the {}-bit range!'.format(length, bits))

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
            raise ValueError('Data is too big ({}) for quint16 index to its end!'
                             .format(len(self.data)),
                             self.name)
        fd.write("\nstatic const char16_t {}[] = {{\n".format(self.name))
        fd.write(wrap_list(self.data))
        fd.write("\n};\n")

def currencyIsoCodeData(s):
    if s:
        return '{' + ",".join(str(ord(x)) for x in s) + '}'
    return "{0,0,0}"

class LocaleSourceEditor (SourceFileEditor):
    __upinit = SourceFileEditor.__init__
    def __init__(self, path, temp, version):
        self.__upinit(path, temp)
        self.writer.write("""
/*
    This part of the file was generated on {} from the
    Common Locale Data Repository v{}

    http://www.unicode.org/cldr/

    Do not edit this section: instead regenerate it using
    cldr2qlocalexml.py and qlocalexml2cpp.py on updated (or
    edited) CLDR data; see qtbase/util/locale_database/.
*/

""".format(datetime.date.today(), version))

class LocaleDataWriter (LocaleSourceEditor):
    def likelySubtags(self, likely):
        self.writer.write('static const QLocaleId likely_subtags[] = {\n')
        for had, have, got, give, last in likely:
            self.writer.write('    {{ {:3d}, {:3d}, {:3d} }}'.format(*have))
            self.writer.write(', {{ {:3d}, {:3d}, {:3d} }}'.format(*give))
            self.writer.write(' ' if last else ',')
            self.writer.write(' // {} -> {}\n'.format(had, got))
        self.writer.write('};\n\n')

    def localeIndex(self, indices):
        self.writer.write('static const quint16 locale_index[] = {\n')
        for pair in indices:
            self.writer.write('{:6d}, // {}\n'.format(*pair))
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
        self.writer.write('static const QLocaleData locale_data[] = {\n')
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
                       endonyms_data.append(locale.countryEndonym)) # 6 entries
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
                              + ', // {}/{}/{}\n'.format(
                    locale.language, locale.script, locale.country))
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
        out('static const char {}_name_list[] =\n'.format(form))
        out('"Default\\0"\n')
        for key, value in book.items():
            if key == 0:
                continue
            out('"' + value[0] + '\\0"\n')
        out(';\n\n')

        out('static const quint16 {}_name_index[] = {{\n'.format(form))
        out('     0, // Any{}\n'.format(form.capitalize()))
        index = 8
        for key, value in book.items():
            if key == 0:
                continue
            name = value[0]
            out('{:6d}, // {}\n'.format(index, name))
            index += len(name) + 1
        out('};\n\n')

    @staticmethod
    def __writeCodeList(out, book, form, width):
        out('static const unsigned char {}_code_list[] =\n'.format(form))
        for key, value in book.items():
            code = value[1]
            code += r'\0' * max(width - len(code), 0)
            out('"{}" // {}\n'.format(code, value[0]))
        out(';\n\n')

    def languageNames(self, languages):
        self.__writeNameData(self.writer.write, languages, 'language')

    def scriptNames(self, scripts):
        self.__writeNameData(self.writer.write, scripts, 'script')

    def countryNames(self, countries):
        self.__writeNameData(self.writer.write, countries, 'country')

    # TODO: unify these next three into the previous three; kept
    # separate for now to verify we're not changing data.

    def languageCodes(self, languages):
        self.__writeCodeList(self.writer.write, languages, 'language', 3)

    def scriptCodes(self, scripts):
        self.__writeCodeList(self.writer.write, scripts, 'script', 4)

    def countryCodes(self, countries): # TODO: unify with countryNames()
        self.__writeCodeList(self.writer.write, countries, 'country', 3)

class CalendarDataWriter (LocaleSourceEditor):
    formatCalendar = (
        '      {{'
        + ','.join(('{:6d}',) * 3 + ('{:5d}',) * 6 + ('{:3d}',) * 6)
        + ' }},').format
    def write(self, calendar, locales, names):
        months_data = StringData('months_data')

        self.writer.write('static const QCalendarLocale locale_data[] = {\n')
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
                e.args += (locale.language, locale.script, locale.country, stem)
                raise

            self.writer.write(
                self.formatCalendar(*(
                        key +
                        tuple(r.index for r in ranges) +
                        tuple(r.length for r in ranges) ))
                + '// {}/{}/{}\n'.format(locale.language, locale.script, locale.country))
        self.writer.write(self.formatCalendar(*( (0,) * (3 + 6 * 2) ))
                          + '// trailing zeros\n')
        self.writer.write('};\n')
        months_data.write(self.writer)

class LocaleHeaderWriter (SourceFileEditor):
    __upinit = SourceFileEditor.__init__
    def __init__(self, path, temp, dupes):
        self.__upinit(path, temp)
        self.__dupes = dupes

    def languages(self, languages):
        self.__enum('Language', languages, self.__language)
        self.writer.write('\n')

    def countries(self, countries):
        self.__enum('Country', countries, self.__country)

    def scripts(self, scripts):
        self.__enum('Script', scripts, self.__script)
        self.writer.write('\n')

    # Implementation details
    from enumdata import (language_aliases as __language,
                          country_aliases as __country,
                          script_aliases as __script)

    def __enum(self, name, book, alias):
        assert book
        out, dupes = self.writer.write, self.__dupes
        out('    enum {} : ushort {{\n'.format(name))
        for key, value in book.items():
            member = value[0].replace('-', ' ')
            if name == 'Script':
                # Don't .capitalize() as some names are already camel-case (see enumdata.py):
                member = ''.join(word[0].upper() + word[1:] for word in member.split())
                if not member.endswith('Script'):
                    member += 'Script'
                if member in dupes:
                    raise Error('The script name "{}" is messy'.format(member))
            else:
                member = ''.join(member.split())
                member = member + name if member in dupes else member
            out('        {} = {},\n'.format(member, key))

        out('\n        '
            + ',\n        '.join('{} = {}'.format(*pair)
                                 for pair in sorted(alias.items()))
            + ',\n\n        Last{} = {}\n    }};\n'.format(name, member))

def usage(name, err, message = ''):
    err.write("""Usage: {} path/to/qlocale.xml root/of/qtbase
""".format(name)) # TODO: elaborate
    if message:
        err.write('\n' + message + '\n')

def main(args, out, err):
    # TODO: Make calendars a command-line parameter
    # map { CLDR name: Qt file name }
    calendars = {'gregorian': 'roman', 'persian': 'jalali', 'islamic': 'hijri',} # 'hebrew': 'hebrew',

    name = args.pop(0)
    if len(args) != 2:
        usage(name, err, 'I expect two arguments')
        return 1

    qlocalexml = args.pop(0)
    qtsrcdir = args.pop(0)

    if not (os.path.isdir(qtsrcdir)
            and all(os.path.isfile(os.path.join(qtsrcdir, 'src', 'corelib', 'text', leaf))
                    for leaf in ('qlocale_data_p.h', 'qlocale.h', 'qlocale.qdoc'))):
        usage(name, err, 'Missing expected files under qtbase source root ' + qtsrcdir)
        return 1

    reader = QLocaleXmlReader(qlocalexml)
    locale_map = dict(reader.loadLocaleMap(calendars, err.write))

    locale_keys = locale_map.keys()
    compareLocaleKeys.default_map = dict(reader.defaultMap())
    locale_keys.sort(compareLocaleKeys)

    try:
        writer = LocaleDataWriter(os.path.join(qtsrcdir,  'src', 'corelib', 'text',
                                               'qlocale_data_p.h'),
                                  qtsrcdir, reader.cldrVersion)
    except IOError as e:
        err.write('Failed to open files to transcribe locale data: ' + (e.message or e.args[1]))
        return 1

    try:
        writer.likelySubtags(reader.likelyMap())
        writer.localeIndex(reader.languageIndices(tuple(k[0] for k in locale_map)))
        writer.localeData(locale_map, locale_keys)
        writer.writer.write('\n')
        writer.languageNames(reader.languages)
        writer.scriptNames(reader.scripts)
        writer.countryNames(reader.countries)
        # TODO: merge the next three into the previous three
        writer.languageCodes(reader.languages)
        writer.scriptCodes(reader.scripts)
        writer.countryCodes(reader.countries)
    except Error as e:
        writer.cleanup()
        err.write('\nError updating locale data: ' + e.message + '\n')
        return 1

    writer.close()

    # Generate calendar data
    for calendar, stem in calendars.items():
        try:
            writer = CalendarDataWriter(os.path.join(qtsrcdir, 'src', 'corelib', 'time',
                                                     'q{}calendar_data_p.h'.format(stem)),
                                        qtsrcdir, reader.cldrVersion)
        except IOError as e:
            err.write('Failed to open files to transcribe ' + calendar
                             + ' data ' + (e.message or e.args[1]))
            return 1

        try:
            writer.write(calendar, locale_map, locale_keys)
        except Error as e:
            writer.cleanup()
            err.write('\nError updating ' + calendar + ' locale data: ' + e.message + '\n')
            return 1

        writer.close()

    # qlocale.h
    try:
        writer = LocaleHeaderWriter(os.path.join(qtsrcdir, 'src', 'corelib', 'text', 'qlocale.h'),
                                    qtsrcdir, reader.dupes)
    except IOError as e:
        err.write('Failed to open files to transcribe qlocale.h: ' + (e.message or e.args[1]))
        return 1

    try:
        writer.languages(reader.languages)
        writer.scripts(reader.scripts)
        writer.countries(reader.countries)
    except Error as e:
        writer.cleanup()
        err.write('\nError updating qlocale.h: ' + e.message + '\n')
        return 1

    writer.close()

    # qlocale.qdoc
    try:
        writer = Transcriber(os.path.join(qtsrcdir, 'src', 'corelib', 'text', 'qlocale.qdoc'),
                             qtsrcdir)
    except IOError as e:
        err.write('Failed to open files to transcribe qlocale.qdoc: ' + (e.message or e.args[1]))
        return 1

    DOCSTRING = "    QLocale's data is based on Common Locale Data Repository "
    try:
        for line in writer.reader:
            if DOCSTRING in line:
                writer.writer.write(DOCSTRING + 'v' + reader.cldrVersion + '.\n')
            else:
                writer.writer.write(line)
    except Error as e:
        writer.cleanup()
        err.write('\nError updating qlocale.qdoc: ' + e.message + '\n')
        return 1

    writer.close()
    return 0

if __name__ == "__main__":
    import sys
    sys.exit(main(sys.argv, sys.stdout, sys.stderr))
