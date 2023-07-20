#! /bin/sh

# Copyright (C) 2017 André Klitzing
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#
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
   jcdiffct.c
   jchuff.c
   jchuff.h
   jcicc.c
   jcinit.c
   jclhuff.c
   jclossls.c
   jcmainct.c
   jcmarker.c
   jcmaster.c
   jcmaster.h
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
   jddiffct.c
   jdhuff.c
   jdhuff.h
   jdicc.c
   jdphuff.c
   jdinput.c
   jdlhuff.c
   jdlossls.c
   jlossls.h
   jdmainct.c
   jdmainct.h
   jdmarker.c
   jdmaster.c
   jdmaster.h
   jdmerge.c
   jdmerge.h
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
   jpegapicomp.h
   jpegint.h
   jpeglib.h
   jmemmgr.c
   jmemnobs.c
   jmemsys.h
   jmorecfg.h
   jpeg_nbits_table.h
   jquant1.c
   jquant2.c
   jsamplecomp.h
   jsimd.h
   jsimddct.h
   jstdhuff.c
   jutils.c
"

for i in $FILES; do
    copy_file "$i" "src/$i"
done
copy_file "jversion.h.in" "src/jversion.h"

cyear=$(grep COPYRIGHT_YEAR $LIBJPEG_DIR/CMakeLists.txt | sed -e 's/.*"\(.*\)".*/\1/')
sed -i -e "s/@COPYRIGHT_YEAR@/$cyear/" $TARGET_DIR/src/jversion.h

sed -n -e 's/^[ ]*"//
           s/\(\\n\)*"[ ]*\\*$//
           /JCOPYRIGHT\ /,/^[ ]*$/ {
               /Copyright/p
           }
          ' $TARGET_DIR/src/jversion.h > $TARGET_DIR/COPYRIGHT.txt


echo Done. $TARGET_DIR/src/jconfig.h and jconfigint.h may need manual updating.
