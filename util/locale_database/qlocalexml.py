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

from typing import Any, Callable, Iterable, Iterator, NoReturn
from xml.sax.saxutils import escape
from xml.dom import minidom

from localetools import Error, qtVersion

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
    def __init__(self, filename: str) -> None:
        self.root: minidom.Element = self.__parse(filename)

        from enumdata import language_map, script_map, territory_map
        # Tuples  of (id, enum name, code, en.xml name) tuples:
        languages = tuple(self.__loadMap('language', language_map))
        scripts = tuple(self.__loadMap('script', script_map))
        territories = tuple(self.__loadMap('territory', territory_map))

        # as enum numeric values, tuple[tuple[int, int, int], tuple[int, int, int]]
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

        self.cldrVersion = self.root.attributes['versionCldr'].nodeValue
        self.qtVersion: str = self.root.attributes['versionQt'].nodeValue
        assert self.qtVersion == qtVersion, (
            'Using QLocaleXml file from incompatible Qt version',
            self.qtVersion, qtVersion
        )

    def loadLocaleMap(self, calendars: Iterable[str], grumble = lambda text: None
                     ) -> Iterator[tuple[tuple[int, int, int], "Locale"]]:
        """Yields id-triplet and locale object for each locale read.

        The id-triplet gives the (language, script, territory) numeric
        values for the QLocale enum members describing the
        locale. Where the relevant enum value is zero (an Any* member
        of the enum), likely subtag rules are used to fill in the
        script or territory, if missing, in this triplet."""
        kid: Callable[[minidom.Element, str], str] = self.__firstChildText
        likely: dict[tuple[int, int, int], tuple[int, int, int]] = dict(self.__likely)
        for elt in self.__eachEltInGroup(self.root, 'localeList', 'locale'):
            locale: Locale = Locale.fromXmlData(lambda k: kid(elt, k), calendars)
            # region is tuple[str|None, ...]
            # zone and meta are dict[str, dict[str, str | tuple[str | None, ...]]]
            region, zone, meta = self.__zoneData(elt)
            locale.update(regionFormats = region,
                          zoneNaming = zone,
                          metaNaming = meta)

            language: int = self.__langByName[locale.language][0]
            script: int = self.__textByName[locale.script][0]
            territory: int = self.__landByName[locale.territory][0]

            if language != 1: # C
                if territory == 0:
                    grumble(f'loadLocaleMap: No territory id for "{locale.language}"\n')

                if script == 0:
                    # Find default script for the given language and territory - see:
                    # http://www.unicode.org/reports/tr35/#Likely_Subtags
                    try:
                        try:
                            to: tuple[int, int, int] = likely[(language, 0, territory)]
                        except KeyError:
                            to = likely[(language, 0, 0)]
                    except KeyError:
                        pass
                    else:
                        script = to[1]
                        locale.script = self.scripts[script][0]

            yield (language, script, territory), locale

    def pruneZoneNaming(self, locmap: dict[tuple[int, int, int], "Locale"],
                        report=lambda *x: 0) -> None:
        """Deduplicate zoneNaming and metaNaming mapings.

        Where one locale would fall back to another via likely subtag
        fallbacks, skip any entries in the former's zoneNaming and metaNaming
        where it agrees with the latter.

        This prunes over half of the (locale, zone) table and nearly two
        thirds of the (locale, meta) table."""
        likely: tuple[tuple[tuple[int, int, int], tuple[int, int, int]], ...
                     ] = tuple((has, got) for have, has, give, got in self.likelyMap())
        def fallbacks(key) -> Iterator[Locale]:
            # Should match QtTimeZoneLocale::fallbackLocalesFor() in qlocale.cpp
            tried: set[tuple[int, int, int]] = { key }
            head =  2
            while head > 0:
                # Retain [:head] of key but use 0 (i.e. Any) for the rest:
                it: tuple[int, int, int] = self.__fillLikely(key[:head] + (0,) * (3 - head), likely)
                if it not in tried:
                    tried.add(it)
                    if it in locmap:
                        yield locmap[it]
                head -= 1

        # TODO: fix case of later fallbacks lacking a short name for a
        # metazone, where earlier ones with a short name all agree.  Maybe do
        # similar for long names, and for zones as well as meta.
        # For a metazone, the territories in its map to IANA ID, combined with
        # the language and script of a locale that lacks names, give locales to
        # consult that might fall back to it, and to which to pay particular
        # attention.

        zonePrior = metaPrior = 0
        zoneCount = metaCount = locCount = 0
        for key, loc in locmap.items():
            zonePrior += len(loc.zoneNaming)
            metaPrior += len(loc.metaNaming)
            # Omit zoneNaming and metaNaming entries that match those
            # of their likely sub-tag fallbacks.
            filtered = False
            for alt in fallbacks(key):
                filtered = True
                # Collect keys to purge before purging, so as not to
                # modify mappings while iterating them.
                purge = [zone for zone, data in loc.zoneNaming.items()
                         if (zone in alt.zoneNaming
                             and data == alt.zoneNaming[zone])]
                zoneCount += len(purge)
                for zone in purge:
                    del loc.zoneNaming[zone]

                purge = [meta for meta, data in loc.metaNaming.items()
                         if (meta in alt.metaNaming
                             and data == alt.metaNaming[meta])]
                metaCount += len(purge)
                for meta in purge:
                    del loc.metaNaming[meta]
            if filtered:
                locCount += 1

        report(f'Pruned duplicates: {zoneCount} (of {zonePrior}) zone '
               f'and {metaCount} (of {metaPrior}) metazone '
               f'entries from {locCount} (of {len(locmap)}) locales.\n')

    def aliasToIana(self) -> Iterator[tuple[str, str]]:
        def attr(elt: minidom.Element, key: str) -> str:
            return elt.attributes[key].nodeValue
        for elt in self.__eachEltInGroup(self.root, 'zoneAliases', 'zoneAlias'):
            yield attr(elt, 'alias'), attr(elt, 'iana')

    def msToIana(self) -> Iterator[tuple[str, str]]:
        kid: Callable[[minidom.Element, str], str] = self.__firstChildText
        for elt in self.__eachEltInGroup(self.root, 'windowsZone', 'msZoneIana'):
            yield kid(elt, 'msid'), elt.attributes['iana'].nodeValue

    def msLandIanas(self) -> Iterator[tuple[str, str, str]]:
        kid: Callable[[minidom.Element, str], str] = self.__firstChildText
        for elt in self.__eachEltInGroup(self.root, 'windowsZone', 'msLandZones'):
            land: str = elt.attributes['territory'].nodeValue
            yield kid(elt, 'msid'), land, kid(elt, 'ianaids')

    def territoryZone(self) -> Iterator[tuple[str, str]]:
        for elt in self.__eachEltInGroup(self.root, 'landZones', 'landZone'):
            iana, land = self.__textThenAttrs(elt, 'territory')
            yield land, iana

    def metaLandZone(self) -> Iterator[tuple[str, int, str, str]]:
        kid: Callable[[minidom.Element, str], str] = self.__firstChildText
        for elt in self.__eachEltInGroup(self.root, 'metaZones', 'metaZone'):
            meta: str = kid(elt, 'metaname')
            mkey: int = int(elt.attributes['metakey'].nodeValue)
            node: minidom.Element = self.__firstChildElt(elt, 'landZone')
            while node:
                if self.__isNodeNamed(node, 'landZone'):
                    iana, land = self.__textThenAttrs(node, 'territory')
                    yield meta, mkey, land, iana
                node = node.nextSibling

    def zoneMetaStory(self) -> Iterator[tuple[str, int, int, int]]:
        kid = lambda n, k: int(n.attributes[k].nodeValue)
        for elt in self.__eachEltInGroup(self.root, 'zoneStories', 'zoneStory'):
            iana: str = elt.attributes['iana'].nodeValue
            node: minidom.Element = self.__firstChildElt(elt, 'metaInterval')
            while node:
                if self.__isNodeNamed(node, 'metaInterval'):
                    meta: int = kid(node, 'metakey')
                    yield iana, kid(node, 'start'), kid(node, 'stop'), meta
                node = node.nextSibling

    def languageIndices(self, locales: tuple[int, ...]) -> Iterator[tuple[int, str]]:
        index = 0
        for key, value in self.languages.items():
            i, count = 0, locales.count(key)
            if count > 0:
                i = index
                index += count
            yield i, value[0]

    def likelyMap(self) -> Iterator[tuple[str, tuple[int, int, int], str, tuple[int, int, int]]]:
        def tag(t: tuple[tuple[int, str], tuple[int, str], tuple[int, str]]) -> Iterator[str]:
            lang, script, land = t
            yield lang[1] if lang[0] else 'und'
            if script[0]: yield script[1]
            if land[0]: yield land[1]

        def ids(t: tuple[tuple[int, str], tuple[int, str], tuple[int, str]]
                ) -> tuple[int, int, int]:
            return tuple(x[0] for x in t)

        def keyLikely(pair: tuple[tuple[tuple[int, str], tuple[int, str], tuple[int, str]],
                                  tuple[tuple[int, str], tuple[int, str], tuple[int, str]]],
                      kl=self.__keyLikely) -> tuple[int, int, int]:
            """Sort by IDs from first entry in pair

            We're passed a pair (h, g) of triplets (lang, script, territory) of
            pairs (ID, name); we extract the ID from each entry in the first
            triplet, then hand that triplet of IDs off to __keyLikely()."""
            return kl(tuple(x[0] for x in pair[0]))

        # Sort self.__likely to enable binary search in C++ code.
        for have, give in sorted(((self.__fromIds(has),
                                   self.__fromIds(got))
                                  for has, got in self.__likely),
                                 key = keyLikely):
            try:
                yield ('_'.join(tag(have)), ids(have),
                       '_'.join(tag(give)), ids(give))
            except TypeError as what:
                what.args += (have, give)
                raise

    def defaultMap(self) -> Iterator[tuple[tuple[int, int], int]]:
        """Map language and script to their default territory by ID.

        Yields ((language, script), territory) wherever the likely
        sub-tags mapping says language's default locale uses the given
        script and territory."""
        for have, give in self.__likely:
            if have[0] and have[1:] == (0, 0) and give[2]:
                assert have[0] == give[0], (have, give)
                yield (give[:2], give[2])

    def enumify(self, name: str, suffix: str) -> str:
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
    def __loadMap(self, category: str, enum: dict[int, tuple[str, str]]
                 ) -> Iterator[tuple[int, str, str, str]]:
        """Load the language-, script- or territory-map.

        First parameter, category, names the map to load, second is the
        enumdata.py map that corresponds to it.  Yields 4-tuples (id, enum,
        code, name) where id and enum are the enumdata numeric index and name
        (on which the QLocale enums are based), code is the ISO code and name
        is CLDR's en.xml name for the language, script or territory."""
        for element in self.__eachEltInGroup(self.root, f'{category}List', 'naming'):
            name, key, code = self.__textThenAttrs(element, 'id', 'code')
            key = int(key)
            yield key, enum[key][0], code, name

    def __fromIds(self, ids: tuple[int, int, int]
                  ) -> tuple[tuple[int, str], tuple[int, str], tuple[int, str]]:
        # Three (ID, code) pairs:
        return ((ids[0], self.languages[ids[0]][1]),
                (ids[1], self.scripts[ids[1]][1]),
                (ids[2], self.territories[ids[2]][1]))

    # Likely subtag management:
    def __likelySubtagsMap(self) -> Iterator[tuple[tuple[int, int, int], tuple[int, int, int]]]:
        def triplet(element: minidom.Element,
                    keys: tuple[str, str, str]=('language', 'script', 'territory')
                    ) -> tuple[int, int, int]:
            return tuple(int(element.attributes[key].nodeValue) for key in keys)

        kid: Callable[[minidom.Element, str], minidom.Element] = self.__firstChildElt
        for elt in self.__eachEltInGroup(self.root, 'likelySubtags', 'likelySubtag'):
            yield triplet(kid(elt, "from")), triplet(kid(elt, "to"))

    @staticmethod
    def __keyLikely(key: tuple[int, int, int], huge: int=0x10000) -> tuple[int, int, int]:
        """Sort order key for a likely subtag key

        Although the entries are (lang, script, region), sort by (lang, region,
        script) and sort 0 after all non-zero values, in each position. This
        ensures that, when several mappings partially match a requested locale,
        the one we should prefer to use appears first.

        We use 0x10000 as replacement for 0, as all IDs are unsigned short, so
        less than 2^16."""
        # Map zero to huge:
        have = tuple(x or huge for x in key)
        # Use language, territory, script for sort order:
        return have[0], have[2], have[1]

    @classmethod
    def __lowerLikely(cls, key: tuple[int, int, int],
                      likely: tuple[tuple[tuple[int, int, int], tuple[int, int, int]], ...]
                     ) -> int:
        """Lower-bound index for key in the likely subtag table

        Equivalent to the std::lower_bound() calls in
        QLocaleId::withLikelySubtagsAdded()."""
        lo, hi = 0, len(likely)
        key: tuple[int, int, int] = cls.__keyLikely(key)
        while lo + 1 < hi:
            mid, rem = divmod(lo + hi, 2)
            has: tuple[int, int, int] = cls.__keyLikely(likely[mid][0])
            if has < key:
                lo = mid
            elif has > key:
                hi = mid
            else:
                return mid
        return hi

    @classmethod
    def __fillLikely(cls, key: tuple[int, int, int],
                     likely: tuple[tuple[tuple[int, int, int], tuple[int, int, int]], ...]
                     ) -> tuple[int, int, int]:
        """Equivalent to QLocaleId::withLikelySubtagsAdded()

        Takes one (language, script, territory) triple, key, of QLocale enum
        numeric values and returns another that fills in any zero entries based
        on the likely subtag data supplied as likely."""
        lang, script, land = key
        if lang and likely:
            likely: tuple[tuple[tuple[int, int, int], tuple[int, int, int]], ...
                          ] = likely[cls.__lowerLikely(key, likely):]
            for entry in likely:
                vox, txt, ter = entry[0]
                if vox != lang:
                    break
                if land and ter != land:
                    continue
                if script and txt != script:
                    continue

                vox, txt, ter = entry[1]
                return vox, txt or script, ter or land

        if land and likely:
            likely = likely[cls.__lowerLikely((0, script, land), likely):]
            for entry in likely:
                vox, txt, ter = entry[0]
                assert not vox, (key, entry)
                if ter != land:
                    break
                if txt != script:
                    continue

                vox, txt, ter = entry[1]
                return lang or vox, txt or script, ter

        if script and likely:
            likely = likely[cls.__lowerLikely((0, script, 0), likely):]
            for entry in likely:
                vox, txt, ter = entry[0]
                assert not (vox or ter), (key, entry)
                if txt != script:
                    break

                vox, txt, ter = entry[1]
                return lang or vox, txt, land or ter

        if not any(key) and likely:
            likely = likely[cls.__lowerLikely(key, likely):]
            if likely:
                assert len(likely) == 1
                assert likely[0][0] == key
                return likely[0][1]

        return key

    # DOM access:
    from xml.dom import minidom
    @staticmethod
    def __parse(filename: str, read = minidom.parse) -> minidom.Element:
        return read(filename).documentElement

    @staticmethod
    def __isNodeNamed(elt: minidom.Element|minidom.Text, name: str,
                      TYPE: int = minidom.Node.ELEMENT_NODE) -> bool:
        return elt.nodeType == TYPE and elt.nodeName == name
    del minidom

    @staticmethod
    def __eltWords(elt: minidom.Element) -> Iterator[str]:
        child: minidom.Text|minidom.CDATASection|None = elt.firstChild
        while child:
            if child.nodeType in (elt.TEXT_NODE, elt.CDATA_SECTION_NODE):
                # Note: do not strip(), as some group separators are
                # (non-breaking) spaces, that strip() will discard.
                yield child.nodeValue
            child = child.nextSibling

    @classmethod
    def __firstChildElt(cls, parent: minidom.Element, name: str) -> minidom.Element:
        child: minidom.Text|minidom.Element = parent.firstChild
        while child:
            if cls.__isNodeNamed(child, name):
                return child
            child = child.nextSibling

        raise Error(f'No {name} child found')

    @classmethod
    def __firstChildText(cls, elt: minidom.Element, key: str) -> str:
        return ' '.join(cls.__eltWords(cls.__firstChildElt(elt, key)))

    @classmethod
    def __textThenAttrs(cls, elt: minidom.Element, *names: str) -> Iterator[str]:
        """Read an elements text than a sequence of its attributes.

        First parameter is the XML element, subsequent parameters name
        attributes of it. Yields the text of the element, followed by the text
        of each of the attributes in turn."""
        yield ' '.join(cls.__eltWords(elt))
        for name in names:
            yield elt.attributes[name].nodeValue

    @classmethod
    def __zoneData(cls, elt: minidom.Element
                   ) -> tuple[tuple[str | None, ...],
                              dict[str, dict[str, str | tuple[str | None, ...]]],
                              dict[str, dict[str, str | tuple[str | None, ...]]]]:
        # Inverse of writer's __writeLocaleZones()
        region: tuple[str | None, ...] = cls.__readZoneForms(elt, 'regionZoneFormats')
        try:
            zone: minidom.Element = cls.__firstChildElt(elt, 'zoneNaming')
        except Error as what:
            if what.message != 'No zoneNaming child found':
                raise
            zone: dict[str, dict[str, str|tuple[str|None, ...]]] = {}
        else:
            zone = dict(cls.__readZoneNaming(zone))
        try:
            meta: minidom.Element = cls.__firstChildElt(elt, 'metaZoneNaming')
        except Error as what:
            if what.message != 'No metaZoneNaming child found':
                raise
            meta: dict[str, dict[str, str|tuple[str|None, ...]]] = {}
        else:
            meta = dict(cls.__readZoneNaming(meta))
            assert not any('exemplarCity' in v for v in meta.values())
        return region, zone, meta

    @classmethod
    def __readZoneNaming(cls, elt: minidom.Element
                         ) -> Iterator[tuple[str, dict[str, str|tuple[str|None, ...]]]]:
        # Inverse of writer's __writeZoneNaming()
        child: minidom.Element = elt.firstChild
        while child:
            if cls.__isNodeNamed(child, 'zoneNames'):
                iana: str = child.attributes['name'].nodeValue
                try:
                    city: str = cls.__firstChildText(child, 'exemplar')
                except Error:
                    data: dict[str, tuple[str|None, str|None, str|None]] = {}
                else:
                    assert city is not None
                    data: dict[str, str|tuple[str|None, ...]] = { 'exemplarCity': city }
                for form in ('short', 'long'):
                    data[form] = cls.__readZoneForms(child, form)
                yield iana, data

            child = child.nextSibling

    @classmethod
    def __readZoneForms(cls, elt: minidom.Element, name: str
                        ) -> tuple[str | None, ...]:
        # Inverse of writer's __writeZoneForms()
        child: minidom.Element = elt.firstChild
        while child:
            if (cls.__isNodeNamed(child, 'zoneForms')
                and child.attributes['name'].nodeValue == name):
                return tuple(cls.__scanZoneForms(child))
            child = child.nextSibling
        return (None, None, None)

    @classmethod
    def __scanZoneForms(cls, elt: minidom.Element) -> Iterator[str|None]:
        # Read each entry in a zoneForms element, yield three forms:
        for tag in ('generic', 'standard', 'daylightSaving'):
            try:
                node: minidom.Element = cls.__firstChildElt(elt, tag)
            except Error:
                yield None
            else:
                yield ' '.join(cls.__eltWords(node))

    @classmethod
    def __eachEltInGroup(cls, parent: minidom.Element, group: str, key: str
                         ) -> Iterator[minidom.Element]:
        try:
            element: minidom.Element = cls.__firstChildElt(parent, group).firstChild
        except Error:
            element = None

        while element:
            if cls.__isNodeNamed(element, key):
                yield element
            element = element.nextSibling


class Spacer (object):
    def __init__(self, indent:str|int|None = None, initial: str = '') -> None:
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
            self.__call: Callable[[str], str] = lambda x: x
        else:
            self.__each: str = ' ' * indent if isinstance(indent, int) else indent
            self.current = initial
            self.__call = self.__wrap

    def __wrap(self, line: str) -> str:
        if not line:
            return '\n'

        indent: str = self.current
        if line.startswith('</'):
            indent = self.current = indent[:-len(self.__each)]
        elif line.startswith('<') and line[1:2] not in '!?':
            cut = line.find('>')
            if cut < 1 or line[cut - 1] != '/':
                tag = (line[1:] if cut < 0 else line[1 : cut]).strip().split()[0]
                if f'</{tag}>' not in line:
                    self.current += self.__each
        return indent + line + '\n'

    def __call__(self, line: str) -> str:
        return self.__call(line)

class QLocaleXmlWriter (object):
    """Save the full set of locale data to a QLocaleXML file.

    The output saved by this should conform to qlocalexml.rnc's
    schema."""
    def __init__(self, cldrVersion: str, save: Callable[[str], int]|None = None,
                 space: Spacer = Spacer('\t')) -> None:
        """Set up to write digested CLDR data as QLocale XML.

        First argument is the version of CLDR whose data we'll be
        writing. Other arguments are optional.

        Second argument, save, is None (its default) or a callable that will
        write content to where you intend to save it. If None, it is replaced
        with a callable that prints the given content, suppressing the newline
        (but see the following); this is equivalent to passing
        sys.stdout.write.

        Third argument, space, is an object to call on each text output to
        prepend indentation and append newlines, or not as the case may be. The
        default is a Spacer('\t'), which grows indent by a tab after each
        unmatched new tag and shrinks back on a close-tag (its parsing is
        naive, but adequate to how this class uses it), while adding a newline
        to each line."""
        self.__rawOutput: Callable[[str], int] = self.__printit if save is None else save
        self.__wrap = space
        self.__write('<?xml version="1.0" encoding="UTF-8" ?>'
                     # A hint to emacs to make display nicer:
                     '<!--*- tab-width: 4 -*-->')
        self.__openTag('localeDatabase', versionCldr = cldrVersion,
                       versionQt = qtVersion)

    # Output of various sections, in their usual order:
    def enumData(self, code2name: Callable[[str], Callable[[str, str], str]]) -> None:
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
        self.__languages: set[str] = set(p[1] for p in language_map.values()
                                         if not p[1].isspace())
        self.__scripts: set[str] = set(p[1] for p in script_map.values()
                                       if p[1] != 'Zzzz')
        self.__territories: set[str] = set(p[1] for p in territory_map.values()
                                           if p[1] != 'ZZ')

    def likelySubTags(self, entries: Iterator[tuple[tuple[int, int, int, int],
                                                    tuple[int, int, int, int]]]) -> None:
        self.__openTag('likelySubtags')
        for have, give in entries:
            self.__openTag('likelySubtag')
            self.__likelySubTag('from', have)
            self.__likelySubTag('to', give)
            self.__closeTag('likelySubtag')
        self.__closeTag('likelySubtags')

    def zoneData(self, alias: dict[str, str],
                 defaults: dict[str, str],
                 windowsIds: dict[tuple[str, str], str],
                 metamap: dict[str, dict[str, str]],
                 zones: dict[str, tuple[tuple[int, int, str], ...]],
                 territorial: dict[str, str]) -> None:
        self.__openTag('zoneAliases')
        # iana is a single IANA ID
        # name has the same form, but has been made redundant
        # Do case-insensitive sorting, to match how lookup is done:
        for name, iana in sorted(alias.items(), key = lambda s: (s[0].lower(), s[1])):
            if name == iana:
                continue
            self.asTag('zoneAlias', alias = name, iana = iana)
        self.__closeTag('zoneAliases')

        self.__openTag('windowsZone')
        for (msid, code), ids in windowsIds.items():
            # ianaids is a space-joined sequence of IANA IDs
            self.__openTag('msLandZones', territory = code)
            self.inTag('msid', msid)
            self.inTag('ianaids', ids)
            self.__closeTag('msLandZones')

        for winid, iana in defaults.items():
            self.__openTag('msZoneIana', iana=iana)
            self.inTag('msid', winid)
            self.__closeTag('msZoneIana')
        self.__closeTag('windowsZone')

        self.__openTag('landZones')
        for code, iana in territorial.items():
            self.inTag('landZone', iana, territory = code)
        self.__closeTag('landZones')

        metaKey: dict[str, int] = {m: i for i, m in enumerate(sorted(
            metamap, key = lambda m: m.lower()), 1)}

        self.__openTag('metaZones')
        for meta, bok in metamap.items():
            self.__openTag('metaZone', metakey = metaKey[meta])
            self.inTag('metaname', meta)
            for code, iana in bok.items():
                self.inTag('landZone', iana, territory = code)
            self.__closeTag('metaZone')
        self.__closeTag('metaZones')

        self.__openTag('zoneStories')
        for iana, story in sorted(zones.items()):
            self.__openTag('zoneStory', iana = iana)
            for start, stop, meta in story:
                self.asTag('metaInterval', start = start,
                           stop = stop, metakey = metaKey[meta])
            self.__closeTag('zoneStory')
        self.__closeTag('zoneStories')

    def locales(self, locales: dict[tuple[int, int, int, int], "Locale"], calendars: list[str],
                en_US: tuple[int, int, int, int]) -> None:
        """Write the data for each locale.

        First argument, locales, is the mapping whose values are the
        Locale objects, with each key being the matching tuple of
        numeric IDs for language, script, territory and variant.
        Second argument is a tuple of calendar names. Third is the
        tuple of numeric IDs that corresponds to en_US (needed to
        provide fallbacks for the C locale)."""

        def writeLocale(locale: "Locale", cal = calendars, this = self) -> None:
            this.__openTag('locale')
            this.__writeLocale(locale, cal)
            this.__writeLocaleZones(locale)
            this.__closeTag('locale')

        self.__openTag('localeList')
        writeLocale(Locale.C(locales[en_US]))
        for key in sorted(locales.keys()):
            writeLocale(locales[key])
        self.__closeTag('localeList')

    def inTag(self, tag: str, text: str, **attrs: int|str) -> None:
        """Writes an XML element with the given content.

        First parameter, tag, is the element type; second, text, is the content
        of its body, which must be XML-safe (see safeInTag() for when that's
        not assured). Any keyword parameters passed specify attributes to
        include in the opening tag."""
        self.__write(f'<{self.__attrJoin(tag, attrs)}>{text}</{tag}>')

    def asTag(self, tag: str, **attrs: int|str) -> None:
        """Similar to inTag(), but with no content for the element."""
        assert attrs, tag # No point to this otherwise
        self.__write(f'<{self.__attrJoin(tag, attrs)} />')

    def safeInTag(self, tag: str, text: str, **attrs: int|str) -> None:
        """Similar to inTag(), when text isn't known to be XML-safe."""
        if text.isascii():
            self.inTag(tag, self.__xmlSafe(text), **attrs)
        else:
            self.__cdataInTag(tag, text, **attrs)

    def close(self, grumble: Callable[[str], int]) -> None:
        """Finish writing and grumble about any issues discovered."""
        if self.__rawOutput != self.__complain:
            self.__closeTag('localeDatabase')
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
    def __printit(text: str) -> int:
        print(text, end='')
        return 0

    @staticmethod
    def __complain(text) -> NoReturn:
        raise Error('Attempted to write data after closing :-(')

    @staticmethod
    def __attrJoin(tag: str, attrs: dict[str, int|str]) -> str:
        # Content of open-tag with given tag and attributes
        if not attrs:
            return tag
        tail = ' '.join(f'{k}="{v}"' for k, v in attrs.items())
        return f'{tag} {tail}'

    @staticmethod
    def __xmlSafe(text: str) -> str:
        return text.replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;')

    def __cdataInTag(self, tag: str, text: str, **attrs: int|str) -> None:
        self.__write(f'<{self.__attrJoin(tag, attrs)}><![CDATA[{text}]]></{tag}>')

    def __enumTable(self, tag: str, table: dict[int, tuple[str, str]],
                    code2name: Callable[[str], Callable[[str, str], str]]) -> None:
        """Writes a table of QLocale-enum-related data.

        First parameter, tag, is 'language', 'script' or 'territory',
        identifying the relevant table. Second, table, is the enumdata.py
        mapping from numeric enum value to (enum name, ISO code) pairs for that
        type. Last is the englishNaming method of the CldrAccess being used to
        read CLDR data; it is used to map ISO codes to en.xml names."""
        self.__openTag(f'{tag}List')
        enname: Callable[[str, str], str] = code2name(tag)
        for key, (name, code) in table.items():
            self.safeInTag('naming', enname(code, name), id = key, code = code)
        self.__closeTag(f'{tag}List')

    def __likelySubTag(self, tag: str, likely: tuple[int, int, int, int]) -> None:
        self.asTag(tag, language = likely[0], script = likely[1],
                   territory = likely[2]) # variant = likely[3]

    def __writeLocale(self, locale: "Locale", calendars: list[str]) -> None:
        locale.toXml(self.inTag, calendars)
        self.__languages.discard(locale.language_code)
        self.__scripts.discard(locale.script_code)
        self.__territories.discard(locale.territory_code)

    def __writeLocaleZones(self, locale: "Locale") -> None:
        self.__writeZoneForms('regionZoneFormats', locale.regionZoneFormats)
        self.__writeZoneNaming('zoneNaming', locale.zoneNaming)
        self.__writeZoneNaming('metaZoneNaming', locale.metaZoneNaming)

    def __writeZoneNaming(self, group: str,
                          naming: dict[str, dict[str, str|tuple[str|None, str|None, str|None]]]
                          ) -> None:
        if not naming:
            return
        self.__openTag(group)
        for iana in sorted(naming.keys()):  # str
            data: dict[str, str|tuple[str|None, str|None, str|None]] = naming[iana]
            self.__openTag('zoneNames', name=iana)
            if 'exemplarCity' in data:
                self.safeInTag('exemplar', data['exemplarCity'])
            for form in ('short', 'long'):
                if form in data:
                    self.__writeZoneForms(form, data[form])
            self.__closeTag('zoneNames')
        self.__closeTag(group)

    def __writeZoneForms(self, group: str, forms: tuple[str|None, str|None, str|None]) -> None:
        if all(x is None for x in forms):
            return
        self.__openTag('zoneForms', name=group)
        for i, tag in enumerate(('generic', 'standard', 'daylightSaving')):
            if forms[i]:
                self.safeInTag(tag, forms[i])
        self.__closeTag('zoneForms')

    def __openTag(self, tag: str, **attrs: int|str) -> None:
        self.__write(f'<{self.__attrJoin(tag, attrs)}>')

    def __closeTag(self, tag: str) -> None:
        self.__write(f'</{tag}>')

    def __write(self, line: str) -> None:
        self.__rawOutput(self.__wrap(line))

class Locale (object):
    """Holder for the assorted data representing one locale.

    Implemented as a namespace; its constructor and update() have the
    same signatures as those of a dict, acting on the instance's
    __dict__, so the results are accessed as attributes rather than
    mapping keys."""
    def __init__(self, data: dict[str, Any]|None = None, **kw: Any) -> None:
        self.update(data, **kw)

    def update(self, data: dict[str, Any]|None = None, **kw: Any) -> None:
        if data: self.__dict__.update(data)
        if kw: self.__dict__.update(kw)

    def __len__(self) -> int: # Used when testing as a boolean
        return len(self.__dict__)

    @staticmethod
    def propsMonthDay(scale: str, lengths: tuple[str, str, str] = ('long', 'short', 'narrow')
                      ) -> Iterator[str]:
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
               # Formats, previously processed on reading from LDML:
               'positiveOffsetFormat', 'negativeOffsetFormat',
               'gmtOffsetFormat', 'fallbackZoneFormat',
               )
    # Require special handling:
    # 'regionZoneFormats', 'zoneNaming', 'metaZoneNaming'

    # Day-of-Week numbering used by Qt:
    __qDoW = {"mon": 1, "tue": 2, "wed": 3, "thu": 4, "fri": 5, "sat": 6, "sun": 7}

    @classmethod
    def fromXmlData(cls, lookup: Callable[[str], str], calendars: Iterable[str]=('gregorian',)
                    ) -> "Locale":
        """Constructor from the contents of XML elements.

        First parameter, lookup, is called with the names of XML elements that
        should contain the relevant data, within a QLocaleXML locale element
        (within a localeList element); these names mostly match the attributes
        of the object constructed. Its return must be the full text of the
        first child DOM node element with the given name. Attribute values are
        obtained by suitably digesting the returned element texts.

        Optional second parameter, calendars, is a sequence of calendars for
        which data is to be retrieved."""
        data: dict[str, int|str|dict[str, str]] = {}
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

    # NOTE: any change to the XML must be reflected in qlocalexml.rnc
    def toXml(self, write: Callable[[str, str], None], calendars: Iterable[str]=('gregorian',)
              ) -> None:
        """Writes its data as QLocale XML.

        First argument, write, is a callable taking the name and
        content of an XML element; it is expected to be the inTag
        bound method of a QLocaleXmlWriter instance.

        Optional second argument is a list of calendar names, in the
        form used by CLDR; its default is ('gregorian',).
        """
        get: Callable[[str], str | Iterable[int]] = lambda k: getattr(self, k)
        for key in ('language', 'script', 'territory',
                    'decimal', 'group', 'zero', 'list',
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
                    'positiveOffsetFormat', 'negativeOffsetFormat',
                    'gmtOffsetFormat', 'fallbackZoneFormat',
                    ) + tuple(self.propsMonthDay('days')) + tuple(
                '_'.join((k, cal))
                for k in self.propsMonthDay('months')
                for cal in calendars):
            write(key, escape(get(key)))

        # The regionZoneFormats, zoneNaming and metaZoneNaming members
        # are handled by QLocaleXmlWriter.__writeLocaleZones(). Their
        # elements hold sub-elements.

        write('groupSizes', ';'.join(str(x) for x in get('groupSizes')))
        for key in ('currencyDigits', 'currencyRounding'):
            write(key, get(key))

    @classmethod
    def C(cls, en_US: "Locale") -> "Locale":  # return type should be Self from Python 3.11
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
