#! /bin/sh
#############################################################################
##
## Copyright (C) 2012 Giuseppe D'Angelo <dangelog@gmail.com>.
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

# This is a small script to copy the required files from a PCRE tarball
# into 3rdparty/pcre/ , following the instructions found in the NON-UNIX-USE
# file. Documentation, tests, demos etc. are not imported.

if [ $# -ne 2 ]; then
    echo "Usage: $0 pcre_tarball_dir/ \$QTDIR/src/3rdparty/pcre/"
    exit 1
fi

PCRE_DIR=$1
TARGET_DIR=$2

if [ ! -d "$PCRE_DIR" -o ! -r "$PCRE_DIR" -o ! -d "$TARGET_DIR" -o ! -w "$TARGET_DIR" ]; then
    echo "Either the PCRE source dir or the target dir do not exist,"
    echo "are not directories or have the wrong permissions."
    exit 2
fi

# with 1 argument, copies PCRE_DIR/$1 to TARGET_DIR/$1
# with 2 arguments, copies PCRE_DIR/$1 to TARGET_DIR/$2
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
    cp "$PCRE_DIR/$SOURCE_FILE" "$TARGET_DIR/$DEST_FILE"
}

copy_file "pcre.h.generic" "pcre.h"
copy_file "pcre_chartables.c.dist" "pcre_chartables.c"

FILES="
    AUTHORS
    COPYING
    LICENCE
    pcre_internal.h
    ucp.h
    pcre_byte_order.c
    pcre_compile.c
    pcre_config.c
    pcre_dfa_exec.c
    pcre_exec.c
    pcre_fullinfo.c
    pcre_get.c
    pcre_globals.c
    pcre_jit_compile.c
    pcre_maketables.c
    pcre_newline.c
    pcre_ord2utf8.c
    pcre_refcount.c
    pcre_string_utils.c
    pcre_study.c
    pcre_tables.c
    pcre_ucd.c
    pcre_valid_utf8.c
    pcre_version.c
    pcre_xclass.c
    pcre16_byte_order.c
    pcre16_chartables.c
    pcre16_compile.c
    pcre16_config.c
    pcre16_dfa_exec.c
    pcre16_exec.c
    pcre16_fullinfo.c
    pcre16_get.c
    pcre16_globals.c
    pcre16_jit_compile.c
    pcre16_maketables.c
    pcre16_newline.c
    pcre16_ord2utf16.c
    pcre16_refcount.c
    pcre16_string_utils.c
    pcre16_study.c
    pcre16_tables.c
    pcre16_ucd.c
    pcre16_utf16_utils.c
    pcre16_valid_utf16.c
    pcre16_version.c
    pcre16_xclass.c
    sljit/sljitConfig.h
    sljit/sljitConfigInternal.h
    sljit/sljitExecAllocator.c
    sljit/sljitLir.c
    sljit/sljitLir.h
    sljit/sljitNativeARM_32.c
    sljit/sljitNativeARM_64.c
    sljit/sljitNativeARM_T2_32.c
    sljit/sljitNativeMIPS_32.c
    sljit/sljitNativeMIPS_64.c
    sljit/sljitNativeMIPS_common.c
    sljit/sljitNativePPC_32.c
    sljit/sljitNativePPC_64.c
    sljit/sljitNativePPC_common.c
    sljit/sljitNativeSPARC_32.c
    sljit/sljitNativeSPARC_common.c
    sljit/sljitNativeTILEGX_64.c
    sljit/sljitNativeTILEGX-encoder.c
    sljit/sljitNativeX86_32.c
    sljit/sljitNativeX86_64.c
    sljit/sljitNativeX86_common.c
    sljit/sljitUtils.c
"

for i in $FILES; do
    copy_file "$i"
done
