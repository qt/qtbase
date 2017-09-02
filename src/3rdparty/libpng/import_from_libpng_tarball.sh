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

# This is a small script to copy the required files from a libpng tarball
# into 3rdparty/libpng/

if [ $# -ne 2 ]; then
    echo "Usage: $0 libpng_tarball_dir/ \$QTDIR/src/3rdparty/libpng/"
    exit 1
fi

LIBPNG_DIR=$1
TARGET_DIR=$2

if [ ! -d "$LIBPNG_DIR" -o ! -r "$LIBPNG_DIR" -o ! -d "$TARGET_DIR" -o ! -w "$TARGET_DIR" ]; then
    echo "Either the libpng source dir or the target dir do not exist,"
    echo "are not directories or have the wrong permissions."
    exit 2
fi

# with 1 argument, copies LIBPNG_DIR/$1 to TARGET_DIR/$1
# with 2 arguments, copies LIBPNG_DIR/$1 to TARGET_DIR/$2
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
    cp "$LIBPNG_DIR/$SOURCE_FILE" "$TARGET_DIR/$DEST_FILE"
}

copy_file "scripts/pnglibconf.h.prebuilt" "pnglibconf.h"

FILES="
   ANNOUNCE
   README
   CHANGES
   LICENSE
   INSTALL
   libpng-manual.txt

   png.c
   pngerror.c
   pngget.c
   pngmem.c
   pngpread.c
   pngread.c
   pngrio.c
   pngrtran.c
   pngrutil.c
   pngset.c
   pngtrans.c
   pngwio.c
   pngwrite.c
   pngwtran.c
   pngwutil.c

   png.h
   pngpriv.h
   pngstruct.h
   pnginfo.h
   pngconf.h
   pngdebug.h
"

for i in $FILES; do
    copy_file "$i" "$i"
done
