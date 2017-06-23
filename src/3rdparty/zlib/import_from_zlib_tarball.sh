#! /bin/sh
#############################################################################
##
## Copyright (C) 2017 André Klitzing
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

# This is a small script to copy the required files from a zlib tarball
# into 3rdparty/zlib/

if [ $# -ne 2 ]; then
    echo "Usage: $0 zlib_tarball_dir/ \$QTDIR/src/3rdparty/zlib/"
    exit 1
fi

ZLIB_DIR=$1
TARGET_DIR=$2

if [ ! -d "$ZLIB_DIR" -o ! -r "$ZLIB_DIR" -o ! -d "$TARGET_DIR" -o ! -w "$TARGET_DIR" ]; then
    echo "Either the zlib source dir or the target dir do not exist,"
    echo "are not directories or have the wrong permissions."
    exit 2
fi

# with 1 argument, copies ZLIB_DIR/$1 to TARGET_DIR/$1
# with 2 arguments, copies ZLIB_DIR/$1 to TARGET_DIR/$2
copy_file() {
    if [ $# -lt 1 -o $# -gt 2  ]; then
        echo "Wrong number of arguments to copy_file"
        exit 3
    fi

    SOURCE_FILE=$1
    if [ -n "$2" ]; then
        DEST_FILE=$2
    else
        DEST_FILE=$1
    fi

    mkdir -p "$TARGET_DIR/$(dirname "$SOURCE_FILE")"
    cp "$ZLIB_DIR/$SOURCE_FILE" "$TARGET_DIR/$DEST_FILE"
}

FILES="
   README
   ChangeLog

   adler32.c
   compress.c
   crc32.c
   crc32.h
   deflate.c
   deflate.h
   gzclose.c
   gzguts.h
   gzlib.c
   gzread.c
   gzwrite.c
   infback.c
   inffast.c
   inffast.h
   inffixed.h
   inflate.c
   inflate.h
   inftrees.c
   inftrees.h
   trees.c
   trees.h
   uncompr.c
   zconf.h
   zlib.h
   zutil.c
   zutil.h

"

for i in $FILES; do
    copy_file "$i" "src/$i"
done
