#!/usr/bin/env python3
# Copyright (C) 2021 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
"""Convert CLDR data to QLocaleXML

The CLDR data can be downloaded as a zip-file from CLDR_, which has a
sub-directory for each version; you need the ``core.zip`` file for
your version of choice (typically the latest), which you should then
unpack. Alternatively, you can clone the git repo from github_, which
has a tag for each release and a maint/maint-$ver branch for each
major version. Either way, the CLDR top-level directory should have a
subdirectory called common/ which contains (among other things)
subdirectories main/ and supplemental/.

This script has had updates to cope up to v44.1; for later versions,
we may need adaptations. Pass the path of the CLDR top-level directory
to this script as its first command-line argument. Pass the name of
the file in which to write output as the second argument; either omit
it or use '-' to select the standard output. This file is the input
needed by ``./qlocalexml2cpp.py``

When you update the CLDR data, be sure to also update
src/corelib/text/qt_attribution.json's entry for unicode-cldr. Check
this script's output for unknown language, territory or script messages;
if any can be resolved, use their entry in common/main/en.xml to
append new entries to enumdata.py's lists and update documentation in
src/corelib/text/qlocale.qdoc, adding the new entries in alphabetic
order.

Both of the scripts mentioned support --help to tell you how to use
them.

.. _CLDR: https://unicode.org/Public/cldr/
.. _github: https://github.com/unicode-org/cldr
"""

from pathlib import Path
import argparse

from cldr import CldrReader
from typing import TextIO
from qlocalexml import QLocaleXmlWriter


def main(argv: list[str], out: TextIO, err: TextIO) -> int:
    """Generate a QLocaleXML file from CLDR data.

    Takes sys.argv, sys.stdout, sys.stderr (or equivalents) as
    arguments. In argv[1:], it expects the root of the CLDR data
    directory as first parameter and the name of the file in which to
    save QLocaleXML data as second parameter. It accepts a --calendars
    option to select which calendars to support (all available by
    default)."""
    all_calendars = ['gregorian', 'persian', 'islamic']

    parser = argparse.ArgumentParser(
        prog=Path(argv[0]).name,
        description='Generate QLocaleXML from CLDR data.',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('cldr_path', help='path to the root of the CLDR tree')
    parser.add_argument('out_file', help='output XML file name',
                        nargs='?', metavar='out-file.xml')
    parser.add_argument('--calendars', help='select calendars to emit data for',
                        nargs='+', metavar='CALENDAR',
                        choices=all_calendars, default=all_calendars)
    parser.add_argument('-v', '--verbose', help='more verbose output',
                        action='count', default=0)
    parser.add_argument('-q', '--quiet', help='less output',
                        dest='verbose', action='store_const', const=-1)
    args = parser.parse_args(argv[1:])

    root = Path(args.cldr_path)
    root_xml_path = 'common/main/root.xml'

    if not root.joinpath(root_xml_path).exists():
        parser.error('First argument is the root of the CLDR tree: '
                     f'found no {root_xml_path} under {root}')

    xml = args.out_file
    if not xml or xml == '-':
        emit = out
    elif not xml.endswith('.xml'):
        parser.error(f'Please use a .xml extension on your output file name, not {xml}')
    else:
        try:
            emit = open(xml, 'w', encoding="utf-8")
        except IOError as e:
            parser.error(f'Failed to open "{xml}" to write output to it')

    reader = CldrReader(root,
                        (lambda *x: 0) if args.verbose < 0 else
                        # Use stderr for logging if stdout is where our XML is going:
                        err.write if out is emit else out.write,
                        err.write)
    writer = QLocaleXmlWriter(reader.root.cldrVersion, emit.write)

    writer.enumData(reader.root.englishNaming)
    writer.likelySubTags(reader.likelySubTags())
    writer.zoneData(*reader.zoneData()) # Locale-independent zone data.
    en_US = tuple(id for id, name in reader.root.codesToIdName('en', '', 'US'))
    writer.locales(reader.readLocales(args.calendars), args.calendars, en_US)

    writer.close(err.write)
    return 0

if __name__ == '__main__':
    import sys
    sys.exit(main(sys.argv, sys.stdout, sys.stderr))
