# Copyright (C) 2020 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
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
from __future__ import annotations
from localetools import Error
from dateconverter import convert_date
from typing import Any, Callable, Iterator
from xml.dom import minidom

# The github version of CLDR uses '↑↑↑' to indicate "inherit"
INHERIT = '↑↑↑'

def _attrsFromDom(dom: minidom.Element) -> dict[str, str]:
    return { k: (v if isinstance(v, str) else v.nodeValue)
             for k, v in dom.attributes.items() }

class Node (object):
    """Wrapper for an arbitrary DOM node.

    Provides various ways to select children of a node. Selected child
    nodes are returned wrapped as Node objects.  A Node exposes the
    raw DOM node it wraps via its .dom attribute."""

    def __init__(self, elt: minidom.Element,
                 dullAttrs: dict[str, tuple[str, ...]]|None = None, draft: int = 0) -> None:
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
            attr: str = elt.attributes['draft'].nodeValue
        except KeyError:
            self.draft = draft
        else:
            self.draft = max(draft, self.draftScore(attr))

    def attributes(self) -> dict[str, str]:
        return _attrsFromDom(self.dom)

    def findAllChildren(self, tag: str, wanted: dict[str, str]|None = None,
                        allDull: bool = False) -> Iterator[Node]:
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
        dull: tuple[str, ...] = () if allDull else self.__dull[tag]

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

    def findUniqueChild(self, tag: str) -> Node:
        """Returns the single child with the given nodeName.

        Raises Error if there is no such child or there is more than
        one."""
        seq: Iterator[Node] = self.findAllChildren(tag)
        try:
            node: Node = next(seq)
        except StopIteration:
            raise Error('No child found where one was expected', tag)
        for it in seq:
            raise Error('Many children found where only one was expected', tag)
        return node

    @classmethod
    def draftScore(cls, level: str) -> int:
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

def _parseXPath(selector: str) -> tuple[str, dict[str, str]]:
    # Split "tag[attr=val][...]" into tag-name and attribute mapping
    attrs: list[str] = selector.split('[')
    name: str = attrs.pop(0)
    if attrs:
        attrs = [x.strip() for x in attrs]
        assert all(x.endswith(']') for x in attrs)
        attrs: list[list[str]] = [x[:-1].split('=') for x in attrs]
        assert all(len(x) in (1, 2) for x in attrs)
        attrs: Iterator[tuple[str, str]] = (('type', x[0]) if len(x) == 1 else x for x in attrs)
    return name, dict(attrs)

def _iterateEach(iters: Iterator[Iterator[Any]]) -> Iterator[Any]:
    # Flatten a two-layer iterator.
    for it in iters:
        for item in it:
            yield item

class XmlScanner (object):
    """Wrap an XML file to enable XPath access to its nodes.
    """
    def __init__(self, node: Node) -> None:
        self.root = node

    def findNodes(self, xpath: str) -> tuple[Node, ...]:
        """Return all nodes under self.root matching this xpath.

        Ignores any excess attributes."""
        elts: tuple[Node, ...] = (self.root,)
        for selector in xpath.split('/'):
            # tag is a str and attrs is a dict[str, str]
            tag, attrs = _parseXPath(selector)
            elts = tuple(_iterateEach(e.findAllChildren(tag, attrs) for e in elts))
            if not elts:
                break
        return elts

class Supplement (XmlScanner):
    def find(self, xpath: str,
             exclude: tuple[str, ...] = ()) -> Iterator[tuple[str, dict[str, str]]]:
        """Finds nodes by matching a specified xpath.

        If exclude is passed, it should be a sequence of attribute names (its
        default is empty). Any matches to the given xpath that also have any
        attribute in this sequence will be excluded.

        For each childless node matching the xpath, or child of a node matching
        the xpath, this yields a twople (name, attrs) where name is the
        nodeName and attrs is a dict mapping the node's attribute's names to
        their values. For attribute values that are not simple strings, the
        nodeValue of the attribute node is used."""
        elts: tuple[Node, ...] = self.findNodes(xpath)
        for elt in _iterateEach(e.dom.childNodes or (e.dom,)
                                for e in elts
                                if not any(a in e.dom.attributes
                                           for a in exclude)):
            if elt.attributes:
                yield elt.nodeName, _attrsFromDom(elt)

class LocaleScanner (object):
    def __init__(self, name: str, nodes: list[Node], root: XmlScanner) -> None:
        """Set up to scan data for a specified locale.

        First parameter is the name of the locale; it will be used in
        error messages. Second is a sequence of DOM root-nodes of files
        with locale data, later ones serving as fall-backs for data
        missing in earlier ones. Third parameter is the root locale's
        DOM node."""
        self.name, self.nodes, self.base = name, nodes, root

    def find(self, xpath: str, default: str|None = None, draft: int|None = None) -> str:
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

    def tagCodes(self) -> Iterator[str]:
        """Yields four tag codes

        The tag codes are language, script, territory and variant; an
        empty value for any of them indicates that no value was
        provided.  The values are obtained from the primary file's
        top-level <identity> element."""
        root: Node = self.nodes[0]
        ids: Node = root.findUniqueChild('identity')
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

    def currencyData(self, isoCode: str) -> Iterator[tuple[str, str]]:
        """Fetches currency data for this locale.

        Single argument, isoCode, is the ISO currency code for the
        currency in use in the territory. See also numericData, which
        includes some currency formats.
        """
        if isoCode:
            stem: str = f'numbers/currencies/currency[{isoCode}]/'
            symbol: str = self.find(f'{stem}symbol', '')
            name: str = self.__currencyDisplayName(stem)
        else:
            symbol = name = ''
        yield 'currencySymbol', symbol
        yield 'currencyDisplayName', name

    def numericData(self, lookup: Callable[[str], dict[str, str]]
                    ) -> Iterator[tuple[str, str]]:
        """Generate assorted numeric data for the locale.

        First argument, lookup, is a callable that maps a numbering
        system's name to certain data about the system, as a mapping;
        we expect this to have 'digits' as a key.
        """
        system: str = self.find('numbers/defaultNumberingSystem')
        stem: str = f'numbers/symbols[numberSystem={system}]/'
        decimal: str = self.find(f'{stem}decimal')
        group: str = self.find(f'{stem}group')
        if decimal == group:
            # mn_Mong_MN @v43 :-(
            # https://unicode-org.atlassian.net/browse/CLDR-17883
            # https://unicode-org.atlassian.net/browse/CLDR-16413
            clean: int = Node.draftScore('approved')
            decimal = self.find(f'{stem}decimal', draft=clean)
            group = self.find(f'{stem}group', draft=clean)
            assert decimal != group, (self.name, system, decimal)

        yield 'decimal', decimal
        yield 'group', group
        yield 'percent', self.find(f'{stem}percentSign')
        yield 'list', self.find(f'{stem}list')
        yield 'exp', self.find(f'{stem}exponential')
        yield 'groupSizes', self.__numberGrouping(system)

        digits: str = lookup(system)['digits']
        assert len(digits) == 10
        zero: str = digits[0]
        # Qt's number-formatting code assumes digits are consecutive
        # (except Suzhou - see QTBUG-85409 - which shares its zero
        # with CLDR's very-non-contiguous hanidec):
        assert all(ord(c) == i + (0x3020 if ord(zero) == 0x3007 else ord(zero))
                   for i, c in enumerate(digits[1:], 1))
        yield 'zero', zero

        plus: str = self.find(f'{stem}plusSign')
        minus: str = self.find(f'{stem}minusSign')
        yield 'plus', plus
        yield 'minus', minus

        # Currency formatting:
        xpath = 'numbers/currencyFormats/currencyFormatLength/currencyFormat[accounting]/pattern'
        try:
            money: str = self.find(xpath.replace('Formats/',
                                            f'Formats[numberSystem={system}]/'))
        except Error:
            money = self.find(xpath)
        money = self.__currencyFormats(money, plus, minus)
        yield 'currencyFormat', next(money)
        neg = ''
        for it in money:
            assert not neg, 'There should be at most one more pattern'
            neg = it
        yield 'currencyNegativeFormat', neg

    def textPatternData(self) -> Iterator[tuple[str, str]]:
        for key in ('quotationStart', 'alternateQuotationEnd',
                    'quotationEnd', 'alternateQuotationStart'):
            yield key, self.find(f'delimiters/{key}')

        for key in ('start', 'middle', 'end'):
            yield (f'listPatternPart{key.capitalize()}',
                   self.__fromLdmlListPattern(self.find(
                        f'listPatterns/listPattern/listPatternPart[{key}]')))
        yield ('listPatternPartTwo',
               self.__fromLdmlListPattern(self.find(
                    'listPatterns/listPattern/listPatternPart[2]')))

        stem = 'dates/calendars/calendar[gregorian]/'
        # TODO: is wide really the right width to use here ?
        # abbreviated might be an option ... or try both ?
        meridiem = f'{stem}dayPeriods/dayPeriodContext[format]/dayPeriodWidth[wide]/'
        for key in ('am', 'pm'):
            yield key, self.find(f'{meridiem}dayPeriod[{key}]',
                                 draft = Node.draftScore('contributed'))

        for pair in (('long', 'full'), ('short', 'short')):
            for key in ('time', 'date'):
                yield (f'{pair[0]}{key.capitalize()}Format',
                       convert_date(self.find(
                            f'{stem}{key}Formats/{key}FormatLength[{pair[1]}]/{key}Format/pattern')))

    def endonyms(self, language: str, script: str, territory: str, variant: str
                 ) -> Iterator[tuple[str, str]]:
        # TODO: take variant into account ?
        # TODO: QTBUG-47892, support query for all combinations
        for seq in ((language, script, territory),
                    (language, script), (language, territory), (language,)):
            if not all(seq):
                continue
            try:
                yield ('languageEndonym',
                       self.find(f'localeDisplayNames/languages/language[{"_".join(seq)}]'))
            except Error:
                pass
            else:
                break
        else:
            # grumble(failed to find endonym for language)
            yield 'languageEndonym', ''

        yield ('territoryEndonym',
               self.find(f'localeDisplayNames/territories/territory[{territory}]', ''))

    def unitData(self) -> Iterator[tuple[str, str]]:
        yield ('byte_unit',
               self.find('units/unitLength[long]/unit[digital-byte]/displayName',
                         'bytes'))

        unit: str | None = self.__findUnit('', 'B')
        cache = [] # Populated by the SI call, to give hints to the IEC call
        yield ('byte_si_quantified',
               ';'.join(self.__unitCount('', unit, cache)))
        # IEC 60027-2
        # http://physics.nist.gov/cuu/Units/binary.html
        yield ('byte_iec_quantified',
               ';'.join(self.__unitCount('bi', 'iB', cache)))

    def calendarNames(self, calendars: list[str]) -> Iterator[tuple[str, str]]:
        namings: tuple[tuple[str, str, str], ...] = self.__nameForms
        for cal in calendars:
            stem = f'dates/calendars/calendar[{cal}]/months/'
            for key, mode, size in namings:
                prop = f'monthContext[{mode}]/monthWidth[{size}]/'
                yield (f'{key}Months_{cal}',
                       ';'.join(self.find(f'{stem}{prop}month[{i}]')
                                for i in range(1, 13)))

        # Day data (for Gregorian, at least):
        stem = 'dates/calendars/calendar[gregorian]/days/'
        days = ('sun', 'mon', 'tue', 'wed', 'thu', 'fri', 'sat')
        for (key, mode, size) in namings:
            prop = f'dayContext[{mode}]/dayWidth[{size}]/day'
            yield (f'{key}Days',
                   ';'.join(self.find(f'{stem}{prop}[{day}]')
                            for day in days))

    def timeZoneNames(self, alias: dict[str, str]) -> Iterator[tuple[str, Any]]:
        """Names of zones and metazones for this locale.

        Single argument, alias, should be the first return from
        CldrAccess.bcp47Aliases(), used to map CLDR zone IDs to IANA
        zone IDs.

        Reads the dates/timeZoneNames/ element to extract data on how
        this locale names time zones and metazones. Yields name-value
        pairs for attributes of a locale. Most have text values but
        three are structured:

          * 'regionZoneFormats' maps to a triple, (generic, standard,
            daylight-saving) of formats, with a %0 in each for the
            exemplar city or territory of a zone.
          * 'zoneNaming' maps to a mapping from IANA IDs to a dicts
            with keys 'exemplarCity', 'long' and 'short'.
          * 'metaZoneNaming' maps to a mapping from metazone names to
             dicts with keys 'long' and 'short'.

        In both of the latter, 'long' and 'short' map to triples
        (generic, standard, daylight-saving) of locale-appropriate
        names, for the zone or metazone, as appropriate. The value for
        'exemplarCity' is the locale-appropriate name of a city by
        which to identify the timezone.

        See CldrAccess.readMetaZoneMap() for the locale-independent
        data that stitches these pieces together."""
        stem = 'dates/timeZoneNames'
        # '+HH:mm;-HH:mm' (and :ss is also supported, but nowhere used in v45).
        # Sometimes has single-digit hours
        hours: list[str] = self.find(f'{stem}/hourFormat').split(';')
        assert all('H' in f and 'm' in f for f in hours), (hours, self.name)
        yield 'positiveOffsetFormat', hours[0]
        yield 'negativeOffsetFormat', hours[1]

        get: Callable[[str], str] = lambda n, s=stem: self.find(f'{s}/{n}').replace('{0}', '%0')
        # 'GMT{0}' with offset as {0}
        yield 'gmtOffsetFormat', get('gmtFormat')
        # '{0} (Winter|Summer)? Time' with exemplar city or territory as {0}
        yield 'regionZoneFormats', (get('regionFormat'),
                                    get('regionFormat[standard]'),
                                    get('regionFormat[daylight]'))
        # '{1} ({0})' with {1} as meta-zone name (long or short), city
        # or territory as {0}.
        yield 'fallbackZoneFormat', get('fallbackFormat').replace('{1}', '%1')

        zones: dict[str, str|dict[str, str]] = {}
        for elt in self.__find(f'{stem}/zone', allDull=True):
            iana: str = elt.attributes()['type']
            if iana == 'Etc/Unknown': # CLDR special, of no interest to us
                continue
            # Map CLDR-canonical ID to IANA ID:
            iana = alias.get(iana, iana)
            data: dict[str, str|dict[str, str]] = zones.setdefault(iana, {})

            for child in elt.dom.childNodes:
                if child.nodeType != child.ELEMENT_NODE:
                    continue
                tag = child.nodeName
                # We don't want fall-backs from parent locales to
                # override what a more specific locale has said:
                if tag == 'exemplarCity':
                    # Localized name of city to identify this zone with:
                    if tag not in data:
                        value = child.firstChild.nodeValue
                        if value != INHERIT:
                            data[tag] = value
                elif tag in ('long', 'short'):
                    data[tag] = self.__zoneNames(child, data.get(tag, {}))
                # Ignore any other child nodes.
        yield 'zoneNaming', self.__cleanZoneMap(zones)

        metazones: dict[str, dict[str, str]] = {}
        for elt in self.__find(f'{stem}/metazone', allDull=True, mustFind=False):
            meta: str = elt.attributes()['type']
            data = metazones.setdefault(meta, {})
            for child in elt.dom.childNodes:
                if child.nodeType != child.ELEMENT_NODE:
                    continue
                tag = child.nodeName
                if tag in ('long', 'short'):
                    data[tag] = self.__zoneNames(child, data.get(tag, {}))
                # Ignore any other child nodes.
        yield 'metaZoneNaming', self.__cleanZoneMap(metazones)

    # Implementation details
    __nameForms = (
        ('standaloneLong', 'stand-alone', 'wide'),
        ('standaloneShort', 'stand-alone', 'abbreviated'),
        ('standaloneNarrow', 'stand-alone', 'narrow'),
        ('long', 'format', 'wide'),
        ('short', 'format', 'abbreviated'),
        ('narrow', 'format', 'narrow'),
    ) # Used for month and day names

    def __find(self, xpath: str, allDull: bool = False, mustFind: bool = True) -> Iterator[Node]:
        retries: list[list[str]] = [ xpath.split('/') ]
        while retries:
            tags: list[str] = retries.pop()
            elts: tuple[Node, ...] = tuple(self.nodes)
            roots: tuple[Node] = (self.base.root,)
            for selector in tags:
                # tag is a str, attrs is a dict[str, str]
                tag, attrs = _parseXPath(selector)
                elts = tuple(_iterateEach(e.findAllChildren(tag, attrs,
                                                            allDull=allDull)
                                          for e in elts))
                if not elts:
                    break

            else: # Found matching elements
                elts = tuple(self.__skipInheritors(elts))
                if elts:
                    mustFind = False
                # Possibly filter elts to prefer the least drafty ?
                for elt in elts:
                    yield elt

            # Process roots separately: otherwise the alias-processing
            # is excessive (and alias only ever shows up in root.xml,
            # always with source="locale").
            for i, selector in enumerate(tags):
                # tag is a str, attrs is a dict[str, str]
                tag, attrs = _parseXPath(selector)

                for alias in tuple(_iterateEach(r.findAllChildren('alias', allDull=True)
                                                for r in roots)):
                    assert alias.dom.attributes['source'].nodeValue == 'locale', alias
                    replace: list[str] = alias.dom.attributes['path'].nodeValue.split('/')
                    retries.append(self.__xpathJoin(tags[:i], replace, tags[i:]))

                roots = tuple(_iterateEach(r.findAllChildren(tag, attrs,
                                                             allDull=allDull)
                                           for r in roots))
                if not roots:
                    if retries: # Let outer loop fall back on an alias path:
                        break
                    if mustFind:
                        sought = '/'.join(tags)
                        if sought != xpath:
                            sought += f' (for {xpath})'
                        raise Error(f'All lack child {selector} for {sought} in {self.name}')

            else: # Found matching elements
                roots = tuple(self.__skipInheritors(roots))
                if roots:
                    mustFind = False
                for elt in roots:
                    yield elt

        if mustFind:
            sought = '/'.join(tags)
            if sought != xpath:
                sought += f' (for {xpath})'
            raise Error(f'No {sought} in {self.name}')

    @staticmethod
    def __skipInheritors(elts: tuple[Node, ...]) -> Iterator[Node]:
        for elt in elts:
            try:
                if elt.dom.firstChild.nodeValue != INHERIT:
                    yield elt
            except (AttributeError, KeyError):
                yield elt

    def __currencyDisplayName(self, stem: str) -> str | None:
        try:
            return self.find(stem + 'displayName')
        except Error:
            pass
        for x in ('zero', 'one', 'two', 'few', 'many', 'other'):
            try:
                return self.find(f'{stem}displayName[count={x}]')
            except Error:
                pass
        return ''

    @staticmethod
    def __zoneNames(dom: minidom.Element, data: dict[str, str]) -> dict[str, str]:
        for child in dom.childNodes:
            if child.nodeType != child.ELEMENT_NODE:
                continue
            tag = child.nodeName
            if tag in data:
                continue
            if tag in ('generic', 'standard', 'daylight'):
                value = child.firstChild.nodeValue
                if value != INHERIT:
                    data[tag] = value

        return data

    @classmethod
    def __cleanZoneMap(cls, zoneMap: dict[str, dict[str, str|dict[str, str]]]
                       ) -> dict[str, dict[str, str|tuple[str|None, str|None, str|None]]]:
        # In v46.1, tk.xml's Acre metazone merely inherited a generic
        # long name; but root provides no metazone data for it to
        # inherit. So it ended up with {'long': (None, None, None)}
        # for metaZoneNaming['Acre'].
        return {k: v # Discard any keys for which __cleanZone() empties v:
                for k, v in ((k, cls.__cleanZone(v)) for k, v in zoneMap.items())
                if v}

    @staticmethod
    def __cleanZone(data: dict[str, str|dict[str, str]], keys = ('generic', 'standard', 'daylight')
                    ) -> dict[str, str|tuple[str|None, str|None, str|None]]:
        if 'long' in data:
            triad = tuple(data['long'].get(k) for k in keys)
            if any(triad):
                data['long'] = triad
            else:
                del data['long']
        if 'short' in data:
            triad = tuple(data['short'].get(k) for k in keys)
            if any(triad):
                data['short'] = triad
            else:
                del data['short']
        # Leave any other keys alone.
        return data

    def __findUnit(self, keySuffix: str, quantify: str, fallback: str = '') -> str:
        # The displayName for a quantified unit in en.xml is kByte
        # (even for unitLength[narrow]) instead of kB (etc.), so
        # prefer any unitPattern provided, but prune its placeholder:
        for size in ('short', 'narrow'): # TODO: reverse order ?
            stem = f'units/unitLength[{size}{keySuffix}]/unit[digital-{quantify}byte]/'
            for count in ('many', 'few', 'two', 'other', 'zero', 'one'):
                try:
                    ans: str = self.find(f'{stem}unitPattern[count={count}]')
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
                return self.find(f'{stem}displayName')
            except Error:
                pass

        return fallback

    def __unitCount(self, keySuffix: str, suffix: str, cache: list[str],
                    # Stop at exa/exbi: 16 exbi = 2^{64} < zetta =
                    # 1000^7 < zebi = 2^{70}, the next quantifiers up:
                    siQuantifiers: tuple[str, ...] = ('kilo',
                    'mega', 'giga', 'tera', 'peta', 'exa')) -> Iterator[str]:
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
                    suffix = f'i{byte}'
            for q in siQuantifiers:
                # Those don't (yet, v36) exist in CLDR, so we always get the fall-back:
                yield self.__findUnit(keySuffix, q[:2], f'{q[0].upper()}{suffix}')
        else: # first call
            tail = suffix = suffix or 'B'
            for q in siQuantifiers:
                it: str | None = self.__findUnit(keySuffix, q)
                # kB for kilobyte, in contrast with KiB for IEC:
                q = q[0] if q == 'kilo' else q[0].upper()
                if not it:
                    it = q + tail
                elif it.startswith(q):
                    rest = it[1:]
                    tail = rest if all(rest == k for k in cache) else suffix
                    cache.append(rest)
                yield it

    def __numberGrouping(self, system: str) -> tuple[int, int, int]:
        """Sizes of groups of digits within a number.

        Returns a triple (least, higher, fist) for which:
          * least is the number of digits after the last grouping
            separator;
          * higher is the number of digits between grouping
            separators;
          * first is the fewest digits that can appear before the first
            grouping separator.

        Thus (4, 3, 2) would want 1e7 as 1000,0000 but 1e8 as 10,000,0000.

        Note: CLDR does countenance the possibility of grouping also
        in the fractional part.  This is not presently attempted.  Nor
        is placement of the sign character anywhere but at the start
        of the number (some formats may place it at the end, possibly
        elsewhere)."""
        first = int(self.find('numbers/minimumGroupingDigits'))
        assert first < 4, first # We store it in a 2-bit field
        grouping: str | None = self.find(f'numbers/decimalFormats[numberSystem={system}]/'
                             'decimalFormatLength/decimalFormat/pattern')
        groups: list[str] = grouping.split('.')[0].split(',')[-3:]
        assert all(len(x) < 8 for x in groups[-2:]), grouping # we store them in 3-bit fields
        if len(groups) > 2:
            return len(groups[-1]), len(groups[-2]), first

        size = len(groups[-1]) if len(groups) == 2 else 3
        return size, size, first

    @staticmethod
    def __currencyFormats(patterns: str, plus: str, minus: str) -> Iterator[str]:
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
            p = p.replace('\xa4', "%2")
            # Single quote goes away, but double goes to single:
            p = p.replace("''", '###').replace("'", '').replace('###', "'")
            # Use number system's signs:
            p = p.replace('+', plus).replace('-', minus)
            yield p

    @staticmethod
    def __fromLdmlListPattern(pattern: str) -> str:
        # This is a very limited parsing of the format for list pattern part only.
        return pattern.replace('{0}', '%1').replace('{1}', '%2').replace('{2}', '%3')

    @staticmethod
    def __fromLdmlPath(seq: list[str]) -> Iterator[str]: # tool function for __xpathJoin()
        """Convert LDML's [@name='value'] to our [name=value] form."""
        for it in seq:
            # First dismember it:
            attrs: list[str] = it.split('[')
            tag: str = attrs.pop(0)
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
    def __xpathJoin(cls, head: list[str], insert: list[str], tail: list[str]) -> list[str]:
        """Join three lists of XPath selectors.

        Each of head, insert and tail is a sequence of selectors but
        insert may start with some uses of '..', that we want to
        resolve away, and may use LDML's attribute format, that we
        want to convert to our format."""
        while insert and insert[0] == '..':
            insert.pop(0)
            head.pop()
        return head + list(cls.__fromLdmlPath(insert)) + tail
