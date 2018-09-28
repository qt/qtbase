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

# This is a small script to copy the required files from a LIBJPEG tarball
# into 3rdparty/libjpeg/.

if [ $# -ne 2 ]; then
    echo "Usage: $0 LIBJPEG_tarball_dir/ \$QTDIR/src/3rdparty/libjpeg/"
    exit 1
fi

LIBJPEG_DIR=$1
TARGET_DIR=$2

if [ ! -d "$LIBJPEG_DIR" -o ! -r "$LIBJPEG_DIR" -o ! -d "$TARGET_DIR" -o ! -w "$TARGET_DIR" ]; then
    echo "Either the LIBJPEG source dir or the target dir do not exist,"
    echo "are not directories or have the wrong permissions."
    exit 2
fi

# with 1 argument, copies LIBJPEG_DIR/$1 to TARGET_DIR/$1
# with 2 arguments, copies LIBJPEG_DIR/$1 to TARGET_DIR/$2
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
    cp "$LIBJPEG_DIR/$SOURCE_FILE" "$TARGET_DIR/$DEST_FILE"
}

copy_file "LICENSE.md" "LICENSE"

FILES="
   change.log
   ChangeLog.md
   README.md
   README.ijg
   jconfig.h.in
   jconfigint.h.in

   jaricom.c
   jcapimin.c
   jcapistd.c
   jcarith.c
   jccoefct.c
   jccolext.c
   jccolor.c
   jcdctmgr.c
   jchuff.c
   jchuff.h
   jcinit.c
   jcmainct.c
   jcmarker.c
   jcmaster.c
   jcomapi.c
   jcparam.c
   jcphuff.c
   jcprepct.c
   jcsample.c
   jctrans.c
   jdapimin.c
   jdapistd.c
   jdarith.c
   jdatadst.c
   jdatasrc.c
   jdcoefct.c
   jdcoefct.h
   jdcolext.c
   jdcol565.c
   jdcolor.c
   jdct.h
   jddctmgr.c
   jdhuff.c
   jdhuff.h
   jdphuff.c
   jdinput.c
   jdmainct.c
   jdmainct.h
   jdmarker.c
   jdmaster.c
   jdmaster.h
   jdmerge.c
   jdmrgext.c
   jdmrg565.c
   jdpostct.c
   jdsample.c
   jdsample.h
   jdtrans.c
   jerror.c
   jerror.h
   jfdctflt.c
   jfdctfst.c
   jfdctint.c
   jidctred.c
   jidctflt.c
   jidctfst.c
   jidctint.c
   jinclude.h
   jpegcomp.h
   jpegint.h
   jpeglib.h
   jmemmgr.c
   jmemnobs.c
   jmemsys.h
   jmorecfg.h
   jpeg_nbits_table.h
   jquant1.c
   jquant2.c
   jsimd.h
   jsimd_none.c
   jsimddct.h
   jstdhuff.c
   jutils.c
   jversion.h
"

for i in $FILES; do
    copy_file "$i" "src/$i"
done

echo Done. $TARGET_DIR/jconfig.h and jconfigint.h may need manual updating.
