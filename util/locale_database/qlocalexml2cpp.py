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
from xml.dom import minidom
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
    def __init__(self, index, length):
        if index > 0xFFFF or length > 0xFFFF:
            raise Error("Position exceeds ushort range: {},{}".format(index, length))
        self.index = index
        self.length = length
    def __str__(self):
        return " {},{} ".format(self.index, self.length)

class StringData:
    def __init__(self, name):
        self.data = []
        self.hash = {}
        self.name = name

    def append(self, s):
        if s in self.hash:
            return self.hash[s]

        lst = unicode2hex(s)
        index = len(self.data)
        if index > 0xffff:
            raise Error('Data index {} is too big for uint16!'.format(index))
        size = len(lst)
        if size >= 0xffff:
            raise Error('Data is too big ({}) for uint16 size!'.format(size))
        token = None
        try:
            token = StringDataToken(index, size)
        except Error as e:
            e.message += '(on data "{}")'.format(s)
            raise
        self.hash[s] = token
        self.data += lst
        return token

    def write(self, fd):
        fd.write("\nstatic const ushort {}[] = {{\n".format(self.name))
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
                          '  dec  ' # Numeric punctuation
                          ' group '
                          ' list  ' # Delimiter for *numeric* lists
                          ' prcnt ' # Arithmetic symbols
                          '  zero '
                          ' minus '
                          ' plus  '
                          '  exp  '
                          # Width 8 + comma - to make space for these wide labels !
                          ' quotOpn ' # Quotation marks
                          ' quotEnd '
                          'altQtOpn '
                          'altQtEnd '
                          # Width 11 + comma
                          '  lpStart   ' # List pattern
                          '   lpMid    '
                          '   lpEnd    '
                          '   lpTwo    '
                          '   sDtFmt   ' # Date format
                          '   lDtFmt   '
                          '   sTmFmt   ' # Time format
                          '   lTmFmt   '
                          '   ssDays   ' # Days
                          '   slDays   '
                          '   snDays   '
                          '    sDays   '
                          '    lDays   '
                          '    nDays   '
                          '     am     ' # am/pm indicators
                          '     pm     '
                          # Width 8 + comma
                          '  byte   '
                          ' siQuant '
                          'iecQuant '
                          # Width 8+4 + comma
                          '   currISO   '
                          # Width 11 + comma
                          '  currSym   ' # Currency formatting
                          ' currDsply  '
                          '  currFmt   '
                          ' currFmtNeg '
                          '  endoLang  ' # Name of language in itself, and of country
                          '  endoCntry '
                          # Width 6 + comma
                          'curDgt ' # Currency number representation
                          'curRnd '
                          'dow1st ' # First day of week
                          ' wknd+ ' # Week-end start/end days
                          ' wknd-'
                          # No trailing space on last entry (be sure to
                          # pad before adding anything after it).
                          '\n')

        formatLine = ''.join((
            '    {{ ',
            # Locale-identifier
            '{:6d},' * 3,
            # Numeric formats, list delimiter
            '{:6d},' * 8,
            # Quotation marks
            '{:8d},' * 4,
            # List patterns, date/time formats, month/day names, am/pm
            '{:>11s},' * 16,
            # SI/IEC byte-unit abbreviations
            '{:>8s},' * 3,
            # Currency ISO code
            ' {:>10s}, ',
            # Currency and endonyms
            '{:>11s},' * 6,
            # Currency formatting
            '{:6d},{:6d}',
            # Day of week and week-end
            ',{:6d}' * 3,
            ' }}')).format
        for key in names:
            locale = locales[key]
            self.writer.write(formatLine(
                    key[0], key[1], key[2],
                    locale.decimal,
                    locale.group,
                    locale.listDelim,
                    locale.percent,
                    locale.zero,
                    locale.minus,
                    locale.plus,
                    locale.exp,
                    locale.quotationStart,
                    locale.quotationEnd,
                    locale.alternateQuotationStart,
                    locale.alternateQuotationEnd,
                    list_pattern_part_data.append(locale.listPatternPartStart),
                    list_pattern_part_data.append(locale.listPatternPartMiddle),
                    list_pattern_part_data.append(locale.listPatternPartEnd),
                    list_pattern_part_data.append(locale.listPatternPartTwo),
                    date_format_data.append(locale.shortDateFormat),
                    date_format_data.append(locale.longDateFormat),
                    time_format_data.append(locale.shortTimeFormat),
                    time_format_data.append(locale.longTimeFormat),
                    days_data.append(locale.standaloneShortDays),
                    days_data.append(locale.standaloneLongDays),
                    days_data.append(locale.standaloneNarrowDays),
                    days_data.append(locale.shortDays),
                    days_data.append(locale.longDays),
                    days_data.append(locale.narrowDays),
                    am_data.append(locale.am),
                    pm_data.append(locale.pm),
                    byte_unit_data.append(locale.byte_unit),
                    byte_unit_data.append(locale.byte_si_quantified),
                    byte_unit_data.append(locale.byte_iec_quantified),
                    currencyIsoCodeData(locale.currencyIsoCode),
                    currency_symbol_data.append(locale.currencySymbol),
                    currency_display_name_data.append(locale.currencyDisplayName),
                    currency_format_data.append(locale.currencyFormat),
                    currency_format_data.append(locale.currencyNegativeFormat),
                    endonyms_data.append(locale.languageEndonym),
                    endonyms_data.append(locale.countryEndonym),
                    locale.currencyDigits,
                    locale.currencyRounding, # unused (QTBUG-81343)
                    locale.firstDayOfWeek,
                    locale.weekendStart,
                    locale.weekendEnd)
                              + ', // {}/{}/{}\n'.format(
                    locale.language, locale.script, locale.country))
        self.writer.write(formatLine(*( # All zeros, matching the format:
                    (0,) * (3 + 8 + 4) + ('0,0',) * (16 + 3)
                    + (currencyIsoCodeData(0),)
                    + ('0,0',) * 6 + (0,) * (2 + 3) ))
                          + ' // trailing zeros\n')
        self.writer.write('};\n')

        # StringData tables:
        for data in (list_pattern_part_data, date_format_data,
                     time_format_data, days_data,
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
    formatCalendar = ''.join((
        '      {{',
        '{:6d}',
        ',{:6d}' * 2,
        ',{{{:>5s}}}' * 6,
        '}}, ')).format
    def write(self, calendar, locales, names):
        months_data = StringData('months_data')

        self.writer.write('static const QCalendarLocale locale_data[] = {\n')
        self.writer.write('   // '
                          # IDs, width 7 (6 + comma)
                          + ' lang  '
                          + ' script'
                          + ' terr  '
                          # Month-name start-end pairs, width 8 (5 plus '{},'):
                          + ' sShort '
                          + ' sLong  '
                          + ' sNarrow'
                          + ' short  '
                          + ' long   '
                          + ' narrow'
                          # No trailing space on last; be sure
                          # to pad before adding later entries.
                          + '\n')
        for key in names:
            locale = locales[key]
            self.writer.write(
                self.formatCalendar(
                    key[0], key[1], key[2],
                    months_data.append(locale.standaloneShortMonths[calendar]),
                    months_data.append(locale.standaloneLongMonths[calendar]),
                    months_data.append(locale.standaloneNarrowMonths[calendar]),
                    months_data.append(locale.shortMonths[calendar]),
                    months_data.append(locale.longMonths[calendar]),
                    months_data.append(locale.narrowMonths[calendar]))
                + '// {}/{}/{}\n'.format(locale.language, locale.script, locale.country))
        self.writer.write(self.formatCalendar(*( (0,) * 3 + ('0,0',) * 6 ))
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
        out('    enum {} {{\n'.format(name))
        for key, value in book.items():
            member = value[0]
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
