# Copyright (C) 2021 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
"""Digesting the CLDR's data.

Provides two classes:
  CldrReader -- driver for reading CLDR data
  CldrAccess -- used by the reader to access the tree of data files

The former should normally be all you need to access.
See individual classes for further detail.
"""

from typing import Callable, Iterable, Iterator, TextIO
from xml.dom import minidom
from weakref import WeakValueDictionary as CacheDict
from pathlib import Path
from datetime import datetime, timedelta

from ldml import Error, Node, XmlScanner, Supplement, LocaleScanner
from localetools import names_clash
from qlocalexml import Locale

class CldrReader (object):
    def __init__(self, root: Path, grumble: Callable[[str], int] = lambda msg: 0,
                 whitter: Callable[[str], int] = lambda msg: 0) -> None:
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
        self.root.checkEnumData(grumble)
        # TODO: can we do anything but ignore with the namings here ?
        self.__bcp47Alias, _ = self.root.bcp47Aliases()

    def likelySubTags(self) -> Iterator[tuple[tuple[int, int, int, int],
                                              tuple[int, int, int, int]]]:
        """Generator for likely subtag information.

        Yields pairs (have, give) of 4-tuples; if what you have
        matches the left member, giving the right member is probably
        sensible. Each 4-tuple's entries are the full names of a
        language, a script, a territory (usually a country) and a
        variant (currently ignored)."""
        skips = []
        for got, use in self.root.likelySubTags():
            try:
                have: tuple[int, int, int, int] = self.__parseTags(got)
                give: tuple[int, int, int, int] = self.__parseTags(use)
            except Error as e:
                if ((use.startswith(got) or got.startswith('und_'))
                    and e.message.startswith('Unknown ') and ' code ' in e.message):
                    skips.append(use)
                else:
                    self.grumble(f'Skipping likelySubtag "{got}" -> "{use}" ({e})\n')
                continue

            give = (give[0],
                    # Substitute according to http://www.unicode.org/reports/tr35/#Likely_Subtags
                    give[1] or have[1],
                    give[2] or have[2],
                    give[3] or have[3])

            yield have, give

        if skips:
            # TODO: look at LDML's reserved locale tag names; they
            # show up a lot in this, and may be grounds for filtering
            # more out.
            pass # self.__wrapped(self.whitter, 'Skipping likelySubtags (for unknown codes): ', skips)

    def zoneData(self) -> tuple[dict[str, str],
                                dict[str, str],
                                dict[tuple[str, str], str],
                                dict[str, dict[str, str]],
                                dict[str, tuple[tuple[int, int, str], ...]],
                                dict[str, str]]:
        """Locale-independent timezone data.

        Returns a tuple (alias, defaults, winIds, metamap, zones,
        territorial) in which:

          * alias is a mapping from aliases for IANA zone IDs, that
            have the form of IANA IDs, to actual current IANA IDs; in
            particular, this maps each CLDR zone ID to its
            corresponding IANA ID.
          * defaults maps each Windows name for a zone to the IANA ID
            to use for it by default (when no territory is specified,
            or when no entry in winIds matches the given Windows name
            and territory).
          * winIds is a mapping {(winId, land): ianaList} from Windows
            name and territory code to the space-joined list of IANA
            IDs associated with the Windows name in the given
            territory.
          * metamap maps each metazone name to a mapping territory code to
            (single) IANA ID.
          * zones maps each IANA ID to its history of metazone association, in
            th form of a tuple of triples (from, to, meta), where each of from
            and to is a quint32 epoch minute and meta is a metazone name.
          * territorial maps territory codes to IANA IDs (only a very small
            minority of territories are represented).

        and reports on any territories found in CLDR timezone data
        that are not mentioned in enumdata.territory_map, on any
        Windows IDs given in zonedata.windowsIdList that are no longer
        covered by the CLDR data."""
        alias: dict[str, str] = self.__bcp47Alias
        # defaults is a dict[str, str] and winIds is a list[tuple[str, str, str]]
        defaults, winIds = self.root.readWindowsTimeZones(alias)
        # metamap is a dict[str, dict[str, str]],
        # zones is dict[str, tuple[tuple[int, int, str], ...]], territorial is a dict[str, str]
        metamap, zones, territorial = self.root.readMetaZoneMap(alias)

        from zonedata import windowsIdList
        winUnused: set[str] = set(n for n, o in windowsIdList).difference(
            set(defaults).union(w for w, t, ids in winIds))
        if winUnused:
            joined = "\n\t".join(winUnused)
            self.whitter.write(
                f'No Windows ID in\n\t{joined}\nis still in use.\n'
                'They could be removed at the next major version.\n')

        # Check for duplicate entries in winIds:
        last: tuple[str, str, str] = ('', '', '')
        winDup: dict[tuple[str, str], list[str]] = {}
        for triple in sorted(winIds):
            if triple[:2] == last[:2]:
                winDup.setdefault(triple[:2], []).append(triple[-1])
            last = triple
        if winDup:
            joined = '\n\t'.join(f'{t}, {w}: ", ".join(ids)'
                                 for (w, t), ids in winDup.items())
            self.whitter.write(
                f'Duplicated (territory, Windows ID) entries:\n\t{joined}\n')
            winIds = [trip for trip in winIds if trip[:2] not in winDup]
            for (w, t), seq in winDup.items():
                ianaList = []
                for ids in seq:
                    for iana in ids.split():
                        if iana not in ianaList:
                            ianaList.append(iana)
                winIds.append((w, t, ' '.join(ianaList)))

        from enumdata import territory_map
        unLand: set[str] = set(t for w, t, ids in winIds).union(territorial)
        for bok in metamap.values():
            unLand = unLand.union(bok)
        unLand = unLand.difference(v[1] for k, v in territory_map.items())
        if unLand:
            self.grumble.write(
                'Unknown territory codes in timezone data: '
                f'{", ".join(unLand)}\n'
                'Skipping Windows zone mappings for these territories\n')
            winIds = [(w, t, ids) for w, t, ids in winIds if t not in unLand]

        # Convert list of triples to mapping:
        winIds: dict[tuple[str, str], str] = {(w, t): ids for w, t, ids in winIds}

        return alias, defaults, winIds, metamap, zones, territorial

    def readLocales(self, calendars: Iterable[str] = ('gregorian',)
                    ) -> dict[tuple[int, int, int, int], Locale]:
        return {(k.language_id, k.script_id, k.territory_id, k.variant_id): k
                for k in self.__allLocales(calendars)}

    def __allLocales(self, calendars: list[str]) -> Iterator[Locale]:
        def skip(locale: str, reason: str) -> str:
            return f'Skipping defaultContent locale "{locale}" ({reason})\n'

        for locale in self.root.defaultContentLocales:
            try:
                language, script, territory, variant = self.__splitLocale(locale)
            except ValueError:
                self.whitter(skip(locale, 'only language tag'))
                continue

            if not (script or territory):
                self.grumble(skip(locale, 'second tag is neither script nor territory'))
                continue

            if not (language and territory):
                continue

            try:
                yield self.__getLocaleData(self.root.locale(locale), calendars,
                                           language, script, territory, variant)
            except Error as e:
                self.grumble(skip(locale, e.message))

        for locale in self.root.fileLocales:
            try:
                chain = self.root.locale(locale)
                language, script, territory, variant = chain.tagCodes()
                assert language
                # TODO: this skip should probably be based on likely
                # sub-tags, instead of empty territory: if locale has a
                # likely-subtag expansion, that's what QLocale uses,
                # and we'll be saving its data for the expanded locale
                # anyway, so don't need to record it for itself.
                # See also QLocaleXmlReader.loadLocaleMap's grumble.
                if not territory:
                    continue
                yield self.__getLocaleData(chain, calendars, language, script, territory, variant)
            except Error as e:
                self.grumble(f'Skipping file locale "{locale}" ({e})\n')

    import textwrap
    @staticmethod
    def __wrapped(writer, prefix, tokens, wrap = textwrap.wrap) -> None:
        writer('\n'.join(wrap(prefix + ', '.join(tokens),
                              subsequent_indent=' ', width=80)) + '\n')
    del textwrap

    def __parseTags(self, locale: str) -> tuple[int, int, int, int]:
        tags: Iterator[str] = self.__splitLocale(locale)
        language: str = next(tags)
        script = territory = variant = ''
        try:
            script, territory, variant = tags
        except ValueError:
            pass
        return tuple(p[0] for p in self.root.codesToIdName(language, script, territory, variant))

    def __splitLocale(self, name: str) ->  Iterator[str]:
        """Generate (language, script, territory, variant) from a locale name

        Ignores any trailing fields (with a warning), leaves script (a
        capitalised four-letter token), territory (either a number or
        an all-uppercase token) or variant (upper case and digits)
        empty if unspecified.  Only generates one entry if name is a
        single tag (i.e. contains no underscores).  Always yields 1 or
        4 values, never 2 or 3."""
        tags: Iterator[str] = iter(name.split('_'))
        yield next(tags) # Language

        try:
            tag: str = next(tags)
        except StopIteration:
            return

        # Script is always four letters, always capitalised:
        if len(tag) == 4 and tag[0].isupper() and tag[1:].islower():
            yield tag
            try:
                tag = next(tags)
            except StopIteration:
                tag = ''
        else:
            yield ''

        # Territory is upper-case or numeric:
        if tag and tag.isupper() or tag.isdigit():
            yield tag
            try:
                tag = next(tags)
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

        rest = [tag] if tag else []
        rest.extend(tags)

        if rest:
            self.grumble(f'Ignoring unparsed cruft {"_".join(rest)} in {name}\n')

    def __getLocaleData(self, scan: LocaleScanner, calendars: list[str], language: str,
                        script: str, territory: str, variant: str) -> Locale:
        ids, names = zip(*self.root.codesToIdName(language, script, territory, variant))
        assert ids[0] > 0 and ids[2] > 0, (language, script, territory, variant)
        locale = Locale(
            language = names[0], language_code = language, language_id = ids[0],
            script = names[1], script_code = script, script_id = ids[1],
            territory = names[2], territory_code = territory, territory_id = ids[2],
            variant_code = variant, variant_id = ids[3])

        firstDay, weStart, weEnd = self.root.weekData(territory)
        assert all(day in ('mon', 'tue', 'wed', 'thu', 'fri', 'sat', 'sun')
                   for day in (firstDay, weStart, weEnd))

        locale.update(firstDayOfWeek = firstDay,
                      weekendStart = weStart,
                      weekendEnd = weEnd)

        iso, digits, rounding = self.root.currencyData(territory)
        locale.update(currencyIsoCode = iso,
                      currencyDigits = int(digits),
                      currencyRounding = int(rounding))

        locale.update(scan.currencyData(iso))
        locale.update(scan.numericData(self.root.numberSystem))
        locale.update(scan.textPatternData())
        locale.update(scan.endonyms(language, script, territory, variant))
        locale.update(scan.unitData()) # byte, kB, MB, GB, ..., KiB, MiB, GiB, ...
        locale.update(scan.calendarNames(calendars)) # Names of days and months

        # Naming of timezones:
        locale.update(scan.timeZoneNames(self.__bcp47Alias))

        return locale

# Note: various caches assume this class is a singleton, so the
# "default" value for a parameter no caller should pass can serve as
# the cache. If a process were to instantiate this class with distinct
# roots, each cache would be filled by the first to need it !
class CldrAccess (object):
    def __init__(self, root: Path) -> None:
        """Set up a master object for accessing CLDR data.

        Single parameter, root, is the file-system path to the root of
        the unpacked CLDR archive; its common/ sub-directory should
        contain dtd/, main/ and supplemental/ sub-directories."""
        self.root = root

    def xml(self, relative_path: str) -> XmlScanner:
        """Load a single XML file and return its root element as an XmlScanner.

        The path is interpreted relative to self.root"""
        return XmlScanner(Node(self.__xml(relative_path)))

    def supplement(self, name: str) -> Supplement:
        """Loads supplemental data as a Supplement object.

        The name should be that of a file in common/supplemental/, without path.
        """
        return Supplement(Node(self.__xml(f'common/supplemental/{name}')))

    def locale(self, name: str) -> LocaleScanner:
        """Loads all data for a locale as a LocaleScanner object.

        The name should be a locale name; adding suffix '.xml' to it
        should usually yield a file in common/main/.  The returned
        LocaleScanner object packages this file along with all those
        from which it inherits; its methods know how to handle that
        inheritance, where relevant."""
        return LocaleScanner(name, self.__localeRoots(name), self.__rootLocale)

    # see QLocaleXmlWriter.enumData()
    def englishNaming(self, tag: str) -> Callable[[str, str], str]:
        return self.__codeMap(tag).get

    @property
    def fileLocales(self) -> Iterable[str]:
        """Generator for locale IDs seen in file-names.

        All *.xml other than root.xml in common/main/ are assumed to
        identify locales."""
        for path in self.root.joinpath('common/main').glob('*.xml'):
            if path.stem != 'root':
                yield path.stem

    @property
    def defaultContentLocales(self) -> Iterator[str]:
        """Generator for the default content locales."""
        for name, attrs in self.supplement('supplementalMetadata.xml').find('metadata/defaultContent'):
            try:
                locales: str = attrs['locales']
            except KeyError:
                pass
            else:
                for locale in locales.split():
                    yield locale

    def likelySubTags(self) -> Iterator[tuple[str, str]]:
        for ignore, attrs in self.supplement('likelySubtags.xml').find('likelySubtags'):
            yield attrs['from'], attrs['to']

    def numberSystem(self, system: str) -> dict[str, str]:
        """Get a description of a numbering system.

        Returns a mapping, with keys 'digits', 'type' and 'id'; the
        value for this last is system. Raises KeyError for unknown
        number system, ldml.Error on failure to load data."""
        try:
            return self.__numberSystems[system]
        except KeyError:
            raise Error(f'Unsupported number system: {system}')

    def weekData(self, territory: str) -> tuple[str, str, str]:
        """Data on the weekly cycle.

        Returns a triple (W, S, E) of en's short names for week-days;
        W is the first day of the week, S the start of the week-end
        and E the end of the week-end.  Where data for a territory is
        unavailable, the data for CLDR's territory 001 (The World) is
        used."""
        try:
            return self.__weekData[territory]
        except KeyError:
            return self.__weekData['001']

    def currencyData(self, territory: str) -> tuple[str, int, int]:
        """Returns currency data for the given territory code.

        Return value is a tuple (ISO4217 code, digit count, rounding
        mode).  If CLDR provides no data for this territory, ('', 2, 1)
        is the default result.
        """
        try:
            return self.__currencyData[territory]
        except KeyError:
            return '', 2, 1

    def codesToIdName(self, language: str, script: str, territory: str, variant: str = ''
                     ) -> tuple[tuple[int, str], tuple[int, str],
                                tuple[int, str], tuple[int, str]]:
        """Maps each code to the appropriate ID and name.

        Returns a 4-tuple of (ID, name) pairs corresponding to the
        language, script, territory and variant given.  Raises a
        suitable error if any of them is unknown, indicating all that
        are unknown plus suitable names for any that could sensibly be
        added to enumdata.py to make them known.

        Until we implement variant support (QTBUG-81051), the fourth
        member of the returned tuple is always 0 paired with a string
        that should not be used."""
        enum: Callable[[str], dict[str, tuple[int, str]]] = self.__enumMap
        try:
            return (enum('language')[language],
                    enum('script')[script],
                    enum('territory')[territory],
                    enum('variant')[variant])
        except KeyError:
            pass

        parts, values = [], [language, script, territory, variant]
        for index, key in enumerate(('language', 'script', 'territory', 'variant')):
            naming: dict[str, str] = self.__codeMap(key)
            enums: dict[str, tuple[int, str]]  = enum(key)
            value: str = values[index]
            if value not in enums:
                text = f'{key} code {value}'
                name = naming.get(value)
                if name and value != 'POSIX':
                    text += f' (could add {name})'
                parts.append(text)
        if len(parts) > 1:
            parts[-1] = 'and ' + parts[-1]
        else:
            assert parts
            if parts[0].startswith('variant'):
                raise Error(f'No support for {parts[0]}',
                            language, script, territory, variant)
        raise Error('Unknown ' + ', '.join(parts),
                    language, script, territory, variant)

    @staticmethod
    def __checkEnum(given: dict[str, str], proper: dict[str, str], scraps: set[str]
                    ) -> Iterator[tuple[str, str]]:
        # Each is a { code: full name } mapping
        for code, name in given.items():
            try: right: str = proper[code]
            except KeyError:
                # No en.xml name for this code, but supplementalData's
                # parentLocale may still believe in it:
                if code not in scraps:
                    yield name, f'[Found no CLDR name for code {code}]'
                continue
            cleaned: None | str = names_clash(right, name)
            if cleaned:
                yield name, cleaned

    def checkEnumData(self, grumble: Callable[[str], int]) -> None:
        scraps = set()
        for k in self.__parentLocale.keys():
            for f in k.split('_'):
                scraps.add(f)
        from enumdata import language_map, territory_map, script_map
        language = {v: k for k, v in language_map.values() if not v.isspace()}
        territory = {v: k for k, v in territory_map.values() if v != 'ZZ'}
        script = {v: k for k, v in script_map.values() if v != 'Zzzz'}
        lang = dict(self.__checkEnum(language, self.__codeMap('language'), scraps))
        land = dict(self.__checkEnum(territory, self.__codeMap('territory'), scraps))
        text = dict(self.__checkEnum(script, self.__codeMap('script'), scraps))
        if lang or land or text:
            grumble("""\
Using names that don't match CLDR: consider updating the name(s) in
enumdata.py (keeping the old name as an alias):
""")
            if lang:
                grumble('Language:\n\t'
                        + '\n\t'.join(f'{k} -> {v}' for k, v in lang.items())
                        + '\n')
            if land:
                grumble('Territory:\n\t'
                        + '\n\t'.join(f'{k} -> {v}' for k, v in land.items())
                        + '\n')
            if text:
                grumble('Script:\n\t'
                        + '\n\t'.join(f'{k} -> {v}' for k, v in text.items())
                        + '\n')
            grumble('\n')

    def bcp47Aliases(self) -> tuple[dict[str, str], dict[str, str]]:
        """Reads the mapping from CLDR IDs to IANA IDs

        CLDR identifies timezones in various ways but its standard
        'name' for them, here described as a CLDR ID, has the form of
        an IANA ID. CLDR IDs are stable across time, where IANA IDs
        may be revised over time, for example Asia/Calcutta became
        Asia/Kolkata. When a new zone is added to CLDR, it gets the
        then-current IANA ID as its CLDR ID; if it is later
        superseded, CLDR continues using the old ID, so we need a
        mapping from that to current IANA IDs. Helpfully, CLDR
        provides information about aliasing among time-zone IDs.

        The file common/bcp47/timezone.xml has keyword/key/type
        elements with attributes:

          name -- zone code (ignore)
          description -- long name for exemplar location, including
                         territory

        and some of:

          deprecated -- ignore entry if present (has no alias)
          preferred -- only present if deprecated
          since -- version at which this entry was added (ignore)
          alias -- space-joined sequence of IANA-form IDs; first is CLDR ID
          iana -- if present, repeats the alias entry that's the modern IANA ID

        This returns a pair (alias, naming) wherein: alias is a
        mapping from IANA-format IDs to actual IANA IDs, that maps
        each alias to the contemporary ID used by IANA; and naming is
        a mapping from IANA ID to the description it and its aliases
        shared in their keyword/key/type entry."""
        # File has the same form as supplements:
        root = Supplement(Node(self.__xml('common/bcp47/timezone.xml')))

        # If we ever need a mapping back to CLDR ID, we can make
        # (description, space-joined-list) the naming values.
        alias: dict[str, str] = {} # { alias: iana }
        naming: dict[str, str] = {} # { iana: description }
        for item, attrs in root.find('keyword/key/type', exclude=('deprecated',)):
            assert 'description' in attrs, item
            assert 'alias' in attrs, item
            names = attrs['alias'].split()
            assert not any(name in alias for name in names), item
            # CLDR ID is names[0]; if IANA now uses another name for
            # it, this is given as the iana attribute.
            ianaid, fullName = attrs.get('iana', names[0]), attrs['description']
            alias.update({name: ianaid for name in names})
            assert not ianaid in naming
            naming[ianaid] = fullName

        return alias, naming

    def readWindowsTimeZones(self, alias: dict[str, str]) -> tuple[dict[str, str],
                                                                   list[tuple[str, str, str]]]:
        """Digest CLDR's MS-Win time-zone name mapping.

        Single argument, alias, should be the first part of the pair
        returned by a call to bcp47Aliases(); it shall be used to
        transform CLDR IDs into IANA IDs.

        MS-Win have their own eccentric names for time-zones. CLDR
        helpfully provides a translation to more orthodox names,
        albeit these are CLDR IDs - see bcp47Aliases() - rather than
        (up to date) IANA IDs. The windowsZones.xml supplement has
        supplementalData/windowsZones/mapTimezones/mapZone nodes with
        attributes

          territory -- ISO code
          type -- space-joined sequence of CLDR IDs of zones
          other -- Windows name of these zones in the given territory

        When 'territory' is '001', type is always just a single CLDR
        zone ID. This is the default zone for the given Windows name.

        For each mapZone node, its type is split on spacing and
        cleaned up as follows. Those entries that are keys of alias
        are mapped thereby to their canonical IANA IDs; all others are
        presumed to be canonical IANA IDs and left unchanged.  Any
        later duplicates of earlier entries are omitted. The result
        list of IANA IDs is joined with single spaces between to give
        a string s.

        Returns a twople (defaults, windows) in which defaults is a
        mapping, from Windows ID to IANA ID (derived from the mapZone
        nodes with territory='001'), and windows is a list of triples
        (Windows ID, territory code, IANA ID list) in which the first
        two entries are the 'other' and 'territory' fields of a
        mapZone element and the last is s, its cleaned-up list of IANA
        IDs."""

        defaults: dict[str, str] = {}
        windows: list[tuple[str, str, str]] = []
        zones = self.supplement('windowsZones.xml')
        for name, attrs in zones.find('windowsZones/mapTimezones'):
            if name != 'mapZone':
                continue

            wid, code, ianas = attrs['other'], attrs['territory'], []
            for cldr in attrs['type'].split():
                iana = alias.get(cldr, cldr)
                if iana not in ianas:
                    ianas.append(iana)

            if code == '001':
                assert len(ianas) == 1, (wid, *ianas)
                defaults[wid] = ianas[0]
            else:
                windows.append((wid, code, ' '.join(ianas)))

        # For each Windows ID, its default zone is its zone for at
        # least some territory:
        assert all(any(True for w, code, seq in windows
                       if w == wid and zone in seq.split())
                   for wid, zone in defaults.items()), (defaults, windows)

        return defaults, windows

    def readMetaZoneMap(self, alias: dict[str, str]
                        ) -> tuple[dict[str, dict[str, str]],
                                   dict[str, tuple[tuple[int, int, str], ...]],
                                   dict[str, str]]:
        """Digests the metaZones supplemental data.

        Required argument, alias, should be the first of
        bcp47Aliases()'s pair of returns; it is used to map CLDR IDs
        to IANA IDs.

        Ignores the metazoneIds, at least for now. Reads mapTimezones
        as a mapping from metazone to mapping from territory (possibly
        via CLDR ID) to IANA ID; the entry for territory '001' (world)
        is the exemplar zone for the metazone. Reads metazoneInfo as a
        mapping from IANA IDs (possibly given as CLDR IDs) to usage
        histories. Reads primaryZones as a mapping from territory (via
        CLDR IDs) to IANA IDs. Returns a triple of these three
        mappings, in the given order, with the territories mapped to
        pairs of: an integer value of a corresponding
        QLocale::Territory and the territory code (a short string).

        The usage history of each zone is a sequence of triples (to,
        from, metazone) indicating an interval of time, bounded by to
        and from, during which the zone followed the metazone. A
        bounding time of None indicates no bound at its end;
        otherwise, each bound is a "yyyy-MM-dd hh:mm" datetime
        string. Earlier time intervals are sorted before later;
        successive intervals usually abut.

        This is the locale-independent data that we need to glue
        together the ldml.LocaleScanner.timeZoneNames() data per
        locale."""
        metaZones = self.supplement('metaZones.xml') # Doesn't appear to use draft attribute
        # Map CLDR name to IANA name (or use CLDR name if unknown to alias):
        zoneName: Callable[[str], str] = lambda n, g=alias.get: g(n, n)

        metaMap: dict[str, dict[str, str]] = {} # { meta: { territory code: zoneId } }
        # Entry with territory 001 is "golden zone" for the metazone.
        for mapMeta in metaZones.findNodes('metaZones/mapTimezones'):
            attrs = mapMeta.attributes()
            if attrs.get('type', '') != 'metazones':
                continue
            if attrs.get('typeVersion', '') != '2018e':
                # We may be able to cope, but someone needs to check
                raise Error('Version of metazone map type is not 2018e', attrs)

            for node in mapMeta.findAllChildren('mapZone'):
                attrs: dict[str, str] = node.attributes()
                try:
                    meta, code, zone = attrs['other'], attrs['territory'], attrs['type']
                except KeyError:
                    continue

                bok: dict[str, str] = metaMap.setdefault(meta, {})
                assert code not in bok, (meta, code)
                bok[code] = zoneName(zone)
        # Territories not named in a metaMap entry fall back on the
        # value for 001, so every metaMap must have this "golden"
        # entry:
        assert all('001' in bok for bok in metaMap.values())

        def scanUses(zone: Node, check=metaMap) -> Iterator[tuple[str|None, str|None, str]]:
            for node in zone.findAllChildren('usesMetazone'):
                attrs: dict[str, str] = node.attributes()
                mzone: str = attrs['mzone']
                if mzone not in check:
                    raise Error('Unknown metazone', mzone)
                # These are UTC date-times.
                yield attrs.get('from'), attrs.get('to'), mzone

        def sortKey(triple: tuple[str|None, str|None, str]) -> str | None:
            start, stop, mzone = triple
            # The start = None entry should sort first; since its key
            # is its stop, which is likely the next entry's start, we
            # append a zero-second field to any non-None start to make
            # it sort (lexically) after its predecessor's stop value.
            return f'{start}:00' if start else stop
            # (If both start and stop are None, there's only one entry
            # in the list, so the sorting is fatuous and the key
            # doesn't matter).

        def timeRep(text: str, notime: bool, epoch=datetime(1970, 1, 1, 0, 0)) -> int:
            """Map a 'yyyy-MM-dd HH:mm' string to epoch minutes.

            If the HH:mm part is omitted, second parameter notime is true to
            use the end of the day, false for the start. LDML specifies this
            reading of the pure-date values for start and stop attributes.  If
            the HH:mm part is 24:00, the end of the day is also used; LDML
            specifies this but python's datetime.fromisoformat() doesn't like
            it.

            Since the datetimes are given as UTC and only to minute precision,
            with only 1440 minutes in a day, 32 bits can represent a span of
            over 8 thousand years in epoch minutes form. As we use this only
            for times of changes to metazone status of a zone, all after the
            epoch, we can use an unsigned 32-bit int to reach year 10136."""
            try:
                if len(text) == 10:
                    if notime:
                        raise ValueError
                    # else: fromisoformat reads it as start of day
                else:
                    assert len(text) == 16, text

                # If it's given with HH:mm as 24:00, this throws:
                diff: timedelta = datetime.fromisoformat(text) - epoch
            except ValueError:
                diff = datetime.fromisoformat(text[:10]) - epoch
                diff += diff.__class__(days=1)

            assert diff.days >= 0 and diff.seconds >= 0, (diff, text)
            mins, secs = divmod(diff.seconds, 60)
            assert secs == 0, (diff, text)
            return diff.days * 1440 + mins

        def mapTimes(triple: tuple[str|None, str|None, str],
                     alpha: int = 0, omega: int = (1<<32) - 1,
                     torep: Callable[[str, bool, datetime], int] = timeRep
                     ) -> tuple[int, int, str]:
            start, stop, mzone = triple
            start = alpha if start is None else torep(start, False)
            stop = omega if stop is None else torep(stop, True)
            # Treat any date after omega - 1 as end of time, as we need omega -
            # 1 as a sentinel value in the C++ binary chop code.
            if stop + 1 >= omega:
                stop = omega
            return start, stop, mzone

        # zones is { ianaId: ( (from, to, meta), ... ) }
        zones: dict[str, tuple[tuple[int, int, str], ...]] = {}
        for metaInfo in metaZones.findNodes('metaZones/metazoneInfo'):
            for zone in metaInfo.findAllChildren('timezone'):
                iana: str = zoneName(zone.dom.attributes['type'].value)
                story = tuple(sorted(scanUses(zone), key=sortKey))
                # Only {first,last} entry can have None for {from,to}:
                assert not any(s[0] is None for s in story[1:]), (iana, story)
                assert not any(s[1] is None for s in story[:-1]), (iana, story)
                # Each ends when or before the next starts:
                assert all(old[1] <= new[0]
                           for old, new in zip(story[:-1], story[1:])), (iana, story)
                # Now repack story in our compact form:
                zones[iana] = tuple(mapTimes(x) for x in story)

        # Every mapTimeZones entry should have a reverse entry in
        # metazoneInfo:
        assert all(all(any(metaz == s[-1] for s in zones.get(zone, ()))
                       for zone in bok.values())
                   for metaz, bok in metaMap.items())

        territorial: dict[str, str] = {} # { territory code: IANA ID }
        for prime in metaZones.findNodes('primaryZones/primaryZone'):
            code = prime.attributes()['iso3166']
            assert code not in territorial, code
            territorial[code] = zoneName(prime.dom.childNodes[0].wholeText)

        return metaMap, zones, territorial

    @property
    def cldrVersion(self) -> str:
        # Evaluate so as to ensure __cldrVersion is set:
        self.__unDistinguishedAttributes
        return self.__cldrVersion

    # Implementation details
    def __xml(self, relPath: str, cache = CacheDict(), read = minidom.parse) -> minidom.Element:
        try:
            doc: minidom.Element = cache[relPath]
        except KeyError:
            cache[relPath] = doc = read(str(self.root.joinpath(relPath))).documentElement
        return doc

    def __open(self, relative_path: str) -> TextIO:
        return self.root.joinpath(relative_path).open()

    @property
    def __rootLocale(self, cache: list[XmlScanner] = []) -> XmlScanner:
        if not cache:
            cache.append(self.xml('common/main/root.xml'))
        return cache[0]

    @property
    def __supplementalData(self, cache: list[Supplement] = []) -> Supplement:
        if not cache:
            cache.append(self.supplement('supplementalData.xml'))
        return cache[0]

    @property
    def __numberSystems(self, cache: dict[str, dict[str, str]] = {}) -> dict[str, dict[str, str]]:
        if not cache:
            for ignore, attrs in self.supplement('numberingSystems.xml').find('numberingSystems'):
                cache[attrs['id']] = attrs
            assert cache
        return cache

    @property
    def __weekData(self, cache: dict[str, tuple[str, str, str]] = {}
                   ) -> dict[str, tuple[str, str, str]]:
        if not cache:
            # firstDay, weStart and weEnd are all dict[str, str]
            firstDay, weStart, weEnd = self.__getWeekData()
            # Massage those into an easily-consulted form:
            # World defaults given for code '001':
            mon, sat, sun = firstDay['001'], weStart['001'], weEnd['001']
            lands: set[str] = set(firstDay) | set(weStart) | set(weEnd)
            cache.update((land,
                          (firstDay.get(land, mon), weStart.get(land, sat), weEnd.get(land, sun)))
                         for land in lands)
            assert cache
        return cache

    def __getWeekData(self) -> Iterator[dict[str, str]]:
        """Scan for data on the weekly cycle.

        Yields three mappings from locales to en's short names for
        week-days; if a locale isn't a key of a given mapping, it
        should use the '001' (world) locale's value. The first mapping
        gives the day on which the week starts, the second gives the
        day on which the week-end starts, the third gives the last day
        of the week-end."""
        source: Supplement = self.__supplementalData
        for key in ('firstDay', 'weekendStart', 'weekendEnd'):
            result: dict[str, str] = {}
            for ignore, attrs in source.find(f'weekData/{key}'):
                assert ignore == key
                day: str = attrs['day']
                assert day in ('mon', 'tue', 'wed', 'thu', 'fri', 'sat', 'sun'), day
                if 'alt' in attrs:
                    continue
                for loc in attrs.get('territories', '').split():
                    result[loc] = day
            yield result

    @property
    def __currencyData(self, cache: dict[str, tuple[str, int, int]] = {}
                       ) -> dict[str, tuple[str, int, int]]:
        if not cache:
            source = self.__supplementalData
            for elt in source.findNodes('currencyData/region'):
                iso, digits, rounding = '', 2, 1
                # TODO: fractions/info[iso4217=DEFAULT] has rounding=0 - why do we differ ?
                # Also: some fractions/info have cashDigits and cashRounding - should we use them ?
                try:
                    territory = elt.dom.attributes['iso3166'].nodeValue
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
                        f'currencyData/fractions/info[iso4217={iso}]'):
                        digits = int(data['digits'])
                        rounding = int(data['rounding'])
                cache[territory] = iso, digits, rounding
            assert cache

        return cache

    @property
    def __unDistinguishedAttributes(self, cache: dict[str, tuple[str, ...]] = {}
                                    ) -> dict[str, tuple[str, ...]]:
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

    def __scanLdmlDtd(self) -> Iterator[tuple[str, tuple[str, ...]]]:
        """Scan the LDML DTD, record CLDR version

        Yields (tag, attrs) pairs: on elements with a given tag,
        attributes named in its attrs (a tuple) may be ignored in an
        XPath search; other attributes are distinguished attributes,
        in the terminology of LDML's locale-inheritance rules.

        Sets self.__cldrVersion as a side-effect, since this
        information is found in the same file."""
        with self.__open('common/dtd/ldml.dtd') as dtd:
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

    def __enumMap(self, key: str, cache: dict[str, dict[str, tuple[int, str]]] = {}
                  ) -> dict[str, tuple[int, str]]:
        if not cache:
            cache['variant'] = {'': (0, 'This should never be seen outside ldml.py')}
            # They're mappings from numeric value to pairs of full
            # name and short code. What we want, in each case, is a
            # mapping from code to the other two.
            from enumdata import language_map, script_map, territory_map
            for form, book, empty in (('language', language_map, 'AnyLanguage'),
                                      ('script', script_map, 'AnyScript'),
                                      ('territory', territory_map, 'AnyTerritory')):
                cache[form] = {pair[1]: (num, pair[0])
                               for num, pair in book.items() if pair[0] != 'C'}
                # (Have to filter out the C locale, as we give it the
                # same (all space) code as AnyLanguage, whose code
                # should probably be 'und' instead.)

                # Map empty to zero and the any value:
                cache[form][''] = (0, empty)
            # and map language code 'und' also to (0, any):
            cache['language']['und'] = (0, 'AnyLanguage')

        return cache[key]

    def __codeMap(self, key: str, cache: dict[str, dict[str, str]] = {},
                  # Maps our name for it to CLDR's name:
                  naming = {'language': 'languages', 'script': 'scripts',
                            'territory': 'territories', 'variant': 'variants'}) -> dict[str, str]:
        if not cache:
            root: Node = self.xml('common/main/en.xml').root.findUniqueChild('localeDisplayNames')
            for dst, src in naming.items():
                cache[dst] = dict(self.__codeMapScan(root.findUniqueChild(src)))
            assert cache

        return cache[key]

    def __codeMapScan(self, node: Node) -> Iterator[tuple[str, str]]:
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
                # Prefer stand-alone forms of names when present, ignore other
                # alt="..." entries. For example, Traditional and Simplified
                # Han omit "Han" in the plain form, but include it for
                # stand-alone. As the stand-alone version appears later, it
                # over-writes the plain one.
                if (key not in seen or 'alt' not in elt.attributes
                    or elt.attributes['alt'].nodeValue == 'stand-alone'):
                    yield key, value
                    seen.add(key)

    # CLDR uses inheritance between locales to save repetition:
    @property
    def __parentLocale(self, cache: dict[str, str] = {}) -> dict[str, str]:
        # see http://www.unicode.org/reports/tr35/#Parent_Locales
        if not cache:
            for tag, attrs in self.__supplementalData.find('parentLocales',
                                                           ('component',)):
                parent: str = attrs.get('parent', '')
                for child in attrs['locales'].split():
                    cache[child] = parent
            assert cache

        return cache

    def __scanLocaleRoots(self, name: str) -> Iterator[Node]:
        while name and name != 'root':
            path = f'common/main/{name}.xml'
            if self.root.joinpath(path).exists():
                elt: minidom.Element = self.__xml(path) # which has no top-level alias children:
                assert not any(True
                               for child in Node(elt).findAllChildren(
                                       'alias', allDull=True)
                               ), (f"Locale {name} "
                                   "has an archaic top-level alias element")
                yield Node(elt, self.__unDistinguishedAttributes)

            try:
                name = self.__parentLocale[name]
            except KeyError:
                try:
                    name, tail = name.rsplit('_', 1)
                except ValueError: # No tail to discard: we're done
                    break

    class __Seq (list): pass # No weakref for tuple and list, but list sub-class is ok.
    def __localeRoots(self, name: str, cache = CacheDict()) -> __Seq:
        try:
            chain: CldrAccess.__Seq = cache[name]
        except KeyError:
            cache[name] = chain = CldrAccess.__Seq(self.__scanLocaleRoots(name))
        return chain

# Unpolute the namespace: we don't need to export these.
del minidom, CacheDict
