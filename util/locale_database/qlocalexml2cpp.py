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
import sys
import tempfile
import datetime

from qlocalexml import QLocaleXmlReader
from enumdata import language_aliases, country_aliases, script_aliases
from localetools import unicode2hex, wrap_list, Error

# TODO: Make calendars a command-line parameter
# map { CLDR name: Qt file name }
calendars = {'gregorian': 'roman', 'persian': 'jalali', 'islamic': 'hijri',} # 'hebrew': 'hebrew',

generated_template = """
/*
    This part of the file was generated on %s from the
    Common Locale Data Repository v%s

    http://www.unicode.org/cldr/

    Do not edit this section: instead regenerate it using
    cldr2qlocalexml.py and qlocalexml2cpp.py on updated (or
    edited) CLDR data; see qtbase/util/locale_database/.
*/

"""

def fixedScriptName(name, dupes):
    # Don't .capitalize() as some names are already camel-case (see enumdata.py):
    name = ''.join(word[0].upper() + word[1:] for word in name.split())
    if name[-6:] != "Script":
        name = name + "Script"
    if name in dupes:
        sys.stderr.write("\n\n\nERROR: The script name '%s' is messy" % name)
        sys.exit(1)
    return name

def fixedCountryName(name, dupes):
    if name in dupes:
        return name.replace(" ", "") + "Country"
    return name.replace(" ", "")

def fixedLanguageName(name, dupes):
    if name in dupes:
        return name.replace(" ", "") + "Language"
    return name.replace(" ", "")

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
            raise Error("Position exceeds ushort range: %d,%d " % (index, length))
        self.index = index
        self.length = length
    def __str__(self):
        return " %d,%d " % (self.index, self.length)

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
        if index > 65535:
            print "\n\n\n#error Data index is too big!"
            sys.stderr.write ("\n\n\nERROR: index exceeds the uint16 range! index = %d\n" % index)
            sys.exit(1)
        size = len(lst)
        if size >= 65535:
            print "\n\n\n#error Data is too big!"
            sys.stderr.write ("\n\n\nERROR: data size exceeds the uint16 range! size = %d\n" % size)
            sys.exit(1)
        token = None
        try:
            token = StringDataToken(index, size)
        except Error as e:
            sys.stderr.write("\n\n\nERROR: %s: on data '%s'" % (e, s))
            sys.exit(1)
        self.hash[s] = token
        self.data += lst
        return token

    def write(self, fd):
        fd.write("\nstatic const ushort %s[] = {\n" % self.name)
        fd.write(wrap_list(self.data))
        fd.write("\n};\n")

def currencyIsoCodeData(s):
    if s:
        return '{' + ",".join(str(ord(x)) for x in s) + '}'
    return "{0,0,0}"

def usage():
    print "Usage: qlocalexml2cpp.py <path-to-locale.xml> <path-to-qtbase-src-tree>"
    sys.exit(1)

GENERATED_BLOCK_START = "// GENERATED PART STARTS HERE\n"
GENERATED_BLOCK_END = "// GENERATED PART ENDS HERE\n"

def main():
    if len(sys.argv) != 3:
        usage()

    qlocalexml = sys.argv[1]
    qtsrcdir = sys.argv[2]

    if not (os.path.isdir(qtsrcdir)
            and all(os.path.isfile(os.path.join(qtsrcdir, 'src', 'corelib', 'text', leaf))
                    for leaf in ('qlocale_data_p.h', 'qlocale.h', 'qlocale.qdoc'))):
        usage()

    (data_temp_file, data_temp_file_path) = tempfile.mkstemp("qlocale_data_p", dir=qtsrcdir)
    data_temp_file = os.fdopen(data_temp_file, "w")
    qlocaledata_file = open(qtsrcdir + "/src/corelib/text/qlocale_data_p.h", "r")
    s = qlocaledata_file.readline()
    while s and s != GENERATED_BLOCK_START:
        data_temp_file.write(s)
        s = qlocaledata_file.readline()
    data_temp_file.write(GENERATED_BLOCK_START)

    reader = QLocaleXmlReader(qlocalexml)
    locale_map = dict(reader.loadLocaleMap(calendars, sys.stderr.write))
    data_temp_file.write(generated_template % (datetime.date.today(), reader.cldrVersion))

    # Likely subtags map
    data_temp_file.write("static const QLocaleId likely_subtags[] = {\n")
    for had, have, got, give, last in reader.likelyMap():
        data_temp_file.write('    {{ {:3d}, {:3d}, {:3d} }}'.format(*have))
        data_temp_file.write(', {{ {:3d}, {:3d}, {:3d} }}'.format(*give))
        data_temp_file.write(' ' if last else ',')
        data_temp_file.write(' // {} -> {}\n'.format(had, got))
    data_temp_file.write("};\n")

    data_temp_file.write("\n")

    # Locale index
    data_temp_file.write("static const quint16 locale_index[] = {\n")
    for index, name in reader.languageIndices(tuple(k[0] for k in locale_map)):
        data_temp_file.write('{:6d}, // {}\n'.format(index, name))
    data_temp_file.write("     0 // trailing 0\n")
    data_temp_file.write("};\n\n")

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
    data_temp_file.write("static const QLocaleData locale_data[] = {\n")
    # Table headings: keep each label centred in its field, matching line_format:
    data_temp_file.write('   // '
                         # Width 6 + comma:
                         + ' lang  ' # IDs
                         + 'script '
                         + '  terr '
                         + '  dec  ' # Numeric punctuation:
                         + ' group '
                         + ' list  ' # List delimiter
                         + ' prcnt ' # Arithmetic symbols:
                         + '  zero '
                         + ' minus '
                         + ' plus  '
                         + '  exp  '
                         # Width 8 + comma - to make space for these wide labels !
                         + ' quotOpn ' # Quotation marks
                         + ' quotEnd '
                         + 'altQtOpn '
                         + 'altQtEnd '
                         # Width 11 + comma:
                         + '  lpStart   ' # List pattern
                         + '   lpMid    '
                         + '   lpEnd    '
                         + '   lpTwo    '
                         + '   sDtFmt   ' # Date format
                         + '   lDtFmt   '
                         + '   sTmFmt   ' # Time format
                         + '   lTmFmt   '
                         + '   ssDays   ' # Days
                         + '   slDays   '
                         + '   snDays   '
                         + '    sDays   '
                         + '    lDays   '
                         + '    nDays   '
                         + '     am     ' # am/pm indicators
                         + '     pm     '
                         # Width 8 + comma
                         + '  byte   '
                         + ' siQuant '
                         + 'iecQuant '
                         # Width 8+4 + comma
                         + '   currISO   '
                         # Width 11 + comma:
                         + '  currSym   ' # Currency formatting:
                         + ' currDsply  '
                         + '  currFmt   '
                         + ' currFmtNeg '
                         + '  endoLang  ' # Name of language in itself, and of country:
                         + '  endoCntry '
                         # Width 6 + comma:
                         + 'curDgt ' # Currency number representation:
                         + 'curRnd '
                         + 'dow1st ' # First day of week
                         + ' wknd+ ' # Week-end start/end days:
                         + ' wknd-'
                         # No trailing space on last entry (be sure to
                         # pad before adding anything after it).
                         + '\n')

    locale_keys = locale_map.keys()
    compareLocaleKeys.default_map = dict(reader.defaultMap())
    locale_keys.sort(compareLocaleKeys)

    line_format = ('    { '
                   # Locale-identifier:
                   + '%6d,' * 3
                   # Numeric formats, list delimiter:
                   + '%6d,' * 8
                   # Quotation marks:
                   + '%8d,' * 4
                   # List patterns, date/time formats, month/day names, am/pm:
                   + '%11s,' * 16
                   # SI/IEC byte-unit abbreviations:
                   + '%8s,' * 3
                   # Currency ISO code:
                   + ' %10s, '
                   # Currency and endonyms
                   + '%11s,' * 6
                   # Currency formatting:
                   + '%6d,%6d'
                   # Day of week and week-end:
                   + ',%6d' * 3
                   + ' }')
    for key in locale_keys:
        l = locale_map[key]
        data_temp_file.write(line_format
                    % (key[0], key[1], key[2],
                        l.decimal,
                        l.group,
                        l.listDelim,
                        l.percent,
                        l.zero,
                        l.minus,
                        l.plus,
                        l.exp,
                        l.quotationStart,
                        l.quotationEnd,
                        l.alternateQuotationStart,
                        l.alternateQuotationEnd,
                        list_pattern_part_data.append(l.listPatternPartStart),
                        list_pattern_part_data.append(l.listPatternPartMiddle),
                        list_pattern_part_data.append(l.listPatternPartEnd),
                        list_pattern_part_data.append(l.listPatternPartTwo),
                        date_format_data.append(l.shortDateFormat),
                        date_format_data.append(l.longDateFormat),
                        time_format_data.append(l.shortTimeFormat),
                        time_format_data.append(l.longTimeFormat),
                        days_data.append(l.standaloneShortDays),
                        days_data.append(l.standaloneLongDays),
                        days_data.append(l.standaloneNarrowDays),
                        days_data.append(l.shortDays),
                        days_data.append(l.longDays),
                        days_data.append(l.narrowDays),
                        am_data.append(l.am),
                        pm_data.append(l.pm),
                        byte_unit_data.append(l.byte_unit),
                        byte_unit_data.append(l.byte_si_quantified),
                        byte_unit_data.append(l.byte_iec_quantified),
                        currencyIsoCodeData(l.currencyIsoCode),
                        currency_symbol_data.append(l.currencySymbol),
                        currency_display_name_data.append(l.currencyDisplayName),
                        currency_format_data.append(l.currencyFormat),
                        currency_format_data.append(l.currencyNegativeFormat),
                        endonyms_data.append(l.languageEndonym),
                        endonyms_data.append(l.countryEndonym),
                        l.currencyDigits,
                        l.currencyRounding,
                        l.firstDayOfWeek,
                        l.weekendStart,
                        l.weekendEnd)
                             + ", // %s/%s/%s\n" % (l.language, l.script, l.country))
    data_temp_file.write(line_format # All zeros, matching the format:
                         % ( (0,) * (3 + 8 + 4) + ("0,0",) * (16 + 3)
                             + (currencyIsoCodeData(0),)
                             + ("0,0",) * 6 + (0,) * (2 + 3))
                         + " // trailing 0s\n")
    data_temp_file.write("};\n")

    # StringData tables:
    for data in (list_pattern_part_data, date_format_data,
                 time_format_data, days_data,
                 byte_unit_data, am_data, pm_data, currency_symbol_data,
                 currency_display_name_data, currency_format_data,
                 endonyms_data):
        data.write(data_temp_file)

    data_temp_file.write("\n")

    # Language name list
    data_temp_file.write("static const char language_name_list[] =\n")
    data_temp_file.write('"Default\\0"\n')
    for key, value in reader.languages.items():
        if key == 0:
            continue
        data_temp_file.write('"' + value[0] + '\\0"\n')
    data_temp_file.write(";\n")

    data_temp_file.write("\n")

    # Language name index
    data_temp_file.write("static const quint16 language_name_index[] = {\n")
    data_temp_file.write("     0, // AnyLanguage\n")
    index = 8
    for key, value in reader.languages.items():
        if key == 0:
            continue
        language = value[0]
        data_temp_file.write("%6d, // %s\n" % (index, language))
        index += len(language) + 1
    data_temp_file.write("};\n")

    data_temp_file.write("\n")

    # Script name list
    data_temp_file.write("static const char script_name_list[] =\n")
    data_temp_file.write('"Default\\0"\n')
    for key, value in reader.scripts.items():
        if key == 0:
            continue
        data_temp_file.write('"' + value[0] + '\\0"\n')
    data_temp_file.write(";\n")

    data_temp_file.write("\n")

    # Script name index
    data_temp_file.write("static const quint16 script_name_index[] = {\n")
    data_temp_file.write("     0, // AnyScript\n")
    index = 8
    for key, value in reader.scripts.items():
        if key == 0:
            continue
        script = value[0]
        data_temp_file.write("%6d, // %s\n" % (index, script))
        index += len(script) + 1
    data_temp_file.write("};\n")

    data_temp_file.write("\n")

    # Country name list
    data_temp_file.write("static const char country_name_list[] =\n")
    data_temp_file.write('"Default\\0"\n')
    for key, value in reader.countries.items():
        if key == 0:
            continue
        data_temp_file.write('"' + value[0] + '\\0"\n')
    data_temp_file.write(";\n")

    data_temp_file.write("\n")

    # Country name index
    data_temp_file.write("static const quint16 country_name_index[] = {\n")
    data_temp_file.write("     0, // AnyCountry\n")
    index = 8
    for key, value in reader.countries.items():
        if key == 0:
            continue
        country = value[0]
        data_temp_file.write("%6d, // %s\n" % (index, country))
        index += len(country) + 1
    data_temp_file.write("};\n")

    data_temp_file.write("\n")

    # Language code list
    data_temp_file.write("static const unsigned char language_code_list[] =\n")
    for key, value in reader.languages.items():
        code = value[1]
        if len(code) == 2:
            code += r"\0"
        data_temp_file.write('"%2s" // %s\n' % (code, value[0]))
    data_temp_file.write(";\n")

    data_temp_file.write("\n")

    # Script code list
    data_temp_file.write("static const unsigned char script_code_list[] =\n")
    for key, value in reader.scripts.items():
        code = value[1]
        for i in range(4 - len(code)):
            code += "\\0"
        data_temp_file.write('"%2s" // %s\n' % (code, value[0]))
    data_temp_file.write(";\n")

    # Country code list
    data_temp_file.write("static const unsigned char country_code_list[] =\n")
    for key, value in reader.countries.items():
        code = value[1]
        if len(code) == 2:
            code += "\\0"
        data_temp_file.write('"%2s" // %s\n' % (code, value[0]))
    data_temp_file.write(";\n")

    data_temp_file.write("\n")
    data_temp_file.write(GENERATED_BLOCK_END)
    s = qlocaledata_file.readline()
    # skip until end of the old block
    while s and s != GENERATED_BLOCK_END:
        s = qlocaledata_file.readline()

    s = qlocaledata_file.readline()
    while s:
        data_temp_file.write(s)
        s = qlocaledata_file.readline()
    data_temp_file.close()
    qlocaledata_file.close()

    os.remove(qtsrcdir + "/src/corelib/text/qlocale_data_p.h")
    os.rename(data_temp_file_path, qtsrcdir + "/src/corelib/text/qlocale_data_p.h")

    # Generate calendar data
    calendar_format = '      {%6d,%6d,%6d,{%5s},{%5s},{%5s},{%5s},{%5s},{%5s}}, '
    for calendar, stem in calendars.items():
        months_data = StringData('months_data')
        calendar_data_file = "q%scalendar_data_p.h" % stem
        calendar_template_file = open(os.path.join(qtsrcdir, 'src', 'corelib', 'time',
                                                   calendar_data_file), "r")
        (calendar_temp_file, calendar_temp_file_path) = tempfile.mkstemp(calendar_data_file, dir=qtsrcdir)
        calendar_temp_file = os.fdopen(calendar_temp_file, "w")
        s = calendar_template_file.readline()
        while s and s != GENERATED_BLOCK_START:
            calendar_temp_file.write(s)
            s = calendar_template_file.readline()
        calendar_temp_file.write(GENERATED_BLOCK_START)
        calendar_temp_file.write(generated_template % (datetime.date.today(), reader.cldrVersion))
        calendar_temp_file.write("static const QCalendarLocale locale_data[] = {\n")
        calendar_temp_file.write('   // '
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
        for key in locale_keys:
            l = locale_map[key]
            calendar_temp_file.write(
                calendar_format
                % (key[0], key[1], key[2],
                   months_data.append(l.standaloneShortMonths[calendar]),
                   months_data.append(l.standaloneLongMonths[calendar]),
                   months_data.append(l.standaloneNarrowMonths[calendar]),
                   months_data.append(l.shortMonths[calendar]),
                   months_data.append(l.longMonths[calendar]),
                   months_data.append(l.narrowMonths[calendar]))
                + "// %s/%s/%s\n " % (l.language, l.script, l.country))
        calendar_temp_file.write(calendar_format % ( (0,) * 3 + ('0,0',) * 6 )
                                      + '// trailing zeros\n')
        calendar_temp_file.write("};\n")
        months_data.write(calendar_temp_file)
        s = calendar_template_file.readline()
        while s and s != GENERATED_BLOCK_END:
            s = calendar_template_file.readline()
        while s:
            calendar_temp_file.write(s)
            s = calendar_template_file.readline()
        os.rename(calendar_temp_file_path,
                  os.path.join(qtsrcdir, 'src', 'corelib', 'time', calendar_data_file))

    # qlocale.h

    (qlocaleh_temp_file, qlocaleh_temp_file_path) = tempfile.mkstemp("qlocale.h", dir=qtsrcdir)
    qlocaleh_temp_file = os.fdopen(qlocaleh_temp_file, "w")
    qlocaleh_file = open(qtsrcdir + "/src/corelib/text/qlocale.h", "r")
    s = qlocaleh_file.readline()
    while s and s != GENERATED_BLOCK_START:
        qlocaleh_temp_file.write(s)
        s = qlocaleh_file.readline()
    qlocaleh_temp_file.write(GENERATED_BLOCK_START)
    qlocaleh_temp_file.write("// see qlocale_data_p.h for more info on generated data\n")

    # Language enum
    qlocaleh_temp_file.write("    enum Language {\n")
    language = None
    for key, value in reader.languages.items():
        language = fixedLanguageName(value[0], reader.dupes)
        qlocaleh_temp_file.write("        " + language + " = " + str(key) + ",\n")

    qlocaleh_temp_file.write("\n        " +
                             ",\n        ".join('%s = %s' % pair
                                                for pair in sorted(language_aliases.items())) +
                             ",\n")
    qlocaleh_temp_file.write("\n")
    qlocaleh_temp_file.write("        LastLanguage = " + language + "\n")
    qlocaleh_temp_file.write("    };\n")

    qlocaleh_temp_file.write("\n")

    # Script enum
    qlocaleh_temp_file.write("    enum Script {\n")
    script = None
    for key, value in reader.scripts.items():
        script = fixedScriptName(value[0], reader.dupes)
        qlocaleh_temp_file.write("        " + script + " = " + str(key) + ",\n")
    qlocaleh_temp_file.write("\n        " +
                             ",\n        ".join('%s = %s' % pair
                                                for pair in sorted(script_aliases.items())) +
                             ",\n")
    qlocaleh_temp_file.write("\n")
    qlocaleh_temp_file.write("        LastScript = " + script + "\n")
    qlocaleh_temp_file.write("    };\n")

    # Country enum
    qlocaleh_temp_file.write("    enum Country {\n")
    country = None
    for key, value in reader.countries.items():
        country = fixedCountryName(value[0], reader.dupes)
        qlocaleh_temp_file.write("        " + country + " = " + str(key) + ",\n")
    qlocaleh_temp_file.write("\n        " +
                             ",\n        ".join('%s = %s' % pair
                                                for pair in sorted(country_aliases.items())) +
                             ",\n")
    qlocaleh_temp_file.write("\n")
    qlocaleh_temp_file.write("        LastCountry = " + country + "\n")
    qlocaleh_temp_file.write("    };\n")

    qlocaleh_temp_file.write(GENERATED_BLOCK_END)
    s = qlocaleh_file.readline()
    # skip until end of the old block
    while s and s != GENERATED_BLOCK_END:
        s = qlocaleh_file.readline()

    s = qlocaleh_file.readline()
    while s:
        qlocaleh_temp_file.write(s)
        s = qlocaleh_file.readline()
    qlocaleh_temp_file.close()
    qlocaleh_file.close()

    os.remove(qtsrcdir + "/src/corelib/text/qlocale.h")
    os.rename(qlocaleh_temp_file_path, qtsrcdir + "/src/corelib/text/qlocale.h")

    # qlocale.qdoc

    (qlocaleqdoc_temp_file, qlocaleqdoc_temp_file_path) = tempfile.mkstemp("qlocale.qdoc", dir=qtsrcdir)
    qlocaleqdoc_temp_file = os.fdopen(qlocaleqdoc_temp_file, "w")
    qlocaleqdoc_file = open(qtsrcdir + "/src/corelib/text/qlocale.qdoc", "r")
    s = qlocaleqdoc_file.readline()
    DOCSTRING = "    QLocale's data is based on Common Locale Data Repository "
    while s:
        if DOCSTRING in s:
            qlocaleqdoc_temp_file.write(DOCSTRING + "v" + reader.cldrVersion + ".\n")
        else:
            qlocaleqdoc_temp_file.write(s)
        s = qlocaleqdoc_file.readline()
    qlocaleqdoc_temp_file.close()
    qlocaleqdoc_file.close()

    os.remove(qtsrcdir + "/src/corelib/text/qlocale.qdoc")
    os.rename(qlocaleqdoc_temp_file_path, qtsrcdir + "/src/corelib/text/qlocale.qdoc")

if __name__ == "__main__":
    main()
