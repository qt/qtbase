#!/usr/bin/env python2
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
"""Convert CLDR data to qLocaleXML

The CLDR data can be downloaded from CLDR_, which has a sub-directory
for each version; you need the ``core.zip`` file for your version of
choice (typically the latest). This script has had updates to cope up
to v35; for later versions, we may need adaptations. Unpack the
downloaded ``core.zip`` and check it has a common/main/ sub-directory:
pass the path of that root of the download to this script as its first
command-line argument. Pass the name of the file in which to write
output as the second argument; either omit it or use '-' to select the
standard output. This file is the input needed by
``./qlocalexml2cpp.py``

When you update the CLDR data, be sure to also update
src/corelib/text/qt_attribution.json's entry for unicode-cldr. Check
this script's output for unknown language, country or script messages;
if any can be resolved, use their entry in common/main/en.xml to
append new entries to enumdata.py's lists and update documentation in
src/corelib/text/qlocale.qdoc, adding the new entries in alphabetic
order.

While updating the locale data, check also for updates to MS-Win's
time zone names; see cldr2qtimezone.py for details.

.. _CLDR: ftp://unicode.org/Public/cldr/
"""

import os
import sys

from localetools import Error
from cldr import CldrReader
from qlocalexml import QLocaleXmlWriter
from enumdata import language_list, script_list, country_list

def usage(name, err, message = ''):
    err.write("""Usage: {} path/to/cldr/common/main [out-file.xml]
""".format(name)) # TODO: expand command-line, improve help message
    if message:
        err.write('\n' + message + '\n')

def main(args, out, err):
    # TODO: make calendars a command-line option
    calendars = ['gregorian', 'persian', 'islamic'] # 'hebrew'

    # TODO: make argument parsing more sophisticated
    name = args.pop(0)
    if not args:
        usage(name, err, 'Where is your CLDR data tree ?')
        return 1

    root = args.pop(0)
    if not os.path.exists(os.path.join(root, 'common', 'main', 'root.xml')):
        usage(name, err,
              'First argument is the root of the CLDR tree: found no common/main/root.xml under '
              + root)
        return 1

    xml = args.pop(0) if args else None
    if not xml or xml == '-':
        emit = out
    elif not xml.endswith('.xml'):
        usage(name, err, 'Please use a .xml extension on your output file name, not ' + xml)
        return 1
    else:
        try:
            emit = open(xml, 'w')
        except IOError as e:
            usage(name, err, 'Failed to open "{}" to write output to it\n'.format(xml))
            return 1

    if args:
        usage(name, err, 'Too many arguments - excess: ' + ' '.join(args))
        return 1

    if emit.encoding != 'UTF-8' or (emit.encoding is None and sys.getdefaultencoding() != 'UTF-8'):
        reload(sys) # Weirdly, this gets a richer sys module than the plain import got us !
        sys.setdefaultencoding('UTF-8')

    # TODO - command line options to tune choice of grumble and whitter:
    reader = CldrReader(root, err.write, err.write)
    writer = QLocaleXmlWriter(emit.write)

    writer.version(reader.root.cldrVersion)
    writer.enumData(language_list, script_list, country_list)
    writer.likelySubTags(reader.likelySubTags())
    writer.locales(reader.readLocales(calendars), calendars)

    writer.close()
    return 0

if __name__ == '__main__':
    sys.exit(main(sys.argv, sys.stdout, sys.stderr))
