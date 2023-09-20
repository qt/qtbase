#!/bin/bash
# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

PICK_TO_BRANCHES="6.6 6.5 6.2 5.15"
#UPSTREAM=github.com:publicsuffix/list.git          # use this if you have a github account
UPSTREAM=https://github.com/publicsuffix/list.git  # and this if you don't

THIS="util/update_public_suffix_list.sh"
PUBLIC_SUFFIX_LIST_DAT_DIR="$1"
MAKE_DAFSA=src/3rdparty/libpsl/src/psl-make-dafsa
PSL_DATA_CPP=src/3rdparty/libpsl/psl_data.cpp
PUBLIC_SUFFIX_LIST_DAFSA=tests/auto/network/access/qnetworkcookiejar/testdata/publicsuffix/public_suffix_list.dafsa
ATTRIBUTION_JSON=src/3rdparty/libpsl/qt_attribution.json

function msg() {
    echo "$@" 1>&2
}

function die() {
    msg "$@"
    exit 1
}

function check_or_die() {
    TYPE=$1
    OP=$2
    FILE="$3"
    test $OP "$FILE" || die "$TYPE \"$FILE\" not found (test $OP \"$FILE\" failed). Please run $THIS from \$SRCDIR/qtbase and pass the directory containing a checkout of $UPSTEAM on the command line."
}

function run_or_die() {
    msg -n "Running \"$@\"..."
    "$@" || die "Failed"
    msg "Done"
}

INPUT="$PUBLIC_SUFFIX_LIST_DAT_DIR/public_suffix_list.dat"

check_or_die tool -x "$MAKE_DAFSA"
if [ ! -d "$PUBLIC_SUFFIX_LIST_DAT_DIR" ]; then
    msg -n "$PUBLIC_SUFFIX_LIST_DAT_DIR does not exist; Clone $UPSTREAM there? [y/N]"
    read -N1 -t60
    msg
    if [ "x$REPLY" = "xy" -o "x$REPLY" = "xY" ]; then
        run_or_die git clone "$UPSTREAM" "$PUBLIC_SUFFIX_LIST_DAT_DIR"
    else
        check_or_die publicsuffix/list.git -d "$PUBLIC_SUFFIX_LIST_DAT_DIR" # reuse error message
    fi
fi
check_or_die publicsuffix/list.git -d "$PUBLIC_SUFFIX_LIST_DAT_DIR"
check_or_die input -r "$INPUT"
check_or_die output -w "$PSL_DATA_CPP"
check_or_die binary-output -w "$PUBLIC_SUFFIX_LIST_DAFSA"

GITSHA1=$(cd "$PUBLIC_SUFFIX_LIST_DAT_DIR" && git log -1 --format=format:%H)
TODAY=$(date +%Y-%m-%d)
msg "Using $INPUT @ $GITSHA1, fetched on $TODAY"

run_or_die "$MAKE_DAFSA" "$INPUT" "$PSL_DATA_CPP"
run_or_die "$MAKE_DAFSA" --output-format=binary "$INPUT" "$PUBLIC_SUFFIX_LIST_DAFSA"

# update the first Version line in qt_attribution.json with the new SHA1 and date:
run_or_die sed -i -e "1,/\"Version\":/{ /\"Version\":/ {  s/[0-9a-fA-F]\{40\}/$GITSHA1/;   s/[0-9]\{4\}-[0-9]\{2\}-[0-9]\{2\}/$TODAY/ } }" "$ATTRIBUTION_JSON"

run_or_die git add "$PSL_DATA_CPP"
run_or_die git add "$PUBLIC_SUFFIX_LIST_DAFSA"
run_or_die git add "$ATTRIBUTION_JSON"

run_or_die git commit -m "Update public suffix list

Version $GITSHA1, fetched on
$TODAY.

Pick-to: $PICK_TO_BRANCHES
" --edit
