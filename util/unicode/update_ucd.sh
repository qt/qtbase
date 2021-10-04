#! /bin/sh
#############################################################################
##
## Copyright (C) 2021 The Qt Company Ltd.
## Contact: https://www.qt.io/licensing/
##
## This file is the build configuration utility of the Qt Toolkit.
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

# This script downloads UCD files for an Unicode release and updates the
# copies used by Qt. It expects the new Unicode version as argument:
#
#      $ ./update_ucd.sh 14.0.0

set -e

if [ "$#" -ne 1 ]
then
    echo "Usage: $0 <UNICODE-VERSION>" >&2
    exit 1
fi

VERSION="$1"

qtbase=$(realpath "$(dirname "$0")"/../..)
tmp=$(mktemp)

download()
{
    wget -nv -P "$tmp" "$1"
}

download "https://www.unicode.org/Public/zipped/$VERSION/UCD.zip"
unzip -q "$tmp/UCD.zip" -d "$tmp"
download "https://www.unicode.org/Public/idna/$VERSION/IdnaMappingTable.txt"
download "https://www.unicode.org/Public/idna/$VERSION/IdnaTestV2.txt"

data_dirs="util/unicode/data \
tests/auto/corelib/io/qurluts46/testdata \
tests/auto/corelib/text/qtextboundaryfinder/data"

for dir in $data_dirs
do
    find "$qtbase/$dir" -name '*.txt' -o -name '*.html'
done | grep -vw 'ReadMe.*\.txt' | while read -r file
do
    base_name=$(basename "$file")
    echo "Updating ${base_name}"
    full_name=$(find "$tmp" -name "$base_name" -print -quit)
    if [ "$full_name" = "" ]
    then
        echo "No source file for: ${base_name}" >&2
        exit 1
    fi
    cp "$full_name" "$file"
done

rm -rf "$tmp"
