#! /bin/sh
#############################################################################
##
## Copyright (C) 2016 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Giuseppe D'Angelo <giuseppe.dangelo@kdab.com>
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

# This is a small script to copy the required files from a PCRE2 tarball
# into 3rdparty/pcre2/ , following the instructions found in the NON-AUTOTOOLS-BUILD
# file. Documentation, tests, demos etc. are not imported.

if [ $# -ne 2 ]; then
    echo "Usage: $0 pcre2_tarball_dir/ \$QTDIR/src/3rdparty/pcre2/"
    exit 1
fi

PCRE2_DIR=$1
TARGET_DIR=$2

if [ ! -d "$PCRE2_DIR" -o ! -r "$PCRE2_DIR" -o ! -d "$TARGET_DIR" -o ! -w "$TARGET_DIR" ]; then
    echo "Either the PCRE2 source dir or the target dir do not exist,"
    echo "are not directories or have the wrong permissions."
    exit 2
fi

# with 1 argument, copies PCRE2_DIR/$1 to TARGET_DIR/$1
# with 2 arguments, copies PCRE2_DIR/$1 to TARGET_DIR/$2
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
    cp "$PCRE2_DIR/$SOURCE_FILE" "$TARGET_DIR/$DEST_FILE"
}

copy_file "src/pcre2.h.generic" "src/pcre2.h"
copy_file "src/pcre2_chartables.c.dist" "src/pcre2_chartables.c"

FILES="
    AUTHORS
    LICENCE

    src/pcre2_auto_possess.c
    src/pcre2_compile.c
    src/pcre2_config.c
    src/pcre2_context.c
    src/pcre2_dfa_match.c
    src/pcre2_error.c
    src/pcre2_extuni.c
    src/pcre2_find_bracket.c
    src/pcre2_internal.h
    src/pcre2_intmodedep.h
    src/pcre2_jit_compile.c
    src/pcre2_jit_match.c
    src/pcre2_jit_misc.c
    src/pcre2_maketables.c
    src/pcre2_match.c
    src/pcre2_match_data.c
    src/pcre2_newline.c
    src/pcre2_ord2utf.c
    src/pcre2_pattern_info.c
    src/pcre2_serialize.c
    src/pcre2_string_utils.c
    src/pcre2_study.c
    src/pcre2_substitute.c
    src/pcre2_substring.c
    src/pcre2_tables.c
    src/pcre2_ucd.c
    src/pcre2_ucp.h
    src/pcre2_valid_utf.c
    src/pcre2_xclass.c
    src/sljit/sljitConfig.h
    src/sljit/sljitConfigInternal.h
    src/sljit/sljitExecAllocator.c
    src/sljit/sljitLir.c
    src/sljit/sljitLir.h
    src/sljit/sljitNativeARM_32.c
    src/sljit/sljitNativeARM_64.c
    src/sljit/sljitNativeARM_T2_32.c
    src/sljit/sljitNativeMIPS_32.c
    src/sljit/sljitNativeMIPS_64.c
    src/sljit/sljitNativeMIPS_common.c
    src/sljit/sljitNativePPC_32.c
    src/sljit/sljitNativePPC_64.c
    src/sljit/sljitNativePPC_common.c
    src/sljit/sljitNativeSPARC_32.c
    src/sljit/sljitNativeSPARC_common.c
    src/sljit/sljitNativeTILEGX_64.c
    src/sljit/sljitNativeTILEGX-encoder.c
    src/sljit/sljitNativeX86_32.c
    src/sljit/sljitNativeX86_64.c
    src/sljit/sljitNativeX86_common.c
    src/sljit/sljitUtils.c
"

for i in $FILES; do
    copy_file "$i"
done
