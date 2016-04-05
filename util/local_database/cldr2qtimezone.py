#!/usr/bin/env python
#############################################################################
##
## Copyright (C) 2016 The Qt Company Ltd.
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


# Script to parse the CLDR supplemental/windowsZones.xml file and encode for use in QTimeZone
# XML structure is as follows:
#
# <supplementalData>
#     <version number="$Revision: 7825 $"/>
#     <generation date="$Date: 2012-10-10 14:45:31 -0700 (Wed, 10 Oct 2012) $"/>
#     <windowsZones>
#         <mapTimezones otherVersion="7dc0101" typeVersion="2012f">
#             <!-- (UTC-08:00) Pacific Time (US & Canada) -->
#             <mapZone other="Pacific Standard Time" territory="001" type="America/Los_Angeles"/>
#             <mapZone other="Pacific Standard Time" territory="CA"  type="America/Vancouver America/Dawson America/Whitehorse"/>
#             <mapZone other="Pacific Standard Time" territory="MX"  type="America/Tijuana"/>
#             <mapZone other="Pacific Standard Time" territory="US"  type="America/Los_Angeles"/>
#             <mapZone other="Pacific Standard Time" territory="ZZ"  type="PST8PDT"/>
#       </mapTimezones>
#     </windowsZones>
# </supplementalData>

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

# List of currently known Windows IDs.  If script fails on missing ID plase add it here
# Not public so may be safely changed.
# Windows Key : [ Windows Id, Offset Seconds ]
windowsIdList = {
    1 : [ u'Afghanistan Standard Time',        16200  ],
    2 : [ u'Alaskan Standard Time',           -32400  ],
    3 : [ u'Arab Standard Time',               10800  ],
    4 : [ u'Arabian Standard Time',            14400  ],
    5 : [ u'Arabic Standard Time',             10800  ],
    6 : [ u'Argentina Standard Time',         -10800  ],
    7 : [ u'Atlantic Standard Time',          -14400  ],
    8 : [ u'AUS Central Standard Time',        34200  ],
    9 : [ u'AUS Eastern Standard Time',        36000  ],
   10 : [ u'Azerbaijan Standard Time',         14400  ],
   11 : [ u'Azores Standard Time',             -3600  ],
   12 : [ u'Bahia Standard Time',             -10800  ],
   13 : [ u'Bangladesh Standard Time',         21600  ],
   14 : [ u'Belarus Standard Time',            10800  ],
   15 : [ u'Canada Central Standard Time',    -21600  ],
   16 : [ u'Cape Verde Standard Time',         -3600  ],
   17 : [ u'Caucasus Standard Time',           14400  ],
   18 : [ u'Cen. Australia Standard Time',     34200  ],
   19 : [ u'Central America Standard Time',   -21600  ],
   20 : [ u'Central Asia Standard Time',       21600  ],
   21 : [ u'Central Brazilian Standard Time', -14400  ],
   22 : [ u'Central Europe Standard Time',      3600  ],
   23 : [ u'Central European Standard Time',    3600  ],
   24 : [ u'Central Pacific Standard Time',    39600  ],
   25 : [ u'Central Standard Time (Mexico)',  -21600  ],
   26 : [ u'Central Standard Time',           -21600  ],
   27 : [ u'China Standard Time',              28800  ],
   28 : [ u'Dateline Standard Time',          -43200  ],
   29 : [ u'E. Africa Standard Time',          10800  ],
   30 : [ u'E. Australia Standard Time',       36000  ],
   31 : [ u'E. South America Standard Time',  -10800  ],
   32 : [ u'Eastern Standard Time',           -18000  ],
   33 : [ u'Eastern Standard Time (Mexico)',  -18000  ],
   34 : [ u'Egypt Standard Time',               7200  ],
   35 : [ u'Ekaterinburg Standard Time',       18000  ],
   36 : [ u'Fiji Standard Time',               43200  ],
   37 : [ u'FLE Standard Time',                 7200  ],
   38 : [ u'Georgian Standard Time',           14400  ],
   39 : [ u'GMT Standard Time',                    0  ],
   40 : [ u'Greenland Standard Time',         -10800  ],
   41 : [ u'Greenwich Standard Time',              0  ],
   42 : [ u'GTB Standard Time',                 7200  ],
   43 : [ u'Hawaiian Standard Time',          -36000  ],
   44 : [ u'India Standard Time',              19800  ],
   45 : [ u'Iran Standard Time',               12600  ],
   46 : [ u'Israel Standard Time',              7200  ],
   47 : [ u'Jordan Standard Time',              7200  ],
   48 : [ u'Kaliningrad Standard Time',         7200  ],
   49 : [ u'Korea Standard Time',              32400  ],
   50 : [ u'Libya Standard Time',               7200  ],
   51 : [ u'Line Islands Standard Time',       50400  ],
   52 : [ u'Magadan Standard Time',            36000  ],
   53 : [ u'Mauritius Standard Time',          14400  ],
   54 : [ u'Middle East Standard Time',         7200  ],
   55 : [ u'Montevideo Standard Time',        -10800  ],
   56 : [ u'Morocco Standard Time',                0  ],
   57 : [ u'Mountain Standard Time (Mexico)', -25200  ],
   58 : [ u'Mountain Standard Time',          -25200  ],
   59 : [ u'Myanmar Standard Time',            23400  ],
   60 : [ u'N. Central Asia Standard Time',    21600  ],
   61 : [ u'Namibia Standard Time',             3600  ],
   62 : [ u'Nepal Standard Time',              20700  ],
   63 : [ u'New Zealand Standard Time',        43200  ],
   64 : [ u'Newfoundland Standard Time',      -12600  ],
   65 : [ u'North Asia East Standard Time',    28800  ],
   66 : [ u'North Asia Standard Time',         25200  ],
   67 : [ u'Pacific SA Standard Time',        -10800  ],
   68 : [ u'E. Europe Standard Time',           7200  ],
   69 : [ u'Pacific Standard Time',           -28800  ],
   70 : [ u'Pakistan Standard Time',           18000  ],
   71 : [ u'Paraguay Standard Time',          -14400  ],
   72 : [ u'Romance Standard Time',             3600  ],
   73 : [ u'Russia Time Zone 3',               14400  ],
   74 : [ u'Russia Time Zone 10',              39600  ],
   75 : [ u'Russia Time Zone 11',              43200  ],
   76 : [ u'Russian Standard Time',            10800  ],
   77 : [ u'SA Eastern Standard Time',        -10800  ],
   78 : [ u'SA Pacific Standard Time',        -18000  ],
   79 : [ u'SA Western Standard Time',        -14400  ],
   80 : [ u'Samoa Standard Time',              46800  ],
   81 : [ u'SE Asia Standard Time',            25200  ],
   82 : [ u'Singapore Standard Time',          28800  ],
   83 : [ u'South Africa Standard Time',        7200  ],
   84 : [ u'Sri Lanka Standard Time',          19800  ],
   85 : [ u'Syria Standard Time',               7200  ],
   86 : [ u'Taipei Standard Time',             28800  ],
   87 : [ u'Tasmania Standard Time',           36000  ],
   88 : [ u'Tokyo Standard Time',              32400  ],
   89 : [ u'Tonga Standard Time',              46800  ],
   90 : [ u'Turkey Standard Time',              7200  ],
   91 : [ u'Ulaanbaatar Standard Time',        28800  ],
   92 : [ u'US Eastern Standard Time',        -18000  ],
   93 : [ u'US Mountain Standard Time',       -25200  ],
   94 : [ u'UTC-02',                           -7200  ],
   95 : [ u'UTC-11',                          -39600  ],
   96 : [ u'UTC',                                  0  ],
   97 : [ u'UTC+12',                           43200  ],
   98 : [ u'Venezuela Standard Time',         -16200  ],
   99 : [ u'Vladivostok Standard Time',        36000  ],
   100: [ u'W. Australia Standard Time',       28800  ],
   101: [ u'W. Central Africa Standard Time',   3600  ],
   102: [ u'W. Europe Standard Time',           3600  ],
   103: [ u'West Asia Standard Time',          18000  ],
   104: [ u'West Pacific Standard Time',       36000  ],
   105: [ u'Yakutsk Standard Time',            32400  ],
   106: [ u'North Korea Standard Time',        30600  ]
}

def windowsIdToKey(windowsId):
    for windowsKey in windowsIdList:
        if windowsIdList[windowsKey][0] == windowsId:
            return windowsKey
    return 0

# List of standard UTC IDs to use.  Not public so may be safely changed.
# Do not remove ID's as is part of API/behavior guarantee
# Key : [ UTC Id, Offset Seconds ]
utcIdList = {
    0 : [ u'UTC',            0  ],  # Goes first so is default
    1 : [ u'UTC-14:00', -50400  ],
    2 : [ u'UTC-13:00', -46800  ],
    3 : [ u'UTC-12:00', -43200  ],
    4 : [ u'UTC-11:00', -39600  ],
    5 : [ u'UTC-10:00', -36000  ],
    6 : [ u'UTC-09:00', -32400  ],
    7 : [ u'UTC-08:00', -28800  ],
    8 : [ u'UTC-07:00', -25200  ],
    9 : [ u'UTC-06:00', -21600  ],
   10 : [ u'UTC-05:00', -18000  ],
   11 : [ u'UTC-04:30', -16200  ],
   12 : [ u'UTC-04:00', -14400  ],
   13 : [ u'UTC-03:30', -12600  ],
   14 : [ u'UTC-03:00', -10800  ],
   15 : [ u'UTC-02:00',  -7200  ],
   16 : [ u'UTC-01:00',  -3600  ],
   17 : [ u'UTC-00:00',      0  ],
   18 : [ u'UTC+00:00',      0  ],
   19 : [ u'UTC+01:00',   3600  ],
   20 : [ u'UTC+02:00',   7200  ],
   21 : [ u'UTC+03:00',  10800  ],
   22 : [ u'UTC+03:30',  12600  ],
   23 : [ u'UTC+04:00',  14400  ],
   24 : [ u'UTC+04:30',  16200  ],
   25 : [ u'UTC+05:00',  18000  ],
   26 : [ u'UTC+05:30',  19800  ],
   27 : [ u'UTC+05:45',  20700  ],
   28 : [ u'UTC+06:00',  21600  ],
   29 : [ u'UTC+06:30',  23400  ],
   30 : [ u'UTC+07:00',  25200  ],
   31 : [ u'UTC+08:00',  28800  ],
   32 : [ u'UTC+09:00',  32400  ],
   33 : [ u'UTC+09:30',  34200  ],
   34 : [ u'UTC+10:00',  36000  ],
   35 : [ u'UTC+11:00',  39600  ],
   36 : [ u'UTC+12:00',  43200  ],
   37 : [ u'UTC+13:00',  46800  ],
   38 : [ u'UTC+14:00',  50400  ],
   39 : [ u'UTC+08:30',  30600  ]
}

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
dataFilePath = qtPath + "/src/corelib/tools/qtimezoneprivate_data_p.h"

if not os.path.isfile(windowsZonesPath):
    usage()

if not os.path.isfile(dataFilePath):
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
                raise xpathlite.Error("Unknown Windows ID, please add \"%s\"" % data['windowsId'])

            countryId = 0
            if data['countryCode'] == u'001':
                defaultDict[data['windowsKey']] = data['ianaList']
            else:
                data['countryId'] = enumdata.countryCodeToId(data['countryCode'])
                if data['countryId'] < 0:
                    raise xpathlite.Error("Unknown Country Code \"%s\"" % data['countryCode'])
                data['country'] = enumdata.country_list[data['countryId']][0]
                windowsIdDict[data['windowsKey'], data['countryId']] = data

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
newTempFile.write("\n\
/*\n\
    This part of the file was generated on %s from the\n\
    Common Locale Data Repository v%s supplemental/windowsZones.xml file %s\n\
\n\
    http://www.unicode.org/cldr/\n\
\n\
    Do not change this data, only generate it using cldr2qtimezone.py.\n\
*/\n\n" % (str(datetime.date.today()), cldr_version, versionNumber) )

windowsIdData = ByteArrayData()
ianaIdData = ByteArrayData()

# Write Windows/IANA table
newTempFile.write("// Windows ID Key, Country Enum, IANA ID Index\n")
newTempFile.write("static const QZoneData zoneDataTable[] = {\n")
for index in windowsIdDict:
    data = windowsIdDict[index]
    newTempFile.write("    { %6d,%6d,%6d }, // %s / %s\n" \
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
for windowsKey in windowsIdList:
    newTempFile.write("    { %6d,%6d,%6d,%6d }, // %s\n" \
                         % (windowsKey,
                            windowsIdData.append(windowsIdList[windowsKey][0]),
                            ianaIdData.append(defaultDict[windowsKey]),
                            windowsIdList[windowsKey][1],
                            windowsIdList[windowsKey][0]))
newTempFile.write("    {      0,     0,     0,     0 } // Trailing zeroes\n")
newTempFile.write("};\n\n")

print "Done Windows Data Table"

# Write UTC ID key table
newTempFile.write("// IANA ID Index, UTC Offset\n")
newTempFile.write("static const QUtcData utcDataTable[] = {\n")
for index in utcIdList:
    data = utcIdList[index]
    newTempFile.write("    { %6d,%6d }, // %s\n" \
                         % (ianaIdData.append(data[0]),
                            data[1],
                            data[0]))
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
