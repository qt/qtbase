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

Provides two class:
  CldrAccess -- used by the reader to access the tree of data files

The former should normally be all you need to access.
See individual classes for further detail.
"""

from xml.dom import minidom
from weakref import WeakValueDictionary as CacheDict
import os

from localetools import Error
from ldml import Node, Supplement

class CldrAccess (object):
    def __init__(self, root):
        """Set up a master object for accessing CLDR data.

        Single parameter, root, is the file-system path to the root of
        the unpacked CLDR archive; its common/ sub-directory should
        contain dtd/, main/ and supplemental/ sub-directories."""
        self.root = root

    def supplement(self, name):
        """Loads supplemental data as a Supplement object.

        The name should be that of a file in common/supplemental/, without path.
        """
        return Supplement(Node(self.__xml(('common', 'supplemental', name))))

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
        self.__scanLdmlDtd()
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
    def __supplementalData(self, cache = []):
        if not cache:
            cache.append(self.supplement('supplementalData.xml'))
        return cache[0]

    def __scanLdmlDtd(self, joinPath = os.path.join):
        """Scan the LDML DTD, record CLDR version."""
        with self.__open(('common', 'dtd', 'ldml.dtd')) as dtd:
            for line in dtd:
                if line.startswith('<!ATTLIST '):
                    parts = line.split()
                    if parts[1:5] == ['version', 'cldrVersion', 'CDATA', '#FIXED']:
                        # parts[5] is the version, in quotes, although the final > might be stuck on its end:
                        self.__cldrVersion = parts[5].split('"')[1]
                        break

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

# Unpolute the namespace: we don't need to export these.
del minidom, CacheDict, os
