#! /bin/sh

# Copyright (C) 2021 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#
# This is a small script to copy the required files from a harfbuzz tarball
# into 3rdparty/harfbuzz-ng/ . Documentation, tests, demos etc. are not imported.
# Steps:
# 1. rm $QTDIR/src/3rdparty/harfbuzz-ng/src/* && rm $QTDIR/src/3rdparty/harfbuzz-ng/src/OT/Layout/GSUB
# 2. source import_from_tarball.sh harfbuzz_tarball_dir/ $QTDIR/src/3rdparty/harfbuzz-ng/
# 3. Check that CMakeLists contains everything

if [ $# -ne 2 ]; then
    echo "Usage: $0 harfbuzz_tarball_dir/ \$QTDIR/src/3rdparty/harfbuzz-ng/"
    exit 1
fi

HB_DIR=$1
TARGET_DIR=$2

if [ ! -d "$HB_DIR" -o ! -r "$HB_DIR" -o ! -d "$TARGET_DIR" -o ! -w "$TARGET_DIR" ]; then
    echo "Either the harfbuzz source dir or the target dir do not exist,"
    echo "are not directories or have the wrong permissions."
    exit 2
fi

# with 1 argument, copies HB_DIR/$1 to TARGET_DIR/$1
# with 2 arguments, copies HB_DIR/$1 to TARGET_DIR/$2
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
    cp -R "$HB_DIR/$SOURCE_FILE" "$TARGET_DIR/$DEST_FILE"
}

FILES=(AUTHORS
       COPYING
       README.md
       THANKS
       )

for i in ${FILES[*]}; do
    copy_file_or_dir "$i"
done

cp -R $HB_DIR/src $TARGET_DIR
