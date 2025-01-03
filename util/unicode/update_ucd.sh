#! /bin/sh
# Copyright (C) 2021 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
