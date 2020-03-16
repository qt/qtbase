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
"""Parsing the Locale Data Markup Language

It's an XML format, so the raw parsing of XML is, of course, delegated
to xml.dom.minidom; but it has its own specific schemata and some
funky rules for combining data from various files (inheritance between
locales). The use of it we're interested in is extraction of CLDR's
data, so some of the material here is specific to CLDR; see cldr.py
for how it is mainly used.

Provides various classes to wrap xml.dom's objects, specifically those
returned by minidom.parse() and their child-nodes:
  Node -- wraps any node in the DOM tree
  XmlScanner -- wraps the root element of a stand-alone XML file
  Supplement -- specializes XmlScanner for supplemental data files
  LocaleScanner -- wraps a locale's inheritance-chain of file roots

See individual classes for further detail.
"""
from localetools import Error
from dateconverter import convert_date

class Node (object):
    """Wrapper for an arbitrary DOM node.

    Provides various ways to select chldren of a node. Selected child
    nodes are returned wrapped as Node objects.  A Node exposes the
    raw DOM node it wraps via its .dom attribute."""

    def __init__(self, elt, dullAttrs = None, draft = 0):
        """Wraps a DOM node for ease of access.

        First argument, elt, is the DOM node to wrap.

        Optional second argument, dullAttrs, should either be None or
        map each LDML tag name to a list of the names of
        non-distinguishing attributes for nodes with the given tag
        name. If None is given, no distinguishing attribute checks are
        performed.

        (Optional third argument, draft, should only be supplied by
        this class's creation of child nodes; it is the maximum draft
        score of any ancestor of the new node.)"""
        self.dom, self.__dull = elt, dullAttrs
        try:
            attr = elt.attributes['draft'].nodeValue
        except KeyError:
            self.draft = draft
        else:
            self.draft = max(draft, self.draftScore(attr))

    def findAllChildren(self, tag, wanted = None, allDull = False):
        """All children that do have the given tag and attributes.

        First argument is the tag: children with any other tag are
        ignored.

        Optional second argument, wanted, should either be None or map
        attribute names to the values they must have. Only child nodes
        with thes attributes set to the given values are yielded.

        By default, nodes that have distinguishing attributes, other
        than those specified in wanted, are ignored.  Pass the allDull
        parameter a true value to suppress this check."""

        if self.__dull is None:
            allDull = True
        dull = () if allDull else self.__dull[tag]

        for child in self.dom.childNodes:
            if child.nodeType != child.ELEMENT_NODE:
                continue
            if child.nodeName != tag:
                continue

            if wanted:
                try:
                    if any(child.attributes[k].nodeValue != v
                           for k, v in wanted.items()):
                        continue
                except KeyError: # Some wanted attribute is missing
                    continue

                if not (allDull or all(k in dull or k in wanted
                                       for k in child.attributes.keys())):
                    continue

            elif not (allDull or all(k in dull
                                     for k in child.attributes.keys())):
                continue

            yield Node(child, self.__dull, self.draft)

    def findUniqueChild(self, tag):
        """Returns the single child with the given nodeName.

        Raises Error if there is no such child or there is more than
        one."""
        seq = self.findAllChildren(tag)
        try:
            node = seq.next()
        except StopIteration:
            raise Error('No child found where one was expected', tag)
        for it in seq:
            raise Error('Many children found where only one was expected', tag)
        return node

    @classmethod
    def draftScore(cls, level):
        """Maps draft level names to numeric scores.

        Single parameter, level, is the least sure value of the draft
        attribute on a node that you're willing to accept; returns a
        numeric value (lower is less drafty).

        Tempting as it is to insist on low draft scores, there are
        many locales in which pretty much every leaf is
        unconfirmed. It may make sense to actually check each
        XmlScanner object, or each node in each LocaleScanner's nodes
        list, to see what its distribution of draft level looks like,
        so as to set the acceptable draft score for its elements
        accordingly. However, for the moment, we mostly just accept
        all elements, regardless of draft values (the one exception is
        am/pm indicators)."""
        return cls.__draftScores.get(level, 5) if level else 0

    # Implementation details:
    __draftScores = dict(true = 4, unconfirmed = 3, provisional = 2,
                         contributed = 1, approved = 0, false = 0)

def _parseXPath(selector):
    # Split "tag[attr=val][...]" into tag-name and attribute mapping
    attrs = selector.split('[')
    name = attrs.pop(0)
    if attrs:
        attrs = [x.strip() for x in attrs]
        assert all(x.endswith(']') for x in attrs)
        attrs = [x[:-1].split('=') for x in attrs]
        assert all(len(x) in (1, 2) for x in attrs)
        attrs = (('type', x[0]) if len(x) == 1 else x for x in attrs)
    return name, dict(attrs)

def _iterateEach(iters):
    # Flatten a two-layer iterator.
    for it in iters:
        for item in it:
            yield item

class XmlScanner (object):
    """Wrap an XML file to enable XPath access to its nodes.
    """
    def __init__(self, node):
        self.root = node

    def findNodes(self, xpath):
        """Return all nodes under self.root matching this xpath.

        Ignores any excess attributes."""
        elts = (self.root,)
        for selector in xpath.split('/'):
            tag, attrs = _parseXPath(selector)
            elts = tuple(_iterateEach(e.findAllChildren(tag, attrs) for e in elts))
            if not elts:
                break
        return elts

class Supplement (XmlScanner):
    def find(self, xpath):
        elts = self.findNodes(xpath)
        for elt in _iterateEach(e.dom.childNodes if e.dom.childNodes else (e.dom,)
                                for e in elts):
            if elt.attributes:
                yield (elt.nodeName,
                       dict((k, v if isinstance(v, basestring) else v.nodeValue)
                            for k, v in elt.attributes.items()))

class LocaleScanner (object):
    def __init__(self, name, nodes, root):
        self.name, self.nodes, self.base = name, nodes, root

    def find(self, xpath, default = None, draft = None):
        """XPath search for the content of an element.

        Required argument, xpath, is the XPath to search for. Optional
        second argument is a default value to use, if no such node is
        found.  Optional third argument is a draft score (see
        Node.draftScore() for details); if given, leaf elements with
        higher draft scores are ignored."""
        try:
            for elt in self.__find(xpath):
                try:
                    if draft is None or elt.draft <= draft:
                        return elt.dom.firstChild.nodeValue
                except (AttributeError, KeyError):
                    pass
        except Error as e:
            if default is None:
                raise
            return default

    def tagCodes(self):
        """Yields four tag codes

        The tag codes are language, script, country and variant; an
        empty value for any of them indicates that no value was
        provided.  The values are obtained from the primary file's
        top-level <identity> element.  An Error is raised if any
        top-level <alias> element of this file has a non-empty source
        attribute; that attribute value is mentioned in the error's
        message."""
        root = self.nodes[0]
        for alias in root.findAllChildren('alias', allDull=True):
            try:
                source = alias.dom.attributes['source'].nodeValue
            except (KeyError, AttributeError):
                pass
            else:
                raise Error('Alias to {}'.format(source))

        ids = root.findUniqueChild('identity')
        for code in ('language', 'script', 'territory', 'variant'):
            for node in ids.findAllChildren(code, allDull=True):
                try:
                    yield node.dom.attributes['type'].nodeValue
                except (KeyError, AttributeError):
                    pass
                else:
                    break # only want one value for each code
            else: # No value for this code, use empty
                yield ''

    def currencyData(self, isoCode):
        """Fetches currency data for this locale.

        Single argument, isoCode, is the ISO currency code for the
        currency in use in the country. See also numericData, which
        includes some currency formats.
        """
        if isoCode:
            stem = 'numbers/currencies/currency[{}]/'.format(isoCode)
            symbol = self.find(stem + 'symbol', '')
            name = ';'.join(
                self.find(stem + 'displayName' + tail, '')
                for tail in ('',) + tuple(
                    '[count={}]'.format(x) for x in ('zero', 'one', 'two', 'few', 'many', 'other')
                )) + ';'
        else:
            symbol = name = ''
        yield 'currencySymbol', symbol
        yield 'currencyDisplayName', name

    def numericData(self, lookup, complain = lambda text: None):
        """Generate assorted numeric data for the locale.

        First argument, lookup, is a callable that maps a numbering
        system's name to certain data about the system, as a mapping;
        we expect this to have u'digits' as a key.
        """
        system = self.find('numbers/defaultNumberingSystem')
        stem = 'numbers/symbols[numberSystem={}]/'.format(system)
        decimal = self.find(stem + 'decimal')
        group = self.find(stem + 'group')
        assert decimal != group, (self.name, system, decimal)
        yield 'decimal', decimal
        yield 'group', group
        yield 'percent', self.find(stem + 'percentSign')
        yield 'list', self.find(stem + 'list')
        # FIXME: don't lower-case:
        yield 'exp', self.find(stem + 'exponential').lower()

        digits = lookup(system)['digits']
        assert len(digits) == 10
        zero = digits[0]
        # Qt's number-formatting code assumes digits are consecutive:
        assert all(ord(c) == i for i, c in enumerate(digits, ord(zero)))
        yield 'zero', zero

        plus = self.find(stem + 'plusSign')
        minus = self.find(stem + 'minusSign')
        yield 'plus', plus
        yield 'minus', minus

        # Currency formatting:
        xpath = 'numbers/currencyFormats/currencyFormatLength/currencyFormat[accounting]/pattern'
        try:
            money = self.find(xpath.replace('Formats/',
                                            'Formats[numberSystem={}]/'.format(system)))
        except Error:
            money = self.find(xpath)
        money = self.__currencyFormats(money, plus, minus)
        yield 'currencyFormat', money.next()
        neg = ''
        for it in money:
            assert not neg, 'There should be at most one more pattern'
            neg = it
        yield 'currencyNegativeFormat', neg

    def textPatternData(self):
        for key in ('quotationStart', 'alternateQuotationEnd',
                    'quotationEnd', 'alternateQuotationStart'):
            yield key, self.find('delimiters/' + key)

        for key in ('start', 'middle', 'end'):
            yield ('listPatternPart' + key.capitalize(),
                   self.__fromLdmlListPattern(self.find(
                        'listPatterns/listPattern/listPatternPart[{}]'.format(key))))
        yield ('listPatternPartTwo',
               self.__fromLdmlListPattern(self.find(
                    'listPatterns/listPattern/listPatternPart[2]')))

        stem = 'dates/calendars/calendar[gregorian]/'
        # TODO: is wide really the right width to use here ?
        # abbreviated might be an option ... or try both ?
        meridiem = stem + 'dayPeriods/dayPeriodContext[format]/dayPeriodWidth[wide]/'
        for key in ('am', 'pm'):
            yield key, self.find(meridiem + 'dayPeriod[{}]'.format(key),
                                 draft = Node.draftScore('contributed'))

        for pair in (('long', 'full'), ('short', 'short')):
            for key in ('time', 'date'):
                yield (pair[0] + key.capitalize() + 'Format',
                       convert_date(self.find(
                            stem + '{}Formats/{}FormatLength[{}]/{}Format/pattern'.format(
                                key, key, pair[1], key))))

    def endonyms(self, language, script, country, variant):
        # TODO: take variant into account ?
        for seq in ((language, script, country),
                    (language, script), (language, country), (language,)):
            if not all(seq):
                continue
            try:
                yield ('languageEndonym',
                       self.find('localeDisplayNames/languages/language[{}]'
                                 .format('_'.join(seq))))
            except Error:
                pass
            else:
                break
        else:
            # grumble(failed to find endonym for language)
            yield 'languageEndonym', ''

        yield ('countryEndonym',
               self.find('localeDisplayNames/territories/territory[{}]'
                         .format(country), ''))

    def unitData(self):
        yield ('byte_unit',
               self.find('units/unitLength[long]/unit[digital-byte]/displayName',
                         'bytes'))

        unit = self.__findUnit('', 'B')
        cache = [] # Populated by the SI call, to give hints to the IEC call
        yield ('byte_si_quantified',
               ';'.join(self.__unitCount('', unit, cache)))
        # IEC 60027-2
        # http://physics.nist.gov/cuu/Units/binary.html
        yield ('byte_iec_quantified',
               ';'.join(self.__unitCount('bi', 'iB', cache)))

    def calendarNames(self, calendars):
        namings = self.__nameForms
        for cal in calendars:
            stem = 'dates/calendars/calendar[' + cal + ']/months/'
            for key, mode, size in namings:
                prop = 'monthContext[' + mode + ']/monthWidth[' + size + ']/'
                yield (key + 'Months_' + cal,
                       ';'.join(self.find(stem + prop + 'month[{}]'.format(i))
                                for i in range(1, 13)) + ';')

        # Day data (for Gregorian, at least):
        stem = 'dates/calendars/calendar[gregorian]/days/'
        days = ('sun', 'mon', 'tue', 'wed', 'thu', 'fri', 'sat')
        for (key, mode, size) in namings:
            prop = 'dayContext[' + mode + ']/dayWidth[' + size + ']/day'
            yield (key + 'Days',
                   ';'.join(self.find(stem + prop + '[' + day + ']')
                            for day in days) + ';')

    # Implementation details
    __nameForms = (
        ('standaloneLong', 'stand-alone', 'wide'),
        ('standaloneShort', 'stand-alone', 'abbreviated'),
        ('standaloneNarrow', 'stand-alone', 'narrow'),
        ('long', 'format', 'wide'),
        ('short', 'format', 'abbreviated'),
        ('narrow', 'format', 'narrow'),
        ) # Used for month and day names

    def __find(self, xpath):
        retries = [ xpath.split('/') ]
        while retries:
            tags, elts, roots = retries.pop(), self.nodes, (self.base.root,)
            for selector in tags:
                tag, attrs = _parseXPath(selector)
                elts = tuple(_iterateEach(e.findAllChildren(tag, attrs) for e in elts))
                if not elts:
                    break

            else: # Found matching elements
                # Possibly filter elts to prefer the least drafty ?
                for elt in elts:
                    yield elt

            # Process roots separately: otherwise the alias-processing
            # is excessive.
            for i, selector in enumerate(tags):
                tag, attrs = _parseXPath(selector)

                for alias in tuple(_iterateEach(r.findAllChildren('alias', allDull=True)
                                                for r in roots)):
                    if alias.dom.attributes['source'].nodeValue == 'locale':
                        replace = alias.dom.attributes['path'].nodeValue.split('/')
                        retries.append(self.__xpathJoin(tags[:i], replace, tags[i:]))

                roots = tuple(_iterateEach(r.findAllChildren(tag, attrs) for r in roots))
                if not roots:
                    if retries: # Let outer loop fall back on an alias path:
                        break
                    sought = '/'.join(tags)
                    if sought != xpath:
                        sought += ' (for {})'.format(xpath)
                    raise Error('All lack child {} for {} in {}'.format(
                            selector, sought, self.name))

            else: # Found matching elements
                for elt in roots:
                    yield elt

        sought = '/'.join(tags)
        if sought != xpath:
            sought += ' (for {})'.format(xpath)
        raise Error('No {} in {}'.format(sought, self.name))

    def __findUnit(self, keySuffix, quantify, fallback=''):
        # The displayName for a quantified unit in en.xml is kByte
        # (even for unitLength[narrow]) instead of kB (etc.), so
        # prefer any unitPattern provided, but prune its placeholder:
        for size in ('short', 'narrow'): # TODO: reverse order ?
            stem = 'units/unitLength[{}]/unit[digital-{}byte]/'.format(size + keySuffix, quantify)
            for count in ('many', 'few', 'two', 'other', 'zero', 'one'):
                try:
                    ans = self.find(stem + 'unitPattern[count={}]'.format(count))
                except Error:
                    continue

                # TODO: do count-handling, instead of discarding placeholders
                if False: # TODO: do it this way, instead !
                    ans = ans.replace('{0}', '').strip()
                elif ans.startswith('{0}'):
                    ans = ans[3:].lstrip()
                if ans:
                    return ans

            try:
                return self.find(stem + 'displayName')
            except Error:
                pass

        return fallback

    def __unitCount(self, keySuffix, suffix, cache,
                    # Stop at exa/exbi: 16 exbi = 2^{64} < zetta =
                    # 1000^7 < zebi = 2^{70}, the next quantifiers up:
                    siQuantifiers = ('kilo', 'mega', 'giga', 'tera', 'peta', 'exa')):
        """Work out the unit quantifiers.

        Unfortunately, the CLDR data only go up to terabytes and we
        want all the way to exabytes; but we can recognize the SI
        quantifiers as prefixes, strip and identify the tail as the
        localized translation for 'B' (e.g. French has 'octet' for
        'byte' and uses ko, Mo, Go, To from which we can extrapolate
        Po, Eo).

        Should be called first for the SI quantifiers, with suffix =
        'B', then for the IEC ones, with suffix = 'iB'; the list cache
        (initially empty before first call) is used to let the second
        call know what the first learned about the localized unit.
        """
        if suffix == 'iB': # second call, re-using first's cache
            if cache:
                byte = cache.pop()
                if all(byte == k for k in cache):
                    suffix = 'i' + byte
            for q in siQuantifiers:
                # Those don't (yet, v36) exist in CLDR, so we always get the fall-back:
                yield self.__findUnit(keySuffix, q[:2], q[0].upper() + suffix)
        else: # first call
            tail = suffix = suffix or 'B'
            for q in siQuantifiers:
                it = self.__findUnit(keySuffix, q)
                # kB for kilobyte, in contrast with KiB for IEC:
                q = q[0] if q == 'kilo' else q[0].upper()
                if not it:
                    it = q + tail
                elif it.startswith(q):
                    rest = it[1:]
                    tail = rest if all(rest == k for k in cache) else suffix
                    cache.append(rest)
                yield it

    @staticmethod
    def __currencyFormats(patterns, plus, minus):
        for p in patterns.split(';'):
            p = p.replace('0', '#').replace(',', '').replace('.', '')
            try:
                cut = p.find('#') + 1
            except ValueError:
                pass
            else:
                p = p[:cut] + p[cut:].replace('#', '')
            p = p.replace('#', "%1")
            # According to http://www.unicode.org/reports/tr35/#Number_Format_Patterns
            # there can be doubled or trippled currency sign, however none of the
            # locales use that.
            p = p.replace(u'\xa4', "%2")
            # Single quote goes away, but double goes to single:
            p = p.replace("''", '###').replace("'", '').replace('###', "'")
            # Use number system's signs:
            p = p.replace('+', plus).replace('-', minus)
            yield p

    @staticmethod
    def __fromLdmlListPattern(pattern):
        # This is a very limited parsing of the format for list pattern part only.
        return pattern.replace('{0}', '%1').replace('{1}', '%2').replace('{2}', '%3')

    @staticmethod
    def __fromLdmlPath(seq): # tool function for __xpathJoin()
        """Convert LDML's [@name='value'] to our [name=value] form."""
        for it in seq:
            # First dismember it:
            attrs = it.split('[')
            tag = attrs.pop(0)
            if not attrs: # Short-cut the easy case:
                yield it
                continue

            assert all(x.endswith(']') for x in attrs)
            attrs = [x[:-1].split('=') for x in attrs]
            # Then fix each attribute specification in it:
            attrs = [(x[0][1:] if x[0].startswith('@') else x[0],
                      x[1][1:-1] if x[1].startswith("'") and x[1].endswith("'") else x[1])
                     for x in attrs]
            # Finally, put it all back together:
            attrs = ['='.join(x) + ']' for x in attrs]
            attrs.insert(0, tag)
            yield '['.join(attrs)

    @classmethod
    def __xpathJoin(cls, head, insert, tail):
        """Join three lists of XPath selectors.

        Each of head, insert and tail is a sequence of selectors but
        insert may start with some uses of '..', that we want to
        resolve away, and may use LDML's attribute format, that we
        want to convert to our format."""
        while insert and insert[0] == '..':
            insert.pop(0)
            head.pop()
        return head + list(cls.__fromLdmlPath(insert)) + tail
