#!/usr/bin/env python2
#############################################################################
##
## Copyright (C) 2017 The Qt Company Ltd.
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
import xml.dom.minidom

from localexml import Locale

class Error:
    def __init__(self, msg):
        self.msg = msg
    def __str__(self):
        return self.msg

def wrap_list(lst):
    def split(lst, size):
        while lst:
            head, lst = lst[:size], lst[size:]
            yield head
    return ",\n".join(", ".join(x) for x in split(lst, 20))

def isNodeNamed(elt, name, TYPE=xml.dom.minidom.Node.ELEMENT_NODE):
    return elt.nodeType == TYPE and elt.nodeName == name

def firstChildElt(parent, name):
    child = parent.firstChild
    while child:
        if isNodeNamed(child, name):
            return child
        child = child.nextSibling

    raise Error('No %s child found' % name)

def eachEltInGroup(parent, group, key):
    try:
        element = firstChildElt(parent, group).firstChild
    except Error:
        element = None

    while element:
        if isNodeNamed(element, key):
            yield element
        element = element.nextSibling

def eltWords(elt):
    child = elt.firstChild
    while child:
        if child.nodeType == elt.TEXT_NODE:
            yield child.nodeValue
        child = child.nextSibling

def firstChildText(elt, key):
    return ' '.join(eltWords(firstChildElt(elt, key)))

def loadMap(doc, category):
    return dict((int(firstChildText(element, 'id')),
                 (firstChildText(element, 'name'),
                  firstChildText(element, 'code')))
                for element in eachEltInGroup(doc.documentElement,
                                              category + 'List', category))

def loadLikelySubtagsMap(doc):
    def triplet(element, keys=('language', 'script', 'country')):
        return tuple(firstChildText(element, key) for key in keys)

    return dict((i, {'from': triplet(firstChildElt(elt, "from")),
                     'to': triplet(firstChildElt(elt, "to"))})
                for i, elt in enumerate(eachEltInGroup(doc.documentElement,
                                                       'likelySubtags', 'likelySubtag')))

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

def findDupes(country_map, language_map):
    country_set = set(v[0] for a, v in country_map.iteritems())
    language_set = set(v[0] for a, v in language_map.iteritems())
    return country_set & language_set

def languageNameToId(name, language_map):
    for key in language_map.keys():
        if language_map[key][0] == name:
            return key
    return -1

def scriptNameToId(name, script_map):
    for key in script_map.keys():
        if script_map[key][0] == name:
            return key
    return -1

def countryNameToId(name, country_map):
    for key in country_map.keys():
        if country_map[key][0] == name:
            return key
    return -1

def loadLocaleMap(doc, language_map, script_map, country_map, likely_subtags_map):
    result = {}

    for locale_elt in eachEltInGroup(doc.documentElement, "localeList", "locale"):
        locale = Locale.fromXmlData(lambda k: firstChildText(locale_elt, k))
        language_id = languageNameToId(locale.language, language_map)
        if language_id == -1:
            sys.stderr.write("Cannot find a language id for '%s'\n" % locale.language)
        script_id = scriptNameToId(locale.script, script_map)
        if script_id == -1:
            sys.stderr.write("Cannot find a script id for '%s'\n" % locale.script)
        country_id = countryNameToId(locale.country, country_map)
        if country_id == -1:
            sys.stderr.write("Cannot find a country id for '%s'\n" % locale.country)

        if language_id != 1: # C
            if country_id == 0:
                sys.stderr.write("loadLocaleMap: No country id for '%s'\n" % locale.language)

            if script_id == 0:
                # find default script for a given language and country (see http://www.unicode.org/reports/tr35/#Likely_Subtags)
                for key in likely_subtags_map.keys():
                    tmp = likely_subtags_map[key]
                    if tmp["from"][0] == locale.language and tmp["from"][1] == "AnyScript" and tmp["from"][2] == locale.country:
                        locale.script = tmp["to"][1]
                        script_id = scriptNameToId(locale.script, script_map)
                        break
            if script_id == 0 and country_id != 0:
                # try with no country
                for key in likely_subtags_map.keys():
                    tmp = likely_subtags_map[key]
                    if tmp["from"][0] == locale.language and tmp["from"][1] == "AnyScript" and tmp["from"][2] == "AnyCountry":
                        locale.script = tmp["to"][1]
                        script_id = scriptNameToId(locale.script, script_map)
                        break

        result[(language_id, script_id, country_id)] = locale

    return result

def compareLocaleKeys(key1, key2):
    if key1 == key2:
        return 0

    if key1[0] == key2[0]:
        l1 = compareLocaleKeys.locale_map[key1]
        l2 = compareLocaleKeys.locale_map[key2]

        if (l1.language, l1.script) in compareLocaleKeys.default_map.keys():
            default = compareLocaleKeys.default_map[(l1.language, l1.script)]
            if l1.country == default:
                return -1
            if l2.country == default:
                return 1

        if key1[1] != key2[1]:
            if (l2.language, l2.script) in compareLocaleKeys.default_map.keys():
                default = compareLocaleKeys.default_map[(l2.language, l2.script)]
                if l2.country == default:
                    return 1
                if l1.country == default:
                    return -1

        if key1[1] != key2[1]:
            return key1[1] - key2[1]
    else:
        return key1[0] - key2[0]

    return key1[2] - key2[2]


def languageCount(language_id, locale_map):
    result = 0
    for key in locale_map.keys():
        if key[0] == language_id:
            result += 1
    return result

def unicode2hex(s):
    lst = []
    for x in s:
        v = ord(x)
        if v > 0xFFFF:
            # make a surrogate pair
            # copied from qchar.h
            high = (v >> 10) + 0xd7c0
            low = (v % 0x400 + 0xdc00)
            lst.append(hex(high))
            lst.append(hex(low))
        else:
            lst.append(hex(v))
    return lst

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

def escapedString(s):
    result = ""
    i = 0
    while i < len(s):
        if s[i] == '"':
            result += '\\"'
            i += 1
        else:
            result += s[i]
            i += 1
    s = result

    line = ""
    need_escape = False
    result = ""
    for c in s:
        if ord(c) < 128 and (not need_escape or ord(c.lower()) < ord('a') or ord(c.lower()) > ord('f')):
            line += c
            need_escape = False
        else:
            line += "\\x%02x" % (ord(c))
            need_escape = True
        if len(line) > 80:
            result = result + "\n" + '"' + line + '"'
            line = ""
    line += "\\0"
    result = result + "\n" + '"' + line + '"'
    if result[0] == "\n":
        result = result[1:]
    return result

def printEscapedString(s):
    print escapedString(s)

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

    localexml = sys.argv[1]
    qtsrcdir = sys.argv[2]

    if not (os.path.isdir(qtsrcdir)
            and all(os.path.isfile(os.path.join(qtsrcdir, 'src', 'corelib', 'tools', leaf))
                    for leaf in ('qlocale_data_p.h', 'qlocale.h', 'qlocale.qdoc'))):
        usage()

    (data_temp_file, data_temp_file_path) = tempfile.mkstemp("qlocale_data_p", dir=qtsrcdir)
    data_temp_file = os.fdopen(data_temp_file, "w")
    qlocaledata_file = open(qtsrcdir + "/src/corelib/tools/qlocale_data_p.h", "r")
    s = qlocaledata_file.readline()
    while s and s != GENERATED_BLOCK_START:
        data_temp_file.write(s)
        s = qlocaledata_file.readline()
    data_temp_file.write(GENERATED_BLOCK_START)

    doc = xml.dom.minidom.parse(localexml)
    language_map = loadMap(doc, 'language')
    script_map = loadMap(doc, 'script')
    country_map = loadMap(doc, 'country')
    likely_subtags_map = loadLikelySubtagsMap(doc)
    default_map = {}
    for key in likely_subtags_map.keys():
        tmp = likely_subtags_map[key]
        if tmp["from"][1] == "AnyScript" and tmp["from"][2] == "AnyCountry" and tmp["to"][2] != "AnyCountry":
            default_map[(tmp["to"][0], tmp["to"][1])] = tmp["to"][2]
    locale_map = loadLocaleMap(doc, language_map, script_map, country_map, likely_subtags_map)
    dupes = findDupes(language_map, country_map)

    cldr_version = firstChildText(doc.documentElement, "version")

    data_temp_file.write("""
/*
    This part of the file was generated on %s from the
    Common Locale Data Repository v%s

    http://www.unicode.org/cldr/

    Do not edit this section: instead regenerate it using
    cldr2qlocalexml.py and qlocalexml2cpp.py on updated (or
    edited) CLDR data; see qtbase/util/local_database/.
*/

""" % (str(datetime.date.today()), cldr_version) )

    # Likely subtags map
    data_temp_file.write("static const QLocaleId likely_subtags[] = {\n")
    index = 0
    for key in likely_subtags_map.keys():
        tmp = likely_subtags_map[key]
        from_language = languageNameToId(tmp["from"][0], language_map)
        from_script = scriptNameToId(tmp["from"][1], script_map)
        from_country = countryNameToId(tmp["from"][2], country_map)
        to_language = languageNameToId(tmp["to"][0], language_map)
        to_script = scriptNameToId(tmp["to"][1], script_map)
        to_country = countryNameToId(tmp["to"][2], country_map)

        cmnt_from = ""
        if from_language != 0:
            cmnt_from = cmnt_from + language_map[from_language][1]
        else:
            cmnt_from = cmnt_from + "und"
        if from_script != 0:
            if cmnt_from:
                cmnt_from = cmnt_from + "_"
            cmnt_from = cmnt_from + script_map[from_script][1]
        if from_country != 0:
            if cmnt_from:
                cmnt_from = cmnt_from + "_"
            cmnt_from = cmnt_from + country_map[from_country][1]
        cmnt_to = ""
        if to_language != 0:
            cmnt_to = cmnt_to + language_map[to_language][1]
        else:
            cmnt_to = cmnt_to + "und"
        if to_script != 0:
            if cmnt_to:
                cmnt_to = cmnt_to + "_"
            cmnt_to = cmnt_to + script_map[to_script][1]
        if to_country != 0:
            if cmnt_to:
                cmnt_to = cmnt_to + "_"
            cmnt_to = cmnt_to + country_map[to_country][1]

        data_temp_file.write("    ")
        data_temp_file.write("{ %3d, %3d, %3d }, { %3d, %3d, %3d }" % (from_language, from_script, from_country, to_language, to_script, to_country))
        index += 1
        if index != len(likely_subtags_map):
            data_temp_file.write(",")
        else:
            data_temp_file.write(" ")
        data_temp_file.write(" // %s -> %s\n" % (cmnt_from, cmnt_to))
    data_temp_file.write("};\n")

    data_temp_file.write("\n")

    # Locale index
    data_temp_file.write("static const quint16 locale_index[] = {\n")
    index = 0
    for key in language_map.keys():
        i = 0
        count = languageCount(key, locale_map)
        if count > 0:
            i = index
            index += count
        data_temp_file.write("%6d, // %s\n" % (i, language_map[key][0]))
    data_temp_file.write("     0 // trailing 0\n")
    data_temp_file.write("};\n\n")

    list_pattern_part_data = StringData('list_pattern_part_data')
    date_format_data = StringData('date_format_data')
    time_format_data = StringData('time_format_data')
    months_data = StringData('months_data')
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
                         + '  ssMonth   ' # Months
                         + '  slMonth   '
                         + '  snMonth   '
                         + '   sMonth   '
                         + '   lMonth   '
                         + '   nMonth   '
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
    compareLocaleKeys.default_map = default_map
    compareLocaleKeys.locale_map = locale_map
    locale_keys.sort(compareLocaleKeys)

    line_format = ('    { '
                   # Locale-identifier:
                   + '%6d,' * 3
                   # Numeric formats, list delimiter:
                   + '%6d,' * 8
                   # Quotation marks:
                   + '%8d,' * 4
                   # List patterns, date/time formats, month/day names, am/pm:
                   + '%11s,' * 22
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
                        months_data.append(l.standaloneShortMonths),
                        months_data.append(l.standaloneLongMonths),
                        months_data.append(l.standaloneNarrowMonths),
                        months_data.append(l.shortMonths),
                        months_data.append(l.longMonths),
                        months_data.append(l.narrowMonths),
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
                         % ( (0,) * (3 + 8 + 4) + ("0,0",) * (22 + 3)
                             + (currencyIsoCodeData(0),)
                             + ("0,0",) * 6 + (0,) * (2 + 3))
                         + " // trailing 0s\n")
    data_temp_file.write("};\n")

    # StringData tables:
    for data in (list_pattern_part_data, date_format_data,
                 time_format_data, months_data, days_data,
                 byte_unit_data, am_data, pm_data, currency_symbol_data,
                 currency_display_name_data, currency_format_data,
                 endonyms_data):
        data_temp_file.write("\nstatic const ushort %s[] = {\n" % data.name)
        data_temp_file.write(wrap_list(data.data))
        data_temp_file.write("\n};\n")

    data_temp_file.write("\n")

    # Language name list
    data_temp_file.write("static const char language_name_list[] =\n")
    data_temp_file.write('"Default\\0"\n')
    for key in language_map.keys():
        if key == 0:
            continue
        data_temp_file.write('"' + language_map[key][0] + '\\0"\n')
    data_temp_file.write(";\n")

    data_temp_file.write("\n")

    # Language name index
    data_temp_file.write("static const quint16 language_name_index[] = {\n")
    data_temp_file.write("     0, // AnyLanguage\n")
    index = 8
    for key in language_map.keys():
        if key == 0:
            continue
        language = language_map[key][0]
        data_temp_file.write("%6d, // %s\n" % (index, language))
        index += len(language) + 1
    data_temp_file.write("};\n")

    data_temp_file.write("\n")

    # Script name list
    data_temp_file.write("static const char script_name_list[] =\n")
    data_temp_file.write('"Default\\0"\n')
    for key in script_map.keys():
        if key == 0:
            continue
        data_temp_file.write('"' + script_map[key][0] + '\\0"\n')
    data_temp_file.write(";\n")

    data_temp_file.write("\n")

    # Script name index
    data_temp_file.write("static const quint16 script_name_index[] = {\n")
    data_temp_file.write("     0, // AnyScript\n")
    index = 8
    for key in script_map.keys():
        if key == 0:
            continue
        script = script_map[key][0]
        data_temp_file.write("%6d, // %s\n" % (index, script))
        index += len(script) + 1
    data_temp_file.write("};\n")

    data_temp_file.write("\n")

    # Country name list
    data_temp_file.write("static const char country_name_list[] =\n")
    data_temp_file.write('"Default\\0"\n')
    for key in country_map.keys():
        if key == 0:
            continue
        data_temp_file.write('"' + country_map[key][0] + '\\0"\n')
    data_temp_file.write(";\n")

    data_temp_file.write("\n")

    # Country name index
    data_temp_file.write("static const quint16 country_name_index[] = {\n")
    data_temp_file.write("     0, // AnyCountry\n")
    index = 8
    for key in country_map.keys():
        if key == 0:
            continue
        country = country_map[key][0]
        data_temp_file.write("%6d, // %s\n" % (index, country))
        index += len(country) + 1
    data_temp_file.write("};\n")

    data_temp_file.write("\n")

    # Language code list
    data_temp_file.write("static const unsigned char language_code_list[] =\n")
    for key in language_map.keys():
        code = language_map[key][1]
        if len(code) == 2:
            code += r"\0"
        data_temp_file.write('"%2s" // %s\n' % (code, language_map[key][0]))
    data_temp_file.write(";\n")

    data_temp_file.write("\n")

    # Script code list
    data_temp_file.write("static const unsigned char script_code_list[] =\n")
    for key in script_map.keys():
        code = script_map[key][1]
        for i in range(4 - len(code)):
            code += "\\0"
        data_temp_file.write('"%2s" // %s\n' % (code, script_map[key][0]))
    data_temp_file.write(";\n")

    # Country code list
    data_temp_file.write("static const unsigned char country_code_list[] =\n")
    for key in country_map.keys():
        code = country_map[key][1]
        if len(code) == 2:
            code += "\\0"
        data_temp_file.write('"%2s" // %s\n' % (code, country_map[key][0]))
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

    os.remove(qtsrcdir + "/src/corelib/tools/qlocale_data_p.h")
    os.rename(data_temp_file_path, qtsrcdir + "/src/corelib/tools/qlocale_data_p.h")

    # qlocale.h

    (qlocaleh_temp_file, qlocaleh_temp_file_path) = tempfile.mkstemp("qlocale.h", dir=qtsrcdir)
    qlocaleh_temp_file = os.fdopen(qlocaleh_temp_file, "w")
    qlocaleh_file = open(qtsrcdir + "/src/corelib/tools/qlocale.h", "r")
    s = qlocaleh_file.readline()
    while s and s != GENERATED_BLOCK_START:
        qlocaleh_temp_file.write(s)
        s = qlocaleh_file.readline()
    qlocaleh_temp_file.write(GENERATED_BLOCK_START)
    qlocaleh_temp_file.write("// see qlocale_data_p.h for more info on generated data\n")

    # Language enum
    qlocaleh_temp_file.write("    enum Language {\n")
    language = ""
    for key in language_map.keys():
        language = fixedLanguageName(language_map[key][0], dupes)
        qlocaleh_temp_file.write("        " + language + " = " + str(key) + ",\n")
    # legacy. should disappear at some point
    qlocaleh_temp_file.write("\n")
    qlocaleh_temp_file.write("        Norwegian = NorwegianBokmal,\n")
    qlocaleh_temp_file.write("        Moldavian = Romanian,\n")
    qlocaleh_temp_file.write("        SerboCroatian = Serbian,\n")
    qlocaleh_temp_file.write("        Tagalog = Filipino,\n")
    qlocaleh_temp_file.write("        Twi = Akan,\n")
    # renamings
    qlocaleh_temp_file.write("        Afan = Oromo,\n")
    qlocaleh_temp_file.write("        Byelorussian = Belarusian,\n")
    qlocaleh_temp_file.write("        Bhutani = Dzongkha,\n")
    qlocaleh_temp_file.write("        Cambodian = Khmer,\n")
    qlocaleh_temp_file.write("        Kurundi = Rundi,\n")
    qlocaleh_temp_file.write("        RhaetoRomance = Romansh,\n")
    qlocaleh_temp_file.write("        Chewa = Nyanja,\n")
    qlocaleh_temp_file.write("        Frisian = WesternFrisian,\n")
    qlocaleh_temp_file.write("        Uigur = Uighur,\n")
    qlocaleh_temp_file.write("\n")
    qlocaleh_temp_file.write("        LastLanguage = " + language + "\n")
    qlocaleh_temp_file.write("    };\n")

    qlocaleh_temp_file.write("\n")

    # Script enum
    qlocaleh_temp_file.write("    enum Script {\n")
    script = ""
    for key in script_map.keys():
        script = fixedScriptName(script_map[key][0], dupes)
        qlocaleh_temp_file.write("        " + script + " = " + str(key) + ",\n")
    # renamings
    qlocaleh_temp_file.write("\n")
    qlocaleh_temp_file.write("        SimplifiedChineseScript = SimplifiedHanScript,\n")
    qlocaleh_temp_file.write("        TraditionalChineseScript = TraditionalHanScript,\n")
    qlocaleh_temp_file.write("\n")
    qlocaleh_temp_file.write("        LastScript = " + script + "\n")
    qlocaleh_temp_file.write("    };\n")

    # Country enum
    qlocaleh_temp_file.write("    enum Country {\n")
    country = ""
    for key in country_map.keys():
        country = fixedCountryName(country_map[key][0], dupes)
        qlocaleh_temp_file.write("        " + country + " = " + str(key) + ",\n")
    # deprecated
    qlocaleh_temp_file.write("\n")
    qlocaleh_temp_file.write("        Tokelau = TokelauCountry,\n")
    qlocaleh_temp_file.write("        Tuvalu = TuvaluCountry,\n")
    # renamings
    qlocaleh_temp_file.write("        DemocraticRepublicOfCongo = CongoKinshasa,\n")
    qlocaleh_temp_file.write("        PeoplesRepublicOfCongo = CongoBrazzaville,\n")
    qlocaleh_temp_file.write("        DemocraticRepublicOfKorea = NorthKorea,\n")
    qlocaleh_temp_file.write("        RepublicOfKorea = SouthKorea,\n")
    qlocaleh_temp_file.write("        RussianFederation = Russia,\n")
    qlocaleh_temp_file.write("        SyrianArabRepublic = Syria,\n")
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

    os.remove(qtsrcdir + "/src/corelib/tools/qlocale.h")
    os.rename(qlocaleh_temp_file_path, qtsrcdir + "/src/corelib/tools/qlocale.h")

    # qlocale.qdoc

    (qlocaleqdoc_temp_file, qlocaleqdoc_temp_file_path) = tempfile.mkstemp("qlocale.qdoc", dir=qtsrcdir)
    qlocaleqdoc_temp_file = os.fdopen(qlocaleqdoc_temp_file, "w")
    qlocaleqdoc_file = open(qtsrcdir + "/src/corelib/tools/qlocale.qdoc", "r")
    s = qlocaleqdoc_file.readline()
    DOCSTRING="    QLocale's data is based on Common Locale Data Repository "
    while s:
        if DOCSTRING in s:
            qlocaleqdoc_temp_file.write(DOCSTRING + "v" + cldr_version + ".\n")
        else:
            qlocaleqdoc_temp_file.write(s)
        s = qlocaleqdoc_file.readline()
    qlocaleqdoc_temp_file.close()
    qlocaleqdoc_file.close()

    os.remove(qtsrcdir + "/src/corelib/tools/qlocale.qdoc")
    os.rename(qlocaleqdoc_temp_file_path, qtsrcdir + "/src/corelib/tools/qlocale.qdoc")

if __name__ == "__main__":
    main()
