# Copyright (C) 2021 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
"""Shared serialization-scanning code for QLocaleXML format.

Provides classes:
  Locale -- common data-type representing one locale as a namespace
  QLocaleXmlWriter -- helper to write a QLocaleXML file
  QLocaleXmlReader -- helper to read a QLocaleXML file back in

Support:
  Spacer -- provides control over indentation of the output.

RelaxNG schema for the used file format can be found in qlocalexml.rnc.
QLocaleXML files can be validated using:

    jing -c qlocalexml.rnc <file.xml>

You can download jing from https://relaxng.org/jclark/jing.html if your
package manager lacks the jing package.
"""

from typing import Iterator
from xml.sax.saxutils import escape

from localetools import Error

# Tools used by Locale:
def camel(seq):
    yield next(seq)
    for word in seq:
        yield word.capitalize()

def camelCase(words):
    return ''.join(camel(iter(words)))

def addEscapes(s):
    return ''.join(c if n < 128 else f'\\x{n:02x}'
                   for n, c in ((ord(c), c) for c in s))

def startCount(c, text): # strspn
    """First index in text where it doesn't have a character in c"""
    assert text and text[0] in c
    try:
        return next((j for j, d in enumerate(text) if d not in c))
    except StopIteration:
        return len(text)

class QLocaleXmlReader (object):
    def __init__(self, filename):
        self.root = self.__parse(filename)

        from enumdata import language_map, script_map, territory_map
        # Lists of (id, enum name, code, en.xml name) tuples:
        languages = tuple(self.__loadMap('language', language_map))
        scripts = tuple(self.__loadMap('script', script_map))
        territories = tuple(self.__loadMap('territory', territory_map))
        self.__likely = tuple(self.__likelySubtagsMap())

        # Mappings {ID: (enum name, code, en.xml name)}
        self.languages = {v[0]: v[1:] for v in languages}
        self.scripts = {v[0]: v[1:] for v in scripts}
        self.territories = {v[0]: v[1:] for v in territories}

        # Private mappings {enum name: (ID, code)}
        self.__langByName = {v[1]: (v[0], v[2]) for v in languages}
        self.__textByName = {v[1]: (v[0], v[2]) for v in scripts}
        self.__landByName = {v[1]: (v[0], v[2]) for v in territories}
        # Other properties:
        self.__dupes = set(v[1] for v in languages) & set(v[1] for v in territories)
        self.cldrVersion = self.__firstChildText(self.root, "version")

    def loadLocaleMap(self, calendars, grumble = lambda text: None):
        kid = self.__firstChildText
        likely = dict(self.__likely)
        for elt in self.__eachEltInGroup(self.root, 'localeList', 'locale'):
            locale = Locale.fromXmlData(lambda k: kid(elt, k), calendars)
            language = self.__langByName[locale.language][0]
            script = self.__textByName[locale.script][0]
            territory = self.__landByName[locale.territory][0]

            if language != 1: # C
                if territory == 0:
                    grumble(f'loadLocaleMap: No territory id for "{locale.language}"\n')

                if script == 0:
                    # Find default script for the given language and territory - see:
                    # http://www.unicode.org/reports/tr35/#Likely_Subtags
                    try:
                        try:
                            to = likely[(locale.language, 'AnyScript', locale.territory)]
                        except KeyError:
                            to = likely[(locale.language, 'AnyScript', 'AnyTerritory')]
                    except KeyError:
                        pass
                    else:
                        locale.script = to[1]
                        script = self.__textByName[locale.script][0]

            yield (language, script, territory), locale

    def aliasToIana(self):
        kid = self.__firstChildText
        for elt in self.__eachEltInGroup(self.root, 'zoneAliases', 'zoneAlias'):
            yield kid(elt, 'alias'), kid(elt, 'iana')

    def msToIana(self):
        kid = self.__firstChildText
        for elt in self.__eachEltInGroup(self.root, 'windowsZone', 'msZoneIana'):
            yield kid(elt, 'msid'), kid(elt, 'iana')

    def msLandIanas(self):
        kid = self.__firstChildText
        for elt in self.__eachEltInGroup(self.root, 'windowsZone', 'msLandZones'):
            yield kid(elt, 'msid'), kid(elt, 'territorycode'), kid(elt, 'ianaids')

    def languageIndices(self, locales):
        index = 0
        for key, value in self.languages.items():
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

        for pair in self.__likely:
            have = self.__fromNames(pair[0])
            give = self.__fromNames(pair[1])
            yield ('_'.join(tag(have)), ids(have),
                   '_'.join(tag(give)), ids(give))

    def defaultMap(self) -> Iterator[tuple[tuple[int, int], int]]:
        """Map language and script to their default territory by ID.

        Yields ((language, script), territory) wherever the likely
        sub-tags mapping says language's default locale uses the given
        script and territory."""
        for have, give in self.__likely:
            if have[1:] == ('AnyScript', 'AnyTerritory') and give[2] != 'AnyTerritory':
                assert have[0] == give[0], (have, give)
                yield ((self.__langByName[give[0]][0],
                        self.__textByName[give[1]][0]),
                       self.__landByName[give[2]][0])

    def enumify(self, name, suffix):
        """Stick together the parts of an enumdata.py name.

        Names given in enumdata.py include spaces and hyphens that we
        can't include in an identifier, such as the name of a member
        of an enum type. Removing those would lose the word
        boundaries, so make sure each word starts with a capital (but
        don't simply capitalize() as some names contain words,
        e.g. McDonald, that have later capitals in them).

        We also need to resolve duplication between languages and
        territories (by adding a suffix to each) and add Script to the
        ends of script-names that don't already end in it."""
        name = name.replace('-', ' ')
        # Don't .capitalize() as McDonald is already camel-case (see enumdata.py):
        name = ''.join(word[0].upper() + word[1:] for word in name.split())
        if suffix != 'Script':
            assert not(name in self.__dupes and name.endswith(suffix))
            return name + suffix if name in self.__dupes else name

        if not name.endswith(suffix):
            name += suffix
        if name in self.__dupes:
            raise Error(f'The script name "{name}" is messy')
        return name

    # Implementation details:
    def __loadMap(self, category, enum):
        kid = self.__firstChildText
        for element in self.__eachEltInGroup(self.root, f'{category}List', category):
            key = int(kid(element, 'id'))
            yield key, enum[key][0], kid(element, 'code'), kid(element, 'name')

    def __likelySubtagsMap(self):
        def triplet(element, keys=('language', 'script', 'territory'), kid = self.__firstChildText):
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
                # Note: do not strip(), as some group separators are
                # non-breaking spaces, that strip() will discard.
                yield child.nodeValue
            child = child.nextSibling

    @classmethod
    def __firstChildElt(cls, parent, name):
        child = parent.firstChild
        while child:
            if cls.__isNodeNamed(child, name):
                return child
            child = child.nextSibling

        raise Error(f'No {name} child found')

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
        to use tabs). If indent is None, no indentation is added, nor
        are line-breaks; otherwise, self(text), for non-empty text,
        shall end with a newline and begin with indentation.

        Second argument, initial, is the initial indentation; it is
        ignored if indent is None. Indentation increases after each
        call to self(text) in which text starts with a tag and doesn't
        include its end-tag; indentation decreases if text starts with
        an end-tag. The text is not parsed any more carefully than
        just described."""
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
            if f'</{tag}>' not in line:
                self.current += self.__each
        return indent + line + '\n'

    def __call__(self, line):
        return self.__call(line)

class QLocaleXmlWriter (object):
    """Save the full set of locale data to a QLocaleXML file.

    The output saved by this should conform to qlocalexml.rnc's
    schema."""
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
    def enumData(self, code2name):
        """Output name/id/code tables for language, script and territory.

        Parameter, code2name, is a function taking 'language',
        'script' or 'territory' and returning a lookup function that
        maps codes, of the relevant type, to their English names. This
        lookup function is passed a code and the name, both taken from
        enumdata.py, that QLocale uses, so the .get() of a dict will
        work. The English name from this lookup will be used by
        QLocale::*ToString() for the enum member whose name is based
        on the enumdata.py name passed as fallback to the lookup."""
        from enumdata import language_map, script_map, territory_map
        self.__enumTable('language', language_map, code2name)
        self.__enumTable('script', script_map, code2name)
        self.__enumTable('territory', territory_map, code2name)
        # Prepare to detect any unused codes (see __writeLocale(), close()):
        self.__languages = set(p[1] for p in language_map.values()
                               if not p[1].isspace())
        self.__scripts = set(p[1] for p in script_map.values()
                             if p[1] != 'Zzzz')
        self.__territories = set(p[1] for p in territory_map.values()
                                 if p[1] != 'ZZ')

    def likelySubTags(self, entries):
        self.__openTag('likelySubtags')
        for have, give in entries:
            self.__openTag('likelySubtag')
            self.__likelySubTag('from', have)
            self.__likelySubTag('to', give)
            self.__closeTag('likelySubtag')
        self.__closeTag('likelySubtags')

    def zoneData(self, alias, defaults, windowsIds):
        self.__openTag('zoneAliases')
        # iana is a single IANA ID
        # name has the same form, but has been made redundant
        for name, iana in sorted(alias.items(), key = lambda s: (s[0].lower(), s[1])):
            self.__openTag('zoneAlias')
            self.inTag('alias', name)
            self.inTag('iana', iana)
            self.__closeTag('zoneAlias')
        self.__closeTag('zoneAliases')

        self.__openTag('windowsZone')
        for (msid, code), ids in windowsIds.items():
            # ianaids is a space-joined sequence of IANA IDs
            self.__openTag('msLandZones')
            self.inTag('msid', msid)
            self.inTag('territorycode', code)
            self.inTag('ianaids', ids)
            self.__closeTag('msLandZones')

        for winid, iana in defaults.items():
            self.__openTag('msZoneIana')
            self.inTag('msid', winid)
            self.inTag('iana', iana)
            self.__closeTag('msZoneIana')
        self.__closeTag('windowsZone')

    def locales(self, locales, calendars, en_US):
        """Write the data for each locale.

        First argument, locales, is the mapping whose values are the
        Locale objects, with each key being the matching tuple of
        numeric IDs for language, script, territory and variant.
        Second argument is a tuple of calendar names. Third is the
        tuple of numeric IDs that corresponds to en_US (needed to
        provide fallbacks for the C locale)."""

        self.__openTag('localeList')
        self.__openTag('locale')
        self.__writeLocale(Locale.C(locales[en_US]), calendars)
        self.__closeTag('locale')
        for key in sorted(locales.keys()):
            self.__openTag('locale')
            self.__writeLocale(locales[key], calendars)
            self.__closeTag('locale')
        self.__closeTag('localeList')

    def version(self, cldrVersion):
        self.inTag('version', cldrVersion)

    def inTag(self, tag, text):
        self.__write(f'<{tag}>{text}</{tag}>')

    def close(self, grumble):
        """Finish writing and grumble about any issues discovered."""
        if self.__rawOutput != self.__complain:
            self.__write('</localeDatabase>')
        self.__rawOutput = self.__complain

        if self.__languages or self.__scripts or self.__territories:
            grumble('Some enum members are unused, corresponding to these tags:\n')
            import textwrap
            def kvetch(kind, seq, g = grumble, w = textwrap.wrap):
                g('\n\t'.join(w(f' {kind}: {", ".join(sorted(seq))}', width=80)) + '\n')
            if self.__languages:
                kvetch('Languages', self.__languages)
            if self.__scripts:
                kvetch('Scripts', self.__scripts)
            if self.__territories:
                kvetch('Territories', self.__territories)
            grumble('It may make sense to deprecate them.\n')

    # Implementation details
    @staticmethod
    def __printit(text):
        print(text, end='')
    @staticmethod
    def __complain(text):
        raise Error('Attempted to write data after closing :-(')

    @staticmethod
    def __xmlSafe(text):
        return text.replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;')

    def __enumTable(self, tag, table, code2name):
        self.__openTag(f'{tag}List')
        enname, safe = code2name(tag), self.__xmlSafe
        for key, (name, code) in table.items():
            self.__openTag(tag)
            self.inTag('name', safe(enname(code, name)))
            self.inTag('id', key)
            self.inTag('code', code)
            self.__closeTag(tag)
        self.__closeTag(f'{tag}List')

    def __likelySubTag(self, tag, likely):
        self.__openTag(tag)
        self.inTag('language', likely[0])
        self.inTag('script', likely[1])
        self.inTag('territory', likely[2])
        # self.inTag('variant', likely[3])
        self.__closeTag(tag)

    def __writeLocale(self, locale, calendars):
        locale.toXml(self.inTag, calendars)
        self.__languages.discard(locale.language_code)
        self.__scripts.discard(locale.script_code)
        self.__territories.discard(locale.territory_code)

    def __openTag(self, tag, **attrs):
        if attrs:
            text = ', '.join(f'{k}="{v}"' for k, v in attrs.items())
            tag = f'{tag} {text}'
        self.__write(f'<{tag}>')
    def __closeTag(self, tag):
        self.__write(f'</{tag}>')

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
    __asint = ("currencyDigits", "currencyRounding")
    # Convert day-name to Qt day-of-week number:
    __asdow = ("firstDayOfWeek", "weekendStart", "weekendEnd")
    # Just use the raw text:
    __astxt = ("language", "languageEndonym", "script", "territory", "territoryEndonym",
               "decimal", "group", "zero",
               "list", "percent", "minus", "plus", "exp",
               "quotationStart", "quotationEnd",
               "alternateQuotationStart", "alternateQuotationEnd",
               "listPatternPartStart", "listPatternPartMiddle",
               "listPatternPartEnd", "listPatternPartTwo", "am", "pm",
               "longDateFormat", "shortDateFormat",
               "longTimeFormat", "shortTimeFormat",
               'byte_unit', 'byte_si_quantified', 'byte_iec_quantified',
               "currencyIsoCode", "currencySymbol", "currencyDisplayName",
               "currencyFormat", "currencyNegativeFormat",
               )

    # Day-of-Week numbering used by Qt:
    __qDoW = {"mon": 1, "tue": 2, "wed": 3, "thu": 4, "fri": 5, "sat": 6, "sun": 7}

    @classmethod
    def fromXmlData(cls, lookup, calendars=('gregorian',)):
        """Constructor from the contents of XML elements.

        First parameter, lookup, is called with the names of XML elements that
        should contain the relevant data, within a QLocaleXML locale element
        (within a localeList element); these names mostly match the attributes
        of the object constructed. Its return must be the full text of the
        first child DOM node element with the given name. Attribute values are
        obtained by suitably digesting the returned element texts.

        Optional second parameter, calendars, is a sequence of calendars for
        which data is to be retrieved."""
        data = {}
        for k in cls.__asint:
            data[k] = int(lookup(k))

        for k in cls.__asdow:
            data[k] = cls.__qDoW[lookup(k)]

        for k in cls.__astxt + tuple(cls.propsMonthDay('days')):
            data['listDelim' if k == 'list' else k] = lookup(k)

        for k in cls.propsMonthDay('months'):
            data[k] = {cal: lookup('_'.join((k, cal))) for cal in calendars}

        grouping = lookup('groupSizes').split(';')
        data.update(groupLeast = int(grouping[0]),
                    groupHigher = int(grouping[1]),
                    groupTop = int(grouping[2]))

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
        for key in ('language', 'script', 'territory'):
            write(key, get(key))
            write(f'{key}code', get(f'{key}_code'))

        for key in ('decimal', 'group', 'zero', 'list',
                    'percent', 'minus', 'plus', 'exp'):
            write(key, get(key))

        for key in ('languageEndonym', 'territoryEndonym',
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
                    'currencyFormat', 'currencyNegativeFormat',
                    ) + tuple(self.propsMonthDay('days')) + tuple(
                '_'.join((k, cal))
                for k in self.propsMonthDay('months')
                for cal in calendars):
            write(key, escape(get(key)))

        write('groupSizes', ';'.join(str(x) for x in get('groupSizes')))
        for key in ('currencyDigits', 'currencyRounding'):
            write(key, get(key))

    @classmethod
    def C(cls, en_US):
        """Returns an object representing the C locale.

        Required argument, en_US, is the corresponding object for the
        en_US locale (or the en_US_POSIX one if we ever support
        variants). The C locale inherits from this, overriding what it
        may need to."""
        base = en_US.__dict__.copy()
        # Soroush's original contribution shortened Jalali month names
        # - contrary to CLDR, which doesn't abbreviate these in
        # root.xml or en.xml, although some locales do, e.g. fr_CA.
        # For compatibility with that,
        for k in ('shortMonths_persian', 'standaloneShortMonths_persian'):
            base[k] = ';'.join(x[:3] for x in base[k].split(';'))

        return cls(base,
                   language='C', language_code='',
                   language_id=0, languageEndonym='',
                   script='AnyScript', script_code='', script_id=0,
                   territory='AnyTerritory', territory_code='',
                   territory_id=0, territoryEndonym='',
                   variant='', variant_code='', variant_id=0,
                   # CLDR has non-ASCII versions of these:
                   quotationStart='"', quotationEnd='"',
                   alternateQuotationStart="'", alternateQuotationEnd="'",
                   # CLDR gives 'dddd, MMMM d, yyyy', 'M/d/yy', 'h:mm:ss Ap tttt',
                   # 'h:mm Ap' with non-breaking space before Ap.
                   longDateFormat='dddd, d MMMM yyyy', shortDateFormat='d MMM yyyy',
                   longTimeFormat='HH:mm:ss t', shortTimeFormat='HH:mm:ss',
                   # CLDR has US-$ and US-style formats:
                   currencyIsoCode='', currencySymbol='', currencyDisplayName='',
                   currencyDigits=2, currencyRounding=1,
                   currencyFormat='%1%2', currencyNegativeFormat='',
                   # We may want to fall back to CLDR for some of these:
                   firstDayOfWeek='mon', # CLDR has 'sun'
                   exp='e', # CLDR has 'E'
                   listPatternPartEnd='%1, %2', # CLDR has '%1, and %2'
                   listPatternPartTwo='%1, %2', # CLDR has '%1 and %2'
                   narrowDays='7;1;2;3;4;5;6', # CLDR has letters
                   narrowMonths_gregorian='1;2;3;4;5;6;7;8;9;10;11;12', # CLDR has letters
                   standaloneNarrowMonths_persian='F;O;K;T;M;S;M;A;A;D;B;E', # CLDR has digits
                   # Keep these explicit, despite matching CLDR:
                   decimal='.', group=',', percent='%',
                   zero='0', minus='-', plus='+',
                   am='AM', pm='PM', weekendStart='sat', weekendEnd='sun')
