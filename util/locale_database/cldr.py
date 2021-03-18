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
"""Digesting the CLDR's data.

Provides two classes:
  CldrReader -- driver for reading CLDR data
  CldrAccess -- used by the reader to access the tree of data files

The former should normally be all you need to access.
See individual classes for further detail.
"""

from xml.dom import minidom
from weakref import WeakValueDictionary as CacheDict
import os

from ldml import Error, Node, XmlScanner, Supplement, LocaleScanner
from qlocalexml import Locale

class CldrReader (object):
    def __init__(self, root, grumble = lambda msg: None, whitter = lambda msg: None):
        """Set up a reader object for reading CLDR data.

        Single parameter, root, is the file-system path to the root of
        the unpacked CLDR archive; its common/ sub-directory should
        contain dtd/, main/ and supplemental/ sub-directories.

        Optional second argument, grumble, is a callable that logs
        warnings and complaints, e.g. sys.stderr.write would be a
        suitable callable.  The default is a no-op that ignores its
        single argument.  Optional third argument is similar, used for
        less interesting output; pass sys.stderr.write for it for
        verbose output."""
        self.root = CldrAccess(root)
        self.whitter, self.grumble = whitter, grumble

    def likelySubTags(self):
        """Generator for likely subtag information.

        Yields pairs (have, give) of 4-tuples; if what you have
        matches the left member, giving the right member is probably
        sensible. Each 4-tuple's entries are the full names of a
        language, a script, a country (strictly territory) and a
        variant (currently ignored)."""
        skips = []
        for got, use in self.root.likelySubTags():
            try:
                have = self.__parseTags(got)
                give = self.__parseTags(use)
            except Error as e:
                if ((use.startswith(got) or got.startswith('und_'))
                    and e.message.startswith('Unknown ') and ' code ' in e.message):
                    skips.append(use)
                else:
                    self.grumble('Skipping likelySubtag "{}" -> "{}" ({})\n'.format(got, use, e.message))
                continue
            if all(code.startswith('Any') and code[3].isupper() for code in have[:-1]):
                continue

            give = (give[0],
                    # Substitute according to http://www.unicode.org/reports/tr35/#Likely_Subtags
                    have[1] if give[1] == 'AnyScript' else give[1],
                    have[2] if give[2] == 'AnyCountry' else give[2],
                    give[3]) # AnyVariant similarly ?

            yield have, give

        if skips:
            # TODO: look at LDML's reserved locale tag names; they
            # show up a lot in this, and may be grounds for filtering
            # more out.
            pass # self.__wrapped(self.whitter, 'Skipping likelySubtags (for unknown codes): ', skips)

    def readLocales(self, calendars = ('gregorian',)):
        locales = tuple(self.__allLocales(calendars))
        return dict(((k.language_id, k.script_id, k.country_id, k.variant_code),
                     k) for k in locales)

    def __allLocales(self, calendars):
        def skip(locale, reason):
            return 'Skipping defaultContent locale "{}" ({})\n'.format(locale, reason)

        for locale in self.root.defaultContentLocales:
            try:
                language, script, country, variant = self.__splitLocale(locale)
            except ValueError:
                self.whitter(skip(locale, 'only language tag'))
                continue

            if not (script or country):
                self.grumble(skip(locale, 'second tag is neither script nor territory'))
                continue

            if not (language and country):
                continue

            try:
                yield self.__getLocaleData(self.root.locale(locale), calendars,
                                           language, script, country, variant)
            except Error as e:
                self.grumble(skip(locale, e.message))

        for locale in self.root.fileLocales:
            try:
                chain = self.root.locale(locale)
                language, script, country, variant = chain.tagCodes()
                assert language
                # TODO: this skip should probably be based on likely
                # sub-tags, instead of empty country: if locale has a
                # likely-subtag expansion, that's what QLocale uses,
                # and we'll be saving its data for the expanded locale
                # anyway, so don't need to record it for itself.
                # See also QLocaleXmlReader.loadLocaleMap's grumble.
                if not country:
                    continue
                yield self.__getLocaleData(chain, calendars, language, script, country, variant)
            except Error as e:
                self.grumble('Skipping file locale "{}" ({})\n'.format(locale, e.message))

    import textwrap
    @staticmethod
    def __wrapped(writer, prefix, tokens, wrap = textwrap.wrap):
        writer('\n'.join(wrap(prefix + ', '.join(tokens),
                              subsequent_indent=' ', width=80)) + '\n')
    del textwrap

    def __parseTags(self, locale):
        tags = self.__splitLocale(locale)
        language = tags.next()
        script = country = variant = ''
        try:
            script, country, variant = tags
        except ValueError:
            pass
        return tuple(p[1] for p in self.root.codesToIdName(language, script, country, variant))

    def __splitLocale(self, name):
        """Generate (language, script, territory, variant) from a locale name

        Ignores any trailing fields (with a warning), leaves script (a
        capitalised four-letter token), territory (either a number or
        an all-uppercase token) or variant (upper case and digits)
        empty if unspecified.  Only generates one entry if name is a
        single tag (i.e. contains no underscores).  Always yields 1 or
        4 values, never 2 or 3."""
        tags = iter(name.split('_'))
        yield tags.next() # Language
        tag = tags.next() # may raise StopIteration

        # Script is always four letters, always capitalised:
        if len(tag) == 4 and tag[0].isupper() and tag[1:].islower():
            yield tag
            try:
                tag = tags.next()
            except StopIteration:
                tag = ''
        else:
            yield ''

        # Territory is upper-case or numeric:
        if tag and tag.isupper() or tag.isdigit():
            yield tag
            try:
                tag = tags.next()
            except StopIteration:
                tag = ''
        else:
            yield ''

        # Variant can be any mixture of upper-case and digits.
        if tag and all(c.isupper() or c.isdigit() for c in tag):
            yield tag
            tag = ''
        else:
            yield ''

        # If nothing is left, StopIteration will avoid the warning:
        if not tag:
            tag = tags.next()
        self.grumble('Ignoring unparsed cruft {} in {}\n'.format('_'.join(tag + tuple(tags)), name))

    def __getLocaleData(self, scan, calendars, language, script, country, variant):
        ids, names = zip(*self.root.codesToIdName(language, script, country, variant))
        assert ids[0] > 0 and ids[2] > 0, (language, script, country, variant)
        locale = Locale(
            language = names[0], language_code = language, language_id = ids[0],
            script = names[1], script_code = script, script_id = ids[1],
            country = names[2], country_code = country, country_id = ids[2],
            variant_code = variant)

        firstDay, weStart, weEnd = self.root.weekData(country)
        assert all(day in ('mon', 'tue', 'wed', 'thu', 'fri', 'sat', 'sun')
                   for day in (firstDay, weStart, weEnd))

        locale.update(firstDayOfWeek = firstDay,
                      weekendStart = weStart,
                      weekendEnd = weEnd)

        iso, digits, rounding = self.root.currencyData(country)
        locale.update(currencyIsoCode = iso,
                      currencyDigits = int(digits),
                      currencyRounding = int(rounding))

        locale.update(scan.currencyData(iso))
        locale.update(scan.numericData(self.root.numberSystem, self.whitter))
        locale.update(scan.textPatternData())
        locale.update(scan.endonyms(language, script, country, variant))
        locale.update(scan.unitData()) # byte, kB, MB, GB, ..., KiB, MiB, GiB, ...
        locale.update(scan.calendarNames(calendars)) # Names of days and months

        return locale

# Note: various caches assume this class is a singleton, so the
# "default" value for a parameter no caller should pass can serve as
# the cache. If a process were to instantiate this class with distinct
# roots, each cache would be filled by the first to need it !
class CldrAccess (object):
    def __init__(self, root):
        """Set up a master object for accessing CLDR data.

        Single parameter, root, is the file-system path to the root of
        the unpacked CLDR archive; its common/ sub-directory should
        contain dtd/, main/ and supplemental/ sub-directories."""
        self.root = root

    def xml(self, *path):
        """Load a single XML file and return its root element as an XmlScanner.

        The path is interpreted relative to self.root"""
        return XmlScanner(Node(self.__xml(path)))

    def supplement(self, name):
        """Loads supplemental data as a Supplement object.

        The name should be that of a file in common/supplemental/, without path.
        """
        return Supplement(Node(self.__xml(('common', 'supplemental', name))))

    def locale(self, name):
        """Loads all data for a locale as a LocaleScanner object.

        The name should be a locale name; adding suffix '.xml' to it
        should usually yield a file in common/main/.  The returned
        LocaleScanner object packages this file along with all those
        from which it inherits; its methods know how to handle that
        inheritance, where relevant."""
        return LocaleScanner(name, self.__localeRoots(name), self.__rootLocale)

    @property
    def fileLocales(self, joinPath = os.path.join, listDirectory = os.listdir,
                    splitExtension = os.path.splitext):
        """Generator for locale IDs seen in file-names.

        All *.xml other than root.xml in common/main/ are assumed to
        identify locales."""
        for name in listDirectory(joinPath(self.root, 'common', 'main')):
            stem, ext = splitExtension(name)
            if ext == '.xml' and stem != 'root':
                yield stem

    @property
    def defaultContentLocales(self):
        """Generator for the default content locales."""
        for name, attrs in self.supplement('supplementalMetadata.xml').find('metadata/defaultContent'):
            try:
                locales = attrs['locales']
            except KeyError:
                pass
            else:
                for locale in locales.split():
                    yield locale

    def likelySubTags(self):
        for ignore, attrs in self.supplement('likelySubtags.xml').find('likelySubtags'):
            yield attrs['from'], attrs['to']

    def numberSystem(self, system):
        """Get a description of a numbering system.

        Returns a mapping, with keys u'digits', u'type' and u'id'; the
        value for this last is system. Raises KeyError for unknown
        number system, ldml.Error on failure to load data."""
        try:
            return self.__numberSystems[system]
        except KeyError:
            raise Error('Unsupported number system: {}'.format(system))

    def weekData(self, country):
        """Data on the weekly cycle.

        Returns a triple (W, S, E) of en's short names for week-days;
        W is the first day of the week, S the start of the week-end
        and E the end of the week-end.  Where data for a country is
        unavailable, the data for CLDR's territory 001 (The World) is
        used."""
        try:
            return self.__weekData[country]
        except KeyError:
            return self.__weekData['001']

    def currencyData(self, country):
        """Returns currency data for the given country code.

        Return value is a tuple (ISO4217 code, digit count, rounding
        mode).  If CLDR provides no data for this country, ('', 2, 1)
        is the default result.
        """
        try:
            return self.__currencyData[country]
        except KeyError:
            return '', 2, 1

    def codesToIdName(self, language, script, country, variant = ''):
        """Maps each code to the appropriate ID and name.

        Returns a 4-tuple of (ID, name) pairs corresponding to the
        language, script, country and variant given.  Raises a
        suitable error if any of them is unknown, indicating all that
        are unknown plus suitable names for any that could sensibly be
        added to enumdata.py to make them known.

        Until we implement variant support (QTBUG-81051), the fourth
        member of the returned tuple is always 0 paired with a string
        that should not be used."""
        enum = self.__enumMap
        try:
            return (enum('language')[language],
                    enum('script')[script],
                    enum('country')[country],
                    enum('variant')[variant])
        except KeyError:
            pass

        parts, values = [], [language, script, country, variant]
        for index, key in enumerate(('language', 'script', 'country', 'variant')):
            naming, enums = self.__codeMap(key), enum(key)
            value = values[index]
            if value not in enums:
                text = '{} code {}'.format(key, value)
                name = naming.get(value)
                if name and value != 'POSIX':
                    text += u' (could add {})'.format(name)
                parts.append(text)
        if len(parts) > 1:
            parts[-1] = 'and ' + parts[-1]
        assert parts
        raise Error('Unknown ' + ', '.join(parts),
                    language, script, country, variant)

    def readWindowsTimeZones(self, lookup): # For use by cldr2qtimezone.py
        """Digest CLDR's MS-Win time-zone name mapping.

        MS-Win have their own eccentric names for time-zones.  CLDR
        helpfully provides a translation to more orthodox names.

        Singe argument, lookup, is a mapping from known MS-Win names
        for locales to a unique integer index (starting at 1).

        The XML structure we read has the form:

 <supplementalData>
     <windowsZones>
         <mapTimezones otherVersion="..." typeVersion="...">
             <!-- (UTC-08:00) Pacific Time (US & Canada) -->
             <mapZone other="Pacific Standard Time" territory="001" type="America/Los_Angeles"/>
             <mapZone other="Pacific Standard Time" territory="CA" type="America/Vancouver America/Dawson America/Whitehorse"/>
             <mapZone other="Pacific Standard Time" territory="US" type="America/Los_Angeles America/Metlakatla"/>
             <mapZone other="Pacific Standard Time" territory="ZZ" type="PST8PDT"/>
         </mapTimezones>
     </windowsZones>
 </supplementalData>
"""
        zones = self.supplement('windowsZones.xml')
        enum = self.__enumMap('country')
        badZones, unLands, defaults, windows = set(), set(), {}, {}

        for name, attrs in zones.find('windowsZones/mapTimezones'):
            if name != 'mapZone':
                continue

            wid, code = attrs['other'], attrs['territory']
            data = dict(windowsId = wid,
                        countryCode = code,
                        ianaList = attrs['type'])

            try:
                key = lookup[wid]
            except KeyError:
                badZones.add(wid)
                key = 0
            data['windowsKey'] = key

            if code == u'001':
                defaults[key] = data['ianaList']
            else:
                try:
                    cid, name = enum[code]
                except KeyError:
                    unLands.append(code)
                    continue
                data.update(countryId = cid, country = name)
                windows[key, cid] = data

        if unLands:
            raise Error('Unknown country codes, please add to enumdata.py: '
                        + ', '.join(sorted(unLands)))

        if badZones:
            raise Error('Unknown Windows IDs, please add to cldr2qtimezone.py: '
                        + ', '.join(sorted(badZones)))

        return self.cldrVersion, defaults, windows

    @property
    def cldrVersion(self):
        # Evaluate so as to ensure __cldrVersion is set:
        self.__unDistinguishedAttributes
        return self.__cldrVersion

    # Implementation details
    def __xml(self, path, cache = CacheDict(), read = minidom.parse, joinPath = os.path.join):
        try:
            doc = cache[path]
        except KeyError:
            cache[path] = doc = read(joinPath(self.root, *path)).documentElement
        return doc

    def __open(self, path, joinPath=os.path.join):
        return open(joinPath(self.root, *path))

    @property
    def __rootLocale(self, cache = []):
        if not cache:
            cache.append(self.xml('common', 'main', 'root.xml'))
        return cache[0]

    @property
    def __supplementalData(self, cache = []):
        if not cache:
            cache.append(self.supplement('supplementalData.xml'))
        return cache[0]

    @property
    def __numberSystems(self, cache = {}, joinPath=os.path.join):
        if not cache:
            for ignore, attrs in self.supplement('numberingSystems.xml').find('numberingSystems'):
                if ord(attrs.get('digits', u'\x10000')[0]) > 0xffff:
                    # FIXME, QTBUG-69324: make this redundant:
                    # omit number system if zero doesn't fit in single-char16 UTF-16 :-(
                    continue

                cache[attrs['id']] = attrs
            assert cache
        return cache

    @property
    def __weekData(self, cache = {}):
        if not cache:
            firstDay, weStart, weEnd = self.__getWeekData()
            # Massage those into an easily-consulted form:
            # World defaults given for code '001':
            mon, sat, sun = firstDay['001'], weStart['001'], weEnd['001']
            lands = set(firstDay) | set(weStart) | set(weEnd)
            cache.update((land,
                          (firstDay.get(land, mon), weStart.get(land, sat), weEnd.get(land, sun)))
                         for land in lands)
            assert cache
        return cache

    def __getWeekData(self):
        """Scan for data on the weekly cycle.

        Yields three mappings from locales to en's short names for
        week-days; if a locale isn't a key of a given mapping, it
        should use the '001' (world) locale's value. The first mapping
        gives the day on which the week starts, the second gives the
        day on which the week-end starts, the third gives the last day
        of the week-end."""
        source = self.__supplementalData
        for key in ('firstDay', 'weekendStart', 'weekendEnd'):
            result = {}
            for ignore, attrs in source.find('weekData/' + key):
                assert ignore == key
                day = attrs['day']
                assert day in ('mon', 'tue', 'wed', 'thu', 'fri', 'sat', 'sun'), day
                if 'alt' in attrs:
                    continue
                for loc in attrs.get('territories', '').split():
                    result[loc] = day
            yield result

    @property
    def __currencyData(self, cache = {}):
        if not cache:
            source = self.__supplementalData
            for elt in source.findNodes('currencyData/region'):
                iso, digits, rounding = '', 2, 1
                try:
                    country = elt.dom.attributes['iso3166'].nodeValue
                except KeyError:
                    continue
                for child in elt.findAllChildren('currency'):
                    try:
                        if child.dom.attributes['tender'].nodeValue == 'false':
                            continue
                    except KeyError:
                        pass
                    try:
                        child.dom.attributes['to'] # Is set if this element has gone out of date.
                    except KeyError:
                        iso = child.dom.attributes['iso4217'].nodeValue
                        break
                if iso:
                    for tag, data in source.find(
                        'currencyData/fractions/info[iso4217={}]'.format(iso)):
                        digits = data['digits']
                        rounding = data['rounding']
                cache[country] = iso, digits, rounding
            assert cache

        return cache

    @property
    def __unDistinguishedAttributes(self, cache = {}, joinPath = os.path.join):
        """Mapping from tag names to lists of attributes.

        LDML defines some attributes as 'distinguishing': if a node
        has distinguishing attributes that weren't specified in an
        XPath, a search on that XPath should exclude the node's
        children.

        This property is a mapping from tag names to tuples of
        attribute names that *aren't* distinguishing for that tag.
        Its value is cached (so its costly computation isonly done
        once) and there's a side-effect of populating its cache: it
        sets self.__cldrVersion to the value found in ldml.dtd, during
        parsing."""
        if not cache:
            cache.update(self.__scanLdmlDtd())
            assert cache

        return cache

    def __scanLdmlDtd(self, joinPath = os.path.join):
        """Scan the LDML DTD, record CLDR version

        Yields (tag, attrs) pairs: on elements with a given tag,
        attributes named in its attrs (a tuple) may be ignored in an
        XPath search; other attributes are distinguished attributes,
        in the terminology of LDML's locale-inheritance rules.

        Sets self.__cldrVersion as a side-effect, since this
        information is found in the same file."""
        with self.__open(('common', 'dtd', 'ldml.dtd')) as dtd:
            tag, ignored, last = None, None, None

            for line in dtd:
                if line.startswith('<!ELEMENT '):
                    if ignored:
                        assert tag
                        yield tag, tuple(ignored)
                    tag, ignored, last = line.split()[1], [], None
                    continue

                if line.startswith('<!ATTLIST '):
                    assert tag is not None
                    parts = line.split()
                    assert parts[1] == tag
                    last = parts[2]
                    if parts[1:5] == ['version', 'cldrVersion', 'CDATA', '#FIXED']:
                        # parts[5] is the version, in quotes, although the final > might be stuck on its end:
                        self.__cldrVersion = parts[5].split('"')[1]
                    continue

                # <!ELEMENT...>s can also be @METADATA, but not @VALUE:
                if '<!--@VALUE-->' in line or (last and '<!--@METADATA-->' in line):
                    assert last is not None
                    assert ignored is not None
                    assert tag is not None
                    ignored.append(last)
                    last = None # No attribute is both value and metadata

            if tag and ignored:
                yield tag, tuple(ignored)

    def __enumMap(self, key, cache = {}):
        if not cache:
            cache['variant'] = {'': (0, 'This should never be seen outside ldml.py')}
            # They're not actually lists: mappings from numeric value
            # to pairs of full name and short code. What we want, in
            # each case, is a mapping from code to the other two.
            from enumdata import language_list, script_list, country_list
            for form, book, empty in (('language', language_list, 'AnyLanguage'),
                                      ('script', script_list, 'AnyScript'),
                                      ('country', country_list, 'AnyCountry')):
                cache[form] = dict((pair[1], (num, pair[0]))
                                   for num, pair in book.items() if pair[0] != 'C')
                # (Have to filter out the C locale, as we give it the
                # same (all space) code as AnyLanguage, whose code
                # should probably be 'und' instead.)

                # Map empty to zero and the any value:
                cache[form][''] = (0, empty)
            # and map language code 'und' also to (0, any):
            cache['language']['und'] = (0, 'AnyLanguage')

        return cache[key]

    def __codeMap(self, key, cache = {},
                  # Maps our name for it to CLDR's name:
                  naming = {'language': 'languages', 'script': 'scripts',
                            'country': 'territories', 'variant': 'variants'}):
        if not cache:
            root = self.xml('common', 'main', 'en.xml').root.findUniqueChild('localeDisplayNames')
            for dst, src in naming.items():
                cache[dst] = dict(self.__codeMapScan(root.findUniqueChild(src)))
            assert cache

        return cache[key]

    def __codeMapScan(self, node):
        """Get mapping from codes to element values.

        Passed in node is a <languages>, <scripts>, <territories> or
        <variants> node, each child of which is a <language>,
        <script>, <territory> or <variant> node as appropriate, whose
        type is a code (of the appropriate flavour) and content is its
        full name.  In some cases, two child nodes have the same type;
        in these cases, one always has an alt attribute and we should
        prefer the other.  Yields all such type, content pairs found
        in node's children (skipping any with an alt attribute, if
        their type has been seen previously)."""
        seen = set()
        for elt in node.dom.childNodes:
            try:
                key, value = elt.attributes['type'].nodeValue, elt.childNodes[0].wholeText
            except (KeyError, ValueError, TypeError):
                pass
            else:
                if key not in seen or not elt.attributes.has_key('alt'):
                    yield key, value
                    seen.add(key)

    # CLDR uses inheritance between locales to save repetition:
    def __parentLocale(self, name, cache = {}):
        # see http://www.unicode.org/reports/tr35/#Parent_Locales
        if not cache:
            for tag, attrs in self.__supplementalData.find('parentLocales'):
                parent = attrs.get('parent', '')
                for child in attrs['locales'].split():
                    cache[child] = parent
            assert cache

        return cache[name]

    def __localeAsDoc(self, name, aliasFor = None,
                      joinPath = os.path.join, exists = os.path.isfile):
        path = ('common', 'main', name + '.xml')
        if exists(joinPath(self.root, *path)):
            elt = self.__xml(path)
            for child in Node(elt).findAllChildren('alias'):
                try:
                    alias = child.dom.attributes['source'].nodeValue
                except (KeyError, AttributeError):
                    pass
                else:
                    return self.__localeAsDoc(alias, aliasFor or name)
            # No alias child with a source:
            return elt

        if aliasFor:
            raise Error('Fatal error: found an alias "{}" -> "{}", but found no file for the alias'
                        .format(aliasFor, name))

    def __scanLocaleRoots(self, name):
        while name and name != 'root':
            doc = self.__localeAsDoc(name)
            if doc is not None:
                yield Node(doc, self.__unDistinguishedAttributes)

            try:
                name = self.__parentLocale(name)
            except KeyError:
                try:
                    name, tail = name.rsplit('_', 1)
                except ValueError: # No tail to discard: we're done
                    break

    class __Seq (list): pass # No weakref for tuple and list, but list sub-class is ok.
    def __localeRoots(self, name, cache = CacheDict()):
        try:
            chain = cache[name]
        except KeyError:
            cache[name] = chain = self.__Seq(self.__scanLocaleRoots(name))
        return chain

# Unpolute the namespace: we don't need to export these.
del minidom, CacheDict, os
