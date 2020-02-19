# coding=utf8
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
"""Shared serialization-scanning code for QLocaleXML format.

Provides classes:
  Locale -- common data-type representing one locale as a namespace
  QLocaleXmlWriter -- helper to write a QLocaleXML file
  QLocaleXmlReader -- helper to read a QLocaleXML file back in

Support:
  Spacer -- provides control over indentation of the output.
"""
from __future__ import print_function
from xml.sax.saxutils import escape

from localetools import Error

# Tools used by Locale:
def camel(seq):
    yield seq.next()
    for word in seq:
        yield word.capitalize()

def camelCase(words):
    return ''.join(camel(iter(words)))

def addEscapes(s):
    return ''.join(c if n < 128 else '\\x{:02x}'.format(n)
                   for n, c in ((ord(c), c) for c in s))

def ordStr(c):
    if len(c) == 1:
        return str(ord(c))
    raise Error('Unable to handle value "{}"'.format(addEscapes(c)))

# Fix for a problem with QLocale returning a character instead of
# strings for QLocale::exponential() and others. So we fallback to
# default values in these cases.
def fixOrdStr(c, d):
    return str(ord(c if len(c) == 1 else d))

def startCount(c, text): # strspn
    """First index in text where it doesn't have a character in c"""
    assert text and text[0] in c
    try:
        return (j for j, d in enumerate(text) if d not in c).next()
    except StopIteration:
        return len(text)

def convertFormat(format):
    """Convert date/time format-specier from CLDR to Qt

    Match up (as best we can) the differences between:
    * https://www.unicode.org/reports/tr35/tr35-dates.html#Date_Field_Symbol_Table
    * QDateTimeParser::parseFormat() and QLocalePrivate::dateTimeToString()
    """
    # Compare and contrast dateconverter.py's convert_date().
    # Need to (check consistency and) reduce redundancy !
    result = ""
    i = 0
    while i < len(format):
        if format[i] == "'":
            result += "'"
            i += 1
            while i < len(format) and format[i] != "'":
                result += format[i]
                i += 1
            if i < len(format):
                result += "'"
                i += 1
        else:
            s = format[i:]
            if s.startswith('E'): # week-day
                n = startCount('E', s)
                if n < 3:
                    result += 'ddd'
                elif n == 4:
                    result += 'dddd'
                else: # 5: narrow, 6 short; but should be name, not number :-(
                    result += 'd' if n < 6 else 'dd'
                i += n
            elif s[0] in 'ab': # am/pm
                # 'b' should distinguish noon/midnight, too :-(
                result += "AP"
                i += startCount('ab', s)
            elif s.startswith('S'): # fractions of seconds: count('S') == number of decimals to show
                result += 'z'
                i += startCount('S', s)
            elif s.startswith('V'): # long time zone specifiers (and a deprecated short ID)
                result += 't'
                i += startCount('V', s)
            elif s[0] in 'zv': # zone
                # Should use full name, e.g. "Central European Time", if 'zzzz' :-(
                # 'v' should get generic non-location format, e.g. PT for "Pacific Time", no DST indicator
                result += "t"
                i += startCount('zv', s)
            else:
                result += format[i]
                i += 1

    return result

class QLocaleXmlReader (object):
    def __init__(self, filename):
        self.root = self.__parse(filename)
        # Lists of (id, name, code) triples:
        languages = tuple(self.__loadMap('language'))
        scripts = tuple(self.__loadMap('script'))
        countries = tuple(self.__loadMap('country'))
        self.__likely = tuple(self.__likelySubtagsMap())
        # Mappings {ID: (name, code)}
        self.languages = dict((v[0], v[1:]) for v in languages)
        self.scripts = dict((v[0], v[1:]) for v in scripts)
        self.countries = dict((v[0], v[1:]) for v in countries)
        # Private mappings {name: (ID, code)}
        self.__langByName = dict((v[1], (v[0], v[2])) for v in languages)
        self.__textByName = dict((v[1], (v[0], v[2])) for v in scripts)
        self.__landByName = dict((v[1], (v[0], v[2])) for v in countries)
        # Other properties:
        self.dupes = set(v[1] for v in languages) & set(v[1] for v in countries)
        self.cldrVersion = self.__firstChildText(self.root, "version")

    def loadLocaleMap(self, calendars, grumble = lambda text: None):
        kid = self.__firstChildText
        likely = dict(self.__likely)
        for elt in self.__eachEltInGroup(self.root, 'localeList', 'locale'):
            locale = Locale.fromXmlData(lambda k: kid(elt, k), calendars)
            language = self.__langByName[locale.language][0]
            script = self.__textByName[locale.script][0]
            country = self.__landByName[locale.country][0]

            if language != 1: # C
                if country == 0:
                    grumble('loadLocaleMap: No country id for "{}"\n'.format(locale.language))

                if script == 0:
                    # Find default script for the given language and country - see:
                    # http://www.unicode.org/reports/tr35/#Likely_Subtags
                    try:
                        try:
                            to = likely[(locale.language, 'AnyScript', locale.country)]
                        except KeyError:
                            to = likely[(locale.language, 'AnyScript', 'AnyCountry')]
                    except KeyError:
                        pass
                    else:
                        locale.script = to[1]
                        script = self.__textByName[locale.script][0]

            yield (language, script, country), locale

    def languageIndices(self, locales):
        index = 0
        for key, value in self.languages.iteritems():
            i, count = 0, locales.count(key)
            if count > 0:
                i = index
                index += count
            yield i, value[0]

    def likelyMap(self):
        def tag(t):
            lang, script, land = t
            yield lang[1] if lang[0] else 'und'
            if script[0]: yield script[1]
            if land[0]: yield land[1]

        def ids(t):
            return tuple(x[0] for x in t)

        for i, pair in enumerate(self.__likely, 1):
            have = self.__fromNames(pair[0])
            give = self.__fromNames(pair[1])
            yield ('_'.join(tag(have)), ids(have),
                   '_'.join(tag(give)), ids(give),
                   i == len(self.__likely))

    def defaultMap(self):
        """Map language and script to their default country by ID.

        Yields ((language, script), country) wherever the likely
        sub-tags mapping says language's default locale uses the given
        script and country."""
        for have, give in self.__likely:
            if have[1:] == ('AnyScript', 'AnyCountry') and give[2] != 'AnyCountry':
                assert have[0] == give[0], (have, give)
                yield ((self.__langByName[give[0]][0],
                        self.__textByName[give[1]][0]),
                       self.__landByName[give[2]][0])

    # Implementation details:
    def __loadMap(self, category):
        kid = self.__firstChildText
        for element in self.__eachEltInGroup(self.root, category + 'List', category):
            yield int(kid(element, 'id')), kid(element, 'name'), kid(element, 'code')

    def __likelySubtagsMap(self):
        def triplet(element, keys=('language', 'script', 'country'), kid = self.__firstChildText):
            return tuple(kid(element, key) for key in keys)

        kid = self.__firstChildElt
        for elt in self.__eachEltInGroup(self.root, 'likelySubtags', 'likelySubtag'):
            yield triplet(kid(elt, "from")), triplet(kid(elt, "to"))

    def __fromNames(self, names):
        return self.__langByName[names[0]], self.__textByName[names[1]], self.__landByName[names[2]]

    # DOM access:
    from xml.dom import minidom
    @staticmethod
    def __parse(filename, read = minidom.parse):
        return read(filename).documentElement

    @staticmethod
    def __isNodeNamed(elt, name, TYPE=minidom.Node.ELEMENT_NODE):
        return elt.nodeType == TYPE and elt.nodeName == name
    del minidom

    @staticmethod
    def __eltWords(elt):
        child = elt.firstChild
        while child:
            if child.nodeType == elt.TEXT_NODE:
                yield child.nodeValue
            child = child.nextSibling

    @classmethod
    def __firstChildElt(cls, parent, name):
        child = parent.firstChild
        while child:
            if cls.__isNodeNamed(child, name):
                return child
            child = child.nextSibling

        raise Error('No {} child found'.format(name))

    @classmethod
    def __firstChildText(cls, elt, key):
        return ' '.join(cls.__eltWords(cls.__firstChildElt(elt, key)))

    @classmethod
    def __eachEltInGroup(cls, parent, group, key):
        try:
            element = cls.__firstChildElt(parent, group).firstChild
        except Error:
            element = None

        while element:
            if cls.__isNodeNamed(element, key):
                yield element
            element = element.nextSibling


class Spacer (object):
    def __init__(self, indent = None, initial = ''):
        """Prepare to manage indentation and line breaks.

        Arguments are both optional.

        First argument, indent, is either None (its default, for
        'minifying'), an ingeter (number of spaces) or the unit of
        text that is to be used for each indentation level (e.g. '\t'
        to use tabs).  If indent is None, no indentation is added, nor
        are line-breaks; otherwise, self(text), for non-empty text,
        shall end with a newline and begin with indentation.

        Second argument, initial, is the initial indentation; it is
        ignored if indent is None.  Indentation increases after each
        call to self(text) in which text starts with a tag and doesn't
        include its end-tag; indentation decreases if text starts with
        an end-tag.  The text is not parsed any more carefully than
        just described.
        """
        if indent is None:
            self.__call = lambda x: x
        else:
            self.__each = ' ' * indent if isinstance(indent, int) else indent
            self.current = initial
            self.__call = self.__wrap

    def __wrap(self, line):
        if not line:
            return '\n'

        indent = self.current
        if line.startswith('</'):
            indent = self.current = indent[:-len(self.__each)]
        elif line.startswith('<') and not line.startswith('<!'):
            cut = line.find('>')
            tag = (line[1:] if cut < 0 else line[1 : cut]).strip().split()[0]
            if '</{}>'.format(tag) not in line:
                self.current += self.__each
        return indent + line + '\n'

    def __call__(self, line):
        return self.__call(line)

class QLocaleXmlWriter (object):
    def __init__(self, save = None, space = Spacer(4)):
        """Set up to write digested CLDR data as QLocale XML.

        Arguments are both optional.

        First argument, save, is None (its default) or a callable that
        will write content to where you intend to save it. If None, it
        is replaced with a callable that prints the given content,
        suppressing the newline (but see the following); this is
        equivalent to passing sys.stdout.write.

        Second argument, space, is an object to call on each text
        output to prepend indentation and append newlines, or not as
        the case may be. The default is a Spacer(4), which grows
        indent by four spaces after each unmatched new tag and shrinks
        back on a close-tag (its parsing is naive, but adequate to how
        this class uses it), while adding a newline to each line.
        """
        self.__rawOutput = self.__printit if save is None else save
        self.__wrap = space
        self.__write('<localeDatabase>')

    # Output of various sections, in their usual order:
    def enumData(self, languages, scripts, countries):
        self.__enumTable('languageList', languages)
        self.__enumTable('scriptList', scripts)
        self.__enumTable('countryList', countries)

    def likelySubTags(self, entries):
        self.__openTag('likelySubtags')
        for have, give in entries:
            self.__openTag('likelySubtag')
            self.__likelySubTag('from', have)
            self.__likelySubTag('to', give)
            self.__closeTag('likelySubtag')
        self.__closeTag('likelySubtags')

    def locales(self, locales, calendars):
        self.__openTag('localeList')
        self.__openTag('locale')
        Locale.C(calendars).toXml(self.inTag, calendars)
        self.__closeTag('locale')
        keys = locales.keys()
        keys.sort()
        for key in keys:
            self.__openTag('locale')
            locales[key].toXml(self.inTag, calendars)
            self.__closeTag('locale')
        self.__closeTag('localeList')

    def version(self, cldrVersion):
        self.inTag('version', cldrVersion)

    def inTag(self, tag, text):
        self.__write('<{0}>{1}</{0}>'.format(tag, text))

    def close(self):
        if self.__rawOutput != self.__complain:
            self.__write('</localeDatabase>')
        self.__rawOutput = self.__complain

    # Implementation details
    @staticmethod
    def __printit(text):
        print(text, end='')
    @staticmethod
    def __complain(text):
        raise Error('Attempted to write data after closing :-(')

    def __enumTable(self, tag, table):
        self.__openTag(tag)
        for key, value in table.iteritems():
            self.__openTag(tag[:-4])
            self.inTag('name', value[0])
            self.inTag('id', key)
            self.inTag('code', value[1])
            self.__closeTag(tag[:-4])
        self.__closeTag(tag)

    def __likelySubTag(self, tag, likely):
        self.__openTag(tag)
        self.inTag('language', likely[0])
        self.inTag('script', likely[1])
        self.inTag('country', likely[2])
        # self.inTag('variant', likely[3])
        self.__closeTag(tag)

    def __openTag(self, tag):
        self.__write('<{}>'.format(tag))
    def __closeTag(self, tag):
        self.__write('</{}>'.format(tag))

    def __write(self, line):
        self.__rawOutput(self.__wrap(line))

class Locale (object):
    """Holder for the assorted data representing one locale.

    Implemented as a namespace; its constructor and update() have the
    same signatures as those of a dict, acting on the instance's
    __dict__, so the results are accessed as attributes rather than
    mapping keys."""
    def __init__(self, data=None, **kw):
        self.update(data, **kw)

    def update(self, data=None, **kw):
        if data: self.__dict__.update(data)
        if kw: self.__dict__.update(kw)

    def __len__(self): # Used when testing as a boolean
        return len(self.__dict__)

    @staticmethod
    def propsMonthDay(scale, lengths=('long', 'short', 'narrow')):
        for L in lengths:
            yield camelCase((L, scale))
            yield camelCase(('standalone', L, scale))

    # Expected to be numbers, read with int():
    __asint = ("decimal", "group", "zero",
               "list", "percent", "minus", "plus", "exp",
               "currencyDigits", "currencyRounding")
    # Single character; use the code-point number for each:
    __asord = ("quotationStart", "quotationEnd",
               "alternateQuotationStart", "alternateQuotationEnd")
    # Convert day-name to Qt day-of-week number:
    __asdow = ("firstDayOfWeek", "weekendStart", "weekendEnd")
    # Convert from CLDR format-strings to QDateTimeParser ones:
    __asfmt = ("longDateFormat", "shortDateFormat", "longTimeFormat", "shortTimeFormat")
    # Just use the raw text:
    __astxt = ("language", "languageEndonym", "script", "country", "countryEndonym",
               "listPatternPartStart", "listPatternPartMiddle",
               "listPatternPartEnd", "listPatternPartTwo", "am", "pm",
               'byte_unit', 'byte_si_quantified', 'byte_iec_quantified',
               "currencyIsoCode", "currencySymbol", "currencyDisplayName",
               "currencyFormat", "currencyNegativeFormat")

    # Day-of-Week numbering used by Qt:
    __qDoW = {"mon": 1, "tue": 2, "wed": 3, "thu": 4, "fri": 5, "sat": 6, "sun": 7}

    @classmethod
    def fromXmlData(cls, lookup, calendars=('gregorian',)):
        """Constructor from the contents of XML elements.

        Single parameter, lookup, is called with the names of XML
        elements that should contain the relevant data, within a CLDR
        locale element (within a localeList element); these names are
        used for the attributes of the object constructed.  Attribute
        values are obtained by suitably digesting the returned element
        texts.\n"""
        data = {}
        for k in cls.__asint:
            data['listDelim' if k == 'list' else k] = int(lookup(k))

        for k in cls.__asord:
            value = lookup(k)
            assert len(value) == 1, \
                (k, value, 'value should be exactly one character')
            data[k] = ord(value)

        for k in cls.__asdow:
            data[k] = cls.__qDoW[lookup(k)]

        for k in cls.__asfmt:
            data[k] = convertFormat(lookup(k))

        for k in cls.__astxt + tuple(cls.propsMonthDay('days')):
            data[k] = lookup(k)

        for k in cls.propsMonthDay('months'):
            data[k] = dict((cal, lookup('_'.join((k, cal)))) for cal in calendars)

        return cls(data)

    def toXml(self, write, calendars=('gregorian',)):
        """Writes its data as QLocale XML.

        First argument, write, is a callable taking the name and
        content of an XML element; it is expected to be the inTag
        bound method of a QLocaleXmlWriter instance.

        Optional second argument is a list of calendar names, in the
        form used by CLDR; its default is ('gregorian',).
        """
        get = lambda k: getattr(self, k)
        for key in ('language', 'script', 'country'):
            write(key, get(key))
            write('{}code'.format(key), get('{}_code'.format(key)))

        for key in ('decimal', 'group', 'zero'):
            write(key, ordStr(get(key)))
        for key, std in (('list', ';'), ('percent', '%'),
                         ('minus', '-'), ('plus', '+'), ('exp', 'e')):
            write(key, fixOrdStr(get(key), std))

        for key in ('languageEndonym', 'countryEndonym',
                    'quotationStart', 'quotationEnd',
                    'alternateQuotationStart', 'alternateQuotationEnd',
                    'listPatternPartStart', 'listPatternPartMiddle',
                    'listPatternPartEnd', 'listPatternPartTwo',
                    'byte_unit', 'byte_si_quantified', 'byte_iec_quantified',
                    'am', 'pm', 'firstDayOfWeek',
                    'weekendStart', 'weekendEnd',
                    'longDateFormat', 'shortDateFormat',
                    'longTimeFormat', 'shortTimeFormat',
                    'currencyIsoCode', 'currencySymbol', 'currencyDisplayName',
                    'currencyFormat', 'currencyNegativeFormat'
                    ) + tuple(self.propsMonthDay('days')) + tuple(
                '_'.join((k, cal))
                for k in self.propsMonthDay('months')
                for cal in calendars):
            write(key, escape(get(key)).encode('utf-8'))

        for key in ('currencyDigits', 'currencyRounding'):
            write(key, get(key))

    # Tools used by __monthNames:
    def fullName(i, name): return name
    def firstThree(i, name): return name[:3]
    def initial(i, name): return name[:1]
    def number(i, name): return str(i + 1)
    def islamicShort(i, name):
        if not name: return name
        if name == 'Shawwal': return 'Shaw.'
        words = name.split()
        if words[0].startswith('Dhu'):
            words[0] = words[0][:7] + '.'
        elif len(words[0]) > 3:
            words[0] = words[0][:3] + '.'
        return ' '.join(words)
    @staticmethod
    def __monthNames(calendars,
                     known={ # Map calendar to (names, extractors...):
            # TODO: do we even need these ?  CLDR's root.xml seems to
            # have them, complete with yeartype="leap" handling for
            # Hebrew's extra.
            'gregorian': (('January', 'February', 'March', 'April', 'May', 'June', 'July',
                           'August', 'September', 'October', 'November', 'December'),
                          # Extractor pairs, (plain, standalone)
                          (fullName, fullName), # long
                          (firstThree, firstThree), # short
                          (number, initial)), # narrow
            'persian': (('Farvardin', 'Ordibehesht', 'Khordad', 'Tir', 'Mordad',
                         'Shahrivar', 'Mehr', 'Aban', 'Azar', 'Dey', 'Bahman', 'Esfand'),
                        (fullName, fullName),
                        (firstThree, firstThree),
                        (number, initial)),
            'islamic': ((u'Muharram', u'Safar', u'Rabiʻ I', u'Rabiʻ II', u'Jumada I',
                         u'Jumada II', u'Rajab', u'Shaʻban', u'Ramadan', u'Shawwal',
                         u'Dhuʻl-Qiʻdah', u'Dhuʻl-Hijjah'),
                        (fullName, fullName),
                        (islamicShort, islamicShort),
                        (number, number)),
            'hebrew': (('Tishri', 'Heshvan', 'Kislev', 'Tevet', 'Shevat', 'Adar I',
                        'Adar', 'Nisan', 'Iyar', 'Sivan', 'Tamuz', 'Av'),
                       (fullName, fullName),
                       (fullName, fullName),
                       (number, number)),
            },
                     sizes=('long', 'short', 'narrow')):
        for cal in calendars:
            try:
                data = known[cal]
            except KeyError as e: # Need to add an entry to known, above.
                e.args += ('Unsupported calendar:', cal)
                raise
            names, get = data[0] + ('',), data[1:]
            for n, size in enumerate(sizes):
                yield ('_'.join((camelCase((size, 'months')), cal)),
                       ';'.join(get[n][0](i, x) for i, x in enumerate(names)))
                yield ('_'.join((camelCase(('standalone', size, 'months')), cal)),
                       ';'.join(get[n][1](i, x) for i, x in enumerate(names)))
    del fullName, firstThree, initial, number, islamicShort

    @classmethod
    def C(cls, calendars=('gregorian',),
          # Empty entry at end to ensure final separator when join()ed:
          days = ('Sunday', 'Monday', 'Tuesday', 'Wednesday',
                  'Thursday', 'Friday', 'Saturday', ''),
          quantifiers=('k', 'M', 'G', 'T', 'P', 'E')):
        """Returns an object representing the C locale."""
        return cls(cls.__monthNames(calendars),
                   language='C', language_code='0', languageEndonym='',
                   script='AnyScript', script_code='0',
                   country='AnyCountry', country_code='0', countryEndonym='',
                   decimal='.', group=',', list=';', percent='%',
                   zero='0', minus='-', plus='+', exp='e',
                   quotationStart='"', quotationEnd='"',
                   alternateQuotationStart='\'', alternateQuotationEnd='\'',
                   listPatternPartStart='%1, %2',
                   listPatternPartMiddle='%1, %2',
                   listPatternPartEnd='%1, %2',
                   listPatternPartTwo='%1, %2',
                   byte_unit='bytes',
                   byte_si_quantified=';'.join(q + 'B' for q in quantifiers),
                   byte_iec_quantified=';'.join(q.upper() + 'iB' for q in quantifiers),
                   am='AM', pm='PM', firstDayOfWeek='mon',
                   weekendStart='sat', weekendEnd='sun',
                   longDateFormat='EEEE, d MMMM yyyy', shortDateFormat='d MMM yyyy',
                   longTimeFormat='HH:mm:ss z', shortTimeFormat='HH:mm:ss',
                   longDays=';'.join(days),
                   shortDays=';'.join(d[:3] for d in days),
                   narrowDays='7;1;2;3;4;5;6;',
                   standaloneLongDays=';'.join(days),
                   standaloneShortDays=';'.join(d[:3] for d in days),
                   standaloneNarrowDays=';'.join(d[:1] for d in days),
                   currencyIsoCode='', currencySymbol='',
                   currencyDisplayName=';' * 7,
                   currencyDigits=2, currencyRounding=1,
                   currencyFormat='%1%2', currencyNegativeFormat='')
