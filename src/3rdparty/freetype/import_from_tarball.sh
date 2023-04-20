#! /bin/sh

# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#
# This is a small script to copy the required files from a freetype tarball
# into 3rdparty/freetype/ . Documentation, tests, demos etc. are not imported.

if [ $# -ne 2 ]; then
    echo "Usage: $0 freetype_tarball_dir/ \$QTDIR/src/3rdparty/freetype/"
    exit 1
fi

FT_DIR=$1
TARGET_DIR=$2

if [ ! -d "$FT_DIR" -o ! -r "$FT_DIR" -o ! -d "$TARGET_DIR" -o ! -w "$TARGET_DIR" ]; then
    echo "Either the freetype source dir or the target dir do not exist,"
    echo "are not directories or have the wrong permissions."
    exit 2
fi

# with 1 argument, copies FT_DIR/$1 to TARGET_DIR/$1
# with 2 arguments, copies FT_DIR/$1 to TARGET_DIR/$2
copy_file_or_dir() {
    if [ $# -lt 1 -o $# -gt 2  ]; then
        echo "Wrong number of arguments to copy_file_or_dir"
        exit 3
    fi

    SOURCE_FILE=$1
    if [ -n "$2" ]; then
        DEST_FILE=$2
    else
        DEST_FILE=$1
    fi

    mkdir -p "$TARGET_DIR/$(dirname "$SOURCE_FILE")"
    cp -R "$FT_DIR/$SOURCE_FILE" "$TARGET_DIR/$DEST_FILE"
}

FILES="
    README
    builds/unix/ftsystem.c
    builds/windows/ftdebug.c
    docs/CHANGES
    docs/CUSTOMIZE
    docs/DEBUG
    docs/PROBLEMS
    docs/TODO
    docs/FTL.TXT
    docs/GPLv2.TXT
    include/
    src/
"

for i in $FILES; do
    copy_file_or_dir "$i"
done
