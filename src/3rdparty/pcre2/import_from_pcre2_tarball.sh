#! /bin/sh

# Copyright (C) 2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Giuseppe D'Angelo <giuseppe.dangelo@kdab.com>
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#
# This is a small script to copy the required files from a PCRE2 tarball
# into 3rdparty/pcre2/ , following the instructions found in the NON-AUTOTOOLS-BUILD
# file. Documentation, tests, demos etc. are not imported.

set -e

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
    AUTHORS.md
    LICENCE.md

    src/pcre2_auto_possess.c
    src/pcre2_chkdint.c
    src/pcre2_compile.c
    src/pcre2_compile.h
    src/pcre2_compile_class.c
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
    src/pcre2_script_run.c
    src/pcre2_serialize.c
    src/pcre2_jit_char_inc.h
    src/pcre2_jit_neon_inc.h
    src/pcre2_jit_simd_inc.h
    src/pcre2_string_utils.c
    src/pcre2_study.c
    src/pcre2_substitute.c
    src/pcre2_substring.c
    src/pcre2_tables.c
    src/pcre2_ucd.c
    src/pcre2_ucp.h
    src/pcre2_ucptables.c
    src/pcre2_util.h
    src/pcre2_valid_utf.c
    src/pcre2_xclass.c
    deps/sljit/sljit_src/sljitConfigCPU.h
    deps/sljit/sljit_src/sljitConfig.h
    deps/sljit/sljit_src/sljitConfigInternal.h
    deps/sljit/sljit_src/sljitLir.c
    deps/sljit/sljit_src/sljitLir.h
    deps/sljit/sljit_src/sljitNativeARM_32.c
    deps/sljit/sljit_src/sljitNativeARM_64.c
    deps/sljit/sljit_src/sljitNativeARM_T2_32.c
    deps/sljit/sljit_src/sljitNativeLOONGARCH_64.c
    deps/sljit/sljit_src/sljitNativeMIPS_32.c
    deps/sljit/sljit_src/sljitNativeMIPS_64.c
    deps/sljit/sljit_src/sljitNativeMIPS_common.c
    deps/sljit/sljit_src/sljitNativePPC_32.c
    deps/sljit/sljit_src/sljitNativePPC_64.c
    deps/sljit/sljit_src/sljitNativePPC_common.c
    deps/sljit/sljit_src/sljitNativeRISCV_32.c
    deps/sljit/sljit_src/sljitNativeRISCV_64.c
    deps/sljit/sljit_src/sljitNativeRISCV_common.c
    deps/sljit/sljit_src/sljitNativeS390X.c
    deps/sljit/sljit_src/sljitNativeX86_32.c
    deps/sljit/sljit_src/sljitNativeX86_64.c
    deps/sljit/sljit_src/sljitNativeX86_common.c
    deps/sljit/sljit_src/sljitSerialize.c
    deps/sljit/sljit_src/sljitUtils.c
    deps/sljit/sljit_src/allocator_src/sljitExecAllocatorPosix.c
    deps/sljit/sljit_src/allocator_src/sljitProtExecAllocatorPosix.c
    deps/sljit/sljit_src/allocator_src/sljitWXExecAllocatorPosix.c
    deps/sljit/sljit_src/allocator_src/sljitProtExecAllocatorNetBSD.c
    deps/sljit/sljit_src/allocator_src/sljitExecAllocatorWindows.c
    deps/sljit/sljit_src/allocator_src/sljitExecAllocatorFreeBSD.c
    deps/sljit/sljit_src/allocator_src/sljitExecAllocatorApple.c
    deps/sljit/sljit_src/allocator_src/sljitWXExecAllocatorWindows.c
    deps/sljit/sljit_src/allocator_src/sljitExecAllocatorCore.c
"

for i in $FILES; do
    copy_file "$i"
done
