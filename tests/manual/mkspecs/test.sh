#!/bin/bash
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


if [ "$1" == "--help" ]; then
    echo "Init a clean git repository somewhere and run this test script from that directory. The first run will"
    echo "produce a bunch of specs. This is your baseline. Run 'git add specs' and commit the baseline. Then run"
    echo "this script again, after making changes to the mkspecs. You should see any diffs you produced."
    exit 0
fi


QMAKE_ARGS="-nocache -d"
SPECS_DIR=$(qmake -query QMAKE_MKSPECS)
SPECS=$(find -L $SPECS_DIR | grep "qmake.conf" | grep -Ev "common|default" | grep "$1")

SEDI="sed -i"
if [ $(uname) == "Darwin" ]; then
    # Mac OS X requires an extension, Linux will barf on it being present
    SEDI='sed -i .backup'
fi

if [ ! -d tmp ]; then
    mkdir tmp
    touch tmp/empty.pro
fi

if [ ! -d specs ]; then
    mkdir specs
fi

git checkout -- specs > /dev/null 2>&1

cd tmp
for spec in $SPECS; do
    spec=$(echo $spec | sed "s|$SPECS_DIR/||" | sed "s|/qmake.conf||")
    output_file=$(echo "$spec.txt" | sed "s|/|-|g")
    echo "Dumping qmake variables for spec '$spec' to 'specs/$output_file'..."
    qmake $QMAKE_ARGS -spec $spec empty.pro 2>&1 |
        sed -n '/Dumping all variables/,$p' |
        grep -Ev "(QMAKE_INTERNAL_INCLUDED_FILES|DISTFILES) ===" > ../specs/$output_file

    if [ -n $QTDIR ]; then
        $SEDI "s|$QTDIR|\$QTDIR|g" ../specs/$output_file
    fi

    if [ -n $QTSRCDIR ]; then
        $SEDI "s|$QTSRCDIR|\$QTDIR|g" ../specs/$output_file
    fi
done
cd ..

rm -f specs/*.backup

git diff --exit-code -- specs > /dev/null
exit_code=$?

if [ $exit_code -eq 0 ]; then
    echo -e "\nNo diff produced (you did good)"
else
    # Show the resulting diff
    git diff -- specs
fi

exit $exit_code

