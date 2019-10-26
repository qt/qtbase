#!/usr/bin/env python2
#############################################################################
##
## Copyright (C) 2019 The Qt Company Ltd.
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
"""Parse CLDR data for QTimeZone use with MS-Windows

Script to parse the CLDR supplemental/windowsZones.xml file and encode
for use in QTimeZone.  See ``./cldr2qlocalexml.py`` for where to get
the CLDR data.  Pass its common/ directory as first parameter to this
script and the qtbase root directory as second parameter.  It shall
update qtbase's src/corelib/time/qtimezoneprivate_data_p.h ready for
use.

The XML structure is as follows:

 <supplementalData>
     <version number="$Revision:...$"/>
     <generation date="$Date:...$"/>
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

import os
import sys
import datetime
import tempfile
import enumdata
import xpathlite
from  xpathlite import DraftResolution
import re
import qlocalexml2cpp

findAlias = xpathlite.findAlias
findEntry = xpathlite.findEntry
findEntryInFile = xpathlite._findEntryInFile
findTagsInFile = xpathlite.findTagsInFile
unicode2hex = qlocalexml2cpp.unicode2hex
wrap_list = qlocalexml2cpp.wrap_list

class ByteArrayData:
    def __init__(self):
        self.data = []
        self.hash = {}
    def append(self, s):
        s = s + '\0'
        if s in self.hash:
            return self.hash[s]

        lst = unicode2hex(s)
        index = len(self.data)
        if index > 65535:
            print "\n\n\n#error Data index is too big!"
            sys.stderr.write ("\n\n\nERROR: index exceeds the uint16 range! index = %d\n" % index)
            sys.exit(1)
        self.hash[s] = index
        self.data += lst
        return index

# List of currently known Windows IDs.
# If this script reports missing IDs, please add them here.
# Look up the offset using (google and) timeanddate.com.
# Not public so may safely be changed.  Please keep in alphabetic order by ID.
# ( Windows Id, Offset Seconds )
windowsIdList = (
    (u'Afghanistan Standard Time',        16200),
    (u'Alaskan Standard Time',           -32400),
    (u'Aleutian Standard Time',          -36000),
    (u'Altai Standard Time',              25200),
    (u'Arab Standard Time',               10800),
    (u'Arabian Standard Time',            14400),
    (u'Arabic Standard Time',             10800),
    (u'Argentina Standard Time',         -10800),
    (u'Astrakhan Standard Time',          14400),
    (u'Atlantic Standard Time',          -14400),
    (u'AUS Central Standard Time',        34200),
    (u'Aus Central W. Standard Time',     31500),
    (u'AUS Eastern Standard Time',        36000),
    (u'Azerbaijan Standard Time',         14400),
    (u'Azores Standard Time',             -3600),
    (u'Bahia Standard Time',             -10800),
    (u'Bangladesh Standard Time',         21600),
    (u'Belarus Standard Time',            10800),
    (u'Bougainville Standard Time',       39600),
    (u'Canada Central Standard Time',    -21600),
    (u'Cape Verde Standard Time',         -3600),
    (u'Caucasus Standard Time',           14400),
    (u'Cen. Australia Standard Time',     34200),
    (u'Central America Standard Time',   -21600),
    (u'Central Asia Standard Time',       21600),
    (u'Central Brazilian Standard Time', -14400),
    (u'Central Europe Standard Time',      3600),
    (u'Central European Standard Time',    3600),
    (u'Central Pacific Standard Time',    39600),
    (u'Central Standard Time (Mexico)',  -21600),
    (u'Central Standard Time',           -21600),
    (u'China Standard Time',              28800),
    (u'Chatham Islands Standard Time',    45900),
    (u'Cuba Standard Time',              -18000),
    (u'Dateline Standard Time',          -43200),
    (u'E. Africa Standard Time',          10800),
    (u'E. Australia Standard Time',       36000),
    (u'E. Europe Standard Time',           7200),
    (u'E. South America Standard Time',  -10800),
    (u'Easter Island Standard Time',     -21600),
    (u'Eastern Standard Time',           -18000),
    (u'Eastern Standard Time (Mexico)',  -18000),
    (u'Egypt Standard Time',               7200),
    (u'Ekaterinburg Standard Time',       18000),
    (u'Fiji Standard Time',               43200),
    (u'FLE Standard Time',                 7200),
    (u'Georgian Standard Time',           14400),
    (u'GMT Standard Time',                    0),
    (u'Greenland Standard Time',         -10800),
    (u'Greenwich Standard Time',              0),
    (u'GTB Standard Time',                 7200),
    (u'Haiti Standard Time',             -18000),
    (u'Hawaiian Standard Time',          -36000),
    (u'India Standard Time',              19800),
    (u'Iran Standard Time',               12600),
    (u'Israel Standard Time',              7200),
    (u'Jordan Standard Time',              7200),
    (u'Kaliningrad Standard Time',         7200),
    (u'Korea Standard Time',              32400),
    (u'Libya Standard Time',               7200),
    (u'Line Islands Standard Time',       50400),
    (u'Lord Howe Standard Time',          37800),
    (u'Magadan Standard Time',            36000),
    (u'Magallanes Standard Time',        -10800), # permanent DST
    (u'Marquesas Standard Time',         -34200),
    (u'Mauritius Standard Time',          14400),
    (u'Middle East Standard Time',         7200),
    (u'Montevideo Standard Time',        -10800),
    (u'Morocco Standard Time',                0),
    (u'Mountain Standard Time (Mexico)', -25200),
    (u'Mountain Standard Time',          -25200),
    (u'Myanmar Standard Time',            23400),
    (u'N. Central Asia Standard Time',    21600),
    (u'Namibia Standard Time',             3600),
    (u'Nepal Standard Time',              20700),
    (u'New Zealand Standard Time',        43200),
    (u'Newfoundland Standard Time',      -12600),
    (u'Norfolk Standard Time',            39600),
    (u'North Asia East Standard Time',    28800),
    (u'North Asia Standard Time',         25200),
    (u'North Korea Standard Time',        30600),
    (u'Omsk Standard Time',               21600),
    (u'Pacific SA Standard Time',        -10800),
    (u'Pacific Standard Time',           -28800),
    (u'Pacific Standard Time (Mexico)',  -28800),
    (u'Pakistan Standard Time',           18000),
    (u'Paraguay Standard Time',          -14400),
    (u'Qyzylorda Standard Time',          18000), # a.k.a. Kyzylorda, in Kazakhstan
    (u'Romance Standard Time',             3600),
    (u'Russia Time Zone 3',               14400),
    (u'Russia Time Zone 10',              39600),
    (u'Russia Time Zone 11',              43200),
    (u'Russian Standard Time',            10800),
    (u'SA Eastern Standard Time',        -10800),
    (u'SA Pacific Standard Time',        -18000),
    (u'SA Western Standard Time',        -14400),
    (u'Saint Pierre Standard Time',      -10800), # New France
    (u'Sakhalin Standard Time',           39600),
    (u'Samoa Standard Time',              46800),
    (u'Sao Tome Standard Time',               0),
    (u'Saratov Standard Time',            14400),
    (u'SE Asia Standard Time',            25200),
    (u'Singapore Standard Time',          28800),
    (u'South Africa Standard Time',        7200),
    (u'Sri Lanka Standard Time',          19800),
    (u'Sudan Standard Time',               7200), # unless they mean South Sudan, +03:00
    (u'Syria Standard Time',               7200),
    (u'Taipei Standard Time',             28800),
    (u'Tasmania Standard Time',           36000),
    (u'Tocantins Standard Time',         -10800),
    (u'Tokyo Standard Time',              32400),
    (u'Tomsk Standard Time',              25200),
    (u'Tonga Standard Time',              46800),
    (u'Transbaikal Standard Time',        32400), # Yakutsk
    (u'Turkey Standard Time',              7200),
    (u'Turks And Caicos Standard Time',  -14400),
    (u'Ulaanbaatar Standard Time',        28800),
    (u'US Eastern Standard Time',        -18000),
    (u'US Mountain Standard Time',       -25200),
    (u'UTC-11',                          -39600),
    (u'UTC-09',                          -32400),
    (u'UTC-08',                          -28800),
    (u'UTC-02',                           -7200),
    (u'UTC',                                  0),
    (u'UTC+12',                           43200),
    (u'UTC+13',                           46800),
    (u'Venezuela Standard Time',         -16200),
    (u'Vladivostok Standard Time',        36000),
    (u'Volgograd Standard Time',          14400),
    (u'W. Australia Standard Time',       28800),
    (u'W. Central Africa Standard Time',   3600),
    (u'W. Europe Standard Time',           3600),
    (u'W. Mongolia Standard Time',        25200), # Hovd
    (u'West Asia Standard Time',          18000),
    (u'West Bank Standard Time',           7200),
    (u'West Pacific Standard Time',       36000),
    (u'Yakutsk Standard Time',            32400),
)

def windowsIdToKey(windowsId):
    for index, pair in enumerate(windowsIdList):
        if pair[0] == windowsId:
            return index + 1
    return 0

# List of standard UTC IDs to use.  Not public so may be safely changed.
# Do not remove IDs, as each entry is part of the API/behavior guarantee.
# ( UTC Id, Offset Seconds )
utcIdList = (
    (u'UTC',            0),  # Goes first so is default
    (u'UTC-14:00', -50400),
    (u'UTC-13:00', -46800),
    (u'UTC-12:00', -43200),
    (u'UTC-11:00', -39600),
    (u'UTC-10:00', -36000),
    (u'UTC-09:00', -32400),
    (u'UTC-08:00', -28800),
    (u'UTC-07:00', -25200),
    (u'UTC-06:00', -21600),
    (u'UTC-05:00', -18000),
    (u'UTC-04:30', -16200),
    (u'UTC-04:00', -14400),
    (u'UTC-03:30', -12600),
    (u'UTC-03:00', -10800),
    (u'UTC-02:00',  -7200),
    (u'UTC-01:00',  -3600),
    (u'UTC-00:00',      0),
    (u'UTC+00:00',      0),
    (u'UTC+01:00',   3600),
    (u'UTC+02:00',   7200),
    (u'UTC+03:00',  10800),
    (u'UTC+03:30',  12600),
    (u'UTC+04:00',  14400),
    (u'UTC+04:30',  16200),
    (u'UTC+05:00',  18000),
    (u'UTC+05:30',  19800),
    (u'UTC+05:45',  20700),
    (u'UTC+06:00',  21600),
    (u'UTC+06:30',  23400),
    (u'UTC+07:00',  25200),
    (u'UTC+08:00',  28800),
    (u'UTC+08:30',  30600),
    (u'UTC+09:00',  32400),
    (u'UTC+09:30',  34200),
    (u'UTC+10:00',  36000),
    (u'UTC+11:00',  39600),
    (u'UTC+12:00',  43200),
    (u'UTC+13:00',  46800),
    (u'UTC+14:00',  50400),
)

def usage():
    print "Usage: cldr2qtimezone.py <path to cldr core/common> <path to qtbase>"
    sys.exit()

if len(sys.argv) != 3:
    usage()

cldrPath = sys.argv[1]
qtPath = sys.argv[2]

if not os.path.isdir(cldrPath) or not os.path.isdir(qtPath):
    usage()

windowsZonesPath = cldrPath + "/supplemental/windowsZones.xml"
tempFileDir = qtPath
dataFilePath = qtPath + "/src/corelib/time/qtimezoneprivate_data_p.h"

if not (os.path.isfile(windowsZonesPath) and os.path.isfile(dataFilePath)):
    usage()

cldr_version = 'unknown'
ldml = open(cldrPath + "/dtd/ldml.dtd", "r")
for line in ldml:
    if 'version cldrVersion CDATA #FIXED' in line:
        cldr_version = line.split('"')[1]

# [[u'version', [(u'number', u'$Revision: 7825 $')]]]
versionNumber = findTagsInFile(windowsZonesPath, "version")[0][1][0][1]

mapTimezones = findTagsInFile(windowsZonesPath, "windowsZones/mapTimezones")

defaultDict = {}
windowsIdDict = {}

if mapTimezones:
    badZones = set()
    for mapZone in mapTimezones:
        # [u'mapZone', [(u'territory', u'MH'), (u'other', u'UTC+12'), (u'type', u'Pacific/Majuro Pacific/Kwajalein')]]
        if mapZone[0] == u'mapZone':
            data = {}
            for attribute in mapZone[1]:
                if attribute[0] == u'other':
                    data['windowsId'] = attribute[1]
                if attribute[0] == u'territory':
                    data['countryCode'] = attribute[1]
                if attribute[0] == u'type':
                    data['ianaList'] = attribute[1]

            data['windowsKey'] = windowsIdToKey(data['windowsId'])
            if data['windowsKey'] <= 0:
                badZones.add(data['windowsId'])

            countryId = 0
            if data['countryCode'] == u'001':
                defaultDict[data['windowsKey']] = data['ianaList']
            else:
                data['countryId'] = enumdata.countryCodeToId(data['countryCode'])
                if data['countryId'] < 0:
                    raise xpathlite.Error("Unknown Country Code \"%s\"" % data['countryCode'])
                data['country'] = enumdata.country_list[data['countryId']][0]
                windowsIdDict[data['windowsKey'], data['countryId']] = data
    if badZones:
        sys.stderr.write('\n\t'.join(["\nUnknown Windows ID, please add:"] + sorted(badZones))
                         + "\nto the windowIdList in cldr2qtimezone.py\n\n")
        raise xpathlite.Error("Unknown Windows IDs")

print "Input file parsed, now writing data"

GENERATED_BLOCK_START = "// GENERATED PART STARTS HERE\n"
GENERATED_BLOCK_END = "// GENERATED PART ENDS HERE\n"

# Create a temp file to write the new data into
(newTempFile, newTempFilePath) = tempfile.mkstemp("qtimezone_data_p", dir=tempFileDir)
newTempFile = os.fdopen(newTempFile, "w")

# Open the old file and copy over the first non-generated section to the new file
oldDataFile = open(dataFilePath, "r")
s = oldDataFile.readline()
while s and s != GENERATED_BLOCK_START:
    newTempFile.write(s)
    s = oldDataFile.readline()

# Write out generated block start tag and warning
newTempFile.write(GENERATED_BLOCK_START)
newTempFile.write("""
/*
    This part of the file was generated on %s from the
    Common Locale Data Repository v%s supplemental/windowsZones.xml file %s

    http://www.unicode.org/cldr/

    Do not edit this code: run cldr2qtimezone.py on updated (or
    edited) CLDR data; see qtbase/util/locale_database/.
*/

""" % (str(datetime.date.today()), cldr_version, versionNumber) )

windowsIdData = ByteArrayData()
ianaIdData = ByteArrayData()

# Write Windows/IANA table
newTempFile.write("// Windows ID Key, Country Enum, IANA ID Index\n")
newTempFile.write("static const QZoneData zoneDataTable[] = {\n")
for index in sorted(windowsIdDict):
    data = windowsIdDict[index]
    newTempFile.write("    { %6d,%6d,%6d }, // %s / %s\n"
                         % (data['windowsKey'],
                            data['countryId'],
                            ianaIdData.append(data['ianaList']),
                            data['windowsId'],
                            data['country']))
newTempFile.write("    {      0,     0,     0 } // Trailing zeroes\n")
newTempFile.write("};\n\n")

print "Done Zone Data"

# Write Windows ID key table
newTempFile.write("// Windows ID Key, Windows ID Index, IANA ID Index, UTC Offset\n")
newTempFile.write("static const QWindowsData windowsDataTable[] = {\n")
for index, pair in enumerate(windowsIdList):
    newTempFile.write("    { %6d,%6d,%6d,%6d }, // %s\n"
                      % (index + 1, windowsIdData.append(pair[0]),
                         ianaIdData.append(defaultDict[index + 1]), pair[1], pair[0]))
newTempFile.write("    {      0,     0,     0,     0 } // Trailing zeroes\n")
newTempFile.write("};\n\n")

print "Done Windows Data Table"

# Write UTC ID key table
newTempFile.write("// IANA ID Index, UTC Offset\n")
newTempFile.write("static const QUtcData utcDataTable[] = {\n")
for pair in utcIdList:
    newTempFile.write("    { %6d,%6d }, // %s\n"
                      % (ianaIdData.append(pair[0]), pair[1], pair[0]))
newTempFile.write("    {     0,      0 } // Trailing zeroes\n")
newTempFile.write("};\n\n")

print "Done UTC Data Table"

# Write out Windows ID's data
newTempFile.write("static const char windowsIdData[] = {\n")
newTempFile.write(wrap_list(windowsIdData.data))
newTempFile.write("\n};\n\n")

# Write out IANA ID's data
newTempFile.write("static const char ianaIdData[] = {\n")
newTempFile.write(wrap_list(ianaIdData.data))
newTempFile.write("\n};\n")

print "Done ID Data Table"

# Write out the end of generated block tag
newTempFile.write(GENERATED_BLOCK_END)
s = oldDataFile.readline()

# Skip through the old generated data in the old file
while s and s != GENERATED_BLOCK_END:
    s = oldDataFile.readline()

# Now copy the rest of the original file into the new file
s = oldDataFile.readline()
while s:
    newTempFile.write(s)
    s = oldDataFile.readline()

# Now close the old and new file, delete the old file and copy the new file in its place
newTempFile.close()
oldDataFile.close()
os.remove(dataFilePath)
os.rename(newTempFilePath, dataFilePath)

print "Data generation completed, please check the new file at " + dataFilePath
