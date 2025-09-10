#!/bin/bash
# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

VERBOSE=0
CONTINUE_ON_FAILURE=1

function warn() {
    echo "$@" >&2
}

function log() {
    [[ "$VERBOSE" -eq 1 ]] && warn "$@"
}

function die() {
    warn "$@"
    exit 1
}

function continue_or_die() {
    if [[ "$CONTINUE_ON_FAILURE" -eq 1 ]] ; then
        warn "$@"
    else
        die "$@"
    fi
}

function usage() {
    die "usage: includemocs6.sh <srcdir> <builddir> <mocs_compilation_file>"
}

#
# sanity checks:
#

[[ ${#@} -eq 3 ]] || usage

SRCDIR="$1"
BUILDDIR="$2"
MOCS_COMPILATION_FILE="$3"

[[ -d "$BUILDDIR" ]] || die "Build dir \"$BUILDDIR\" doesn't exist or isn't a directory."
[[ -d "$SRCDIR"  ]] || die "Source dir \"SRCDIR\" doesn't exist or isn't a directory."
[[ -f "$MOCS_COMPILATION_FILE" ]] || die "MOCs-compilation file \"$MOCS_COMPILATION_FILE\" doesn't exist or isn't a file."

#
# extract the files included by the mocs_compilation.cpp
#

# lines look like this:
#   #include "SOFJASFD/moc_foo.cpp"
#   #include "JFEJGKKS/moc_foo_p.cpp"
# extracting moc_foo(_p).cpp:
grep -E '^#include' "$MOCS_COMPILATION_FILE" | cut -d\" -f2 | cut -d/ -f2 | while read MOCFILE; do
    log "MOCFILE=$MOCFILE"
    C1="${MOCFILE##moc_}"              # foo.cpp or foo_p.cpp or foo_p_p.cpp
    case "$MOCFILE" in
        moc_*_p_p.cpp)
            C2="${C1/_p.cpp/.cpp}"     # foo_p.cpp
            C3="${C2/_p.cpp/.cpp}"     # foo.cpp
            CANDIDATES=("$C1" "$C2" "$C3")
            ;;
        moc_*_p.cpp)
            C2="${C1/_p.cpp/.cpp}"     # foo.cpp
            CANDIDATES=("$C1" "$C2")
            ;;
        moc_*.cpp)
            CANDIDATES=("$C1")
            ;;
        *)
            die "Don't know how to handle moc-file \"$MOCFILE\"..."
            ;;
    esac
    log "CANDIDATES=(${CANDIDATES[@]})"
    for CANDIDATE in "${CANDIDATES[@]}"; do
        CPPFILE="$(find "$SRCDIR" -name "$CANDIDATE")"
        log "CPPFILE=$CPPFILE"
        [[ -f "$CPPFILE" ]] && break
    done
    if [[ -f "$CPPFILE" ]] ; then
        log "going to include $MOCFILE into $CPPFILE"
        if ! grep -qE '^QT_END_NAMESPACE$' "$CPPFILE"; then
            warn "Can't find QT_END_NAMESPACE in \"$CPPFILE\", simply appending the #include. Please check placement manually."
            echo "#include \"$MOCFILE\"" >> "$CPPFILE" || die "Failed to write to \"$CPPFILE\""
        else
            sed -i -e "1,/QT_END_NAMESPACE/ s/QT_END_NAMESPACE/QT_END_NAMESPACE\n\n#include \"$MOCFILE\"/" "$CPPFILE" || die "Failed to includemoc \"$MOCFILE\" into \"$CPPFILE\"."
        fi
    else
        continue_or_die "Can't find a cpp file for $MOCFILE (not looking for .cxx .cc etc)."
    fi
done

exit 0
