#! /bin/sh

#############################################################################
##
## Copyright (C) 2021 The Qt Company Ltd.
## Contact: https://www.qt.io/licensing/
##
## This file is the build configuration utility of the Qt Toolkit.
##
## $QT_BEGIN_LICENSE:LGPL$
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and The Qt Company. For licensing terms
## and conditions see https://www.qt.io/terms-conditions. For further
## information use the contact form at https://www.qt.io/contact-us.
##
## GNU Lesser General Public License Usage
## Alternatively, this file may be used under the terms of the GNU Lesser
## General Public License version 3 as published by the Free Software
## Foundation and appearing in the file LICENSE.LGPL3 included in the
## packaging of this file. Please review the following information to
## ensure the GNU Lesser General Public License version 3 requirements
## will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
##
## GNU General Public License Usage
## Alternatively, this file may be used under the terms of the GNU
## General Public License version 2.0 or (at your option) the GNU General
## Public license version 3 or any later version approved by the KDE Free
## Qt Foundation. The licenses are as published by the Free Software
## Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
## included in the packaging of this file. Please review the following
## information to ensure the GNU General Public License requirements will
## be met: https://www.gnu.org/licenses/gpl-2.0.html and
## https://www.gnu.org/licenses/gpl-3.0.html.
##
## $QT_END_LICENSE$
##
#############################################################################

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
       NEWS
       README.md
       THANKS
       )

for i in ${FILES[*]}; do
    copy_file_or_dir "$i"
done

CODEFILES=($HB_DIR/src/*.cc
           $HB_DIR/src/*.hh
           $HB_DIR/src/*.h)

for i in ${CODEFILES[*]}; do
    cp $i $TARGET_DIR/src/
done

GSUBFILES=($HB_DIR/src/OT/Layout/GSUB/*.hh)

for i in ${GSUBFILES[*]}; do
    cp $i $TARGET_DIR/src/OT/Layout/GSUB/
done
