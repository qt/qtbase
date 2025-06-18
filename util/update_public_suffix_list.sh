#!/bin/bash
# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

PICK_TO_BRANCHES="6.9 6.8 6.5 6.2 5.15"
UPSTREAM=https://publicsuffix.org/list/public_suffix_list.dat

THIS="util/update_public_suffix_list.sh"
PUBLIC_SUFFIX_LIST_DAT_DIR="$(mktemp -d)"
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
    test $OP "$FILE" || die "$TYPE \"$FILE\" not found (test $OP \"$FILE\" failed). Please run $THIS from \$SRCDIR/qtbase."
}

function run_or_die() {
    msg -n "Running \"$@\"..."
    "$@" || die "Failed"
    msg "Done"
}

DAT_FILE="$PUBLIC_SUFFIX_LIST_DAT_DIR/public_suffix_list.dat"

check_or_die tool -x "$MAKE_DAFSA"
check_or_die directory -d "$PUBLIC_SUFFIX_LIST_DAT_DIR"
run_or_die wget $UPSTREAM -O "$DAT_FILE"

check_or_die input -r "$DAT_FILE"
check_or_die output -w "$PSL_DATA_CPP"
check_or_die binary-output -w "$PUBLIC_SUFFIX_LIST_DAFSA"

VERSION=$(run_or_die sed -nE 's,^// VERSION: (.*)$,\1,p' "$DAT_FILE")
if [[ -z "$VERSION" ]]; then
   die "Something is wrong! Recheck the VERSION line in $DAT_FILE and update the script."
fi
GITSHA=$(run_or_die sed -nE 's,^// COMMIT: (.*)$,\1,p' "$DAT_FILE")
msg "Using $DAT_FILE @ ${VERSION} (commit ${GITSHA})"

run_or_die "$MAKE_DAFSA" "$DAT_FILE" "$PSL_DATA_CPP"
run_or_die "$MAKE_DAFSA" --output-format=binary "$DAT_FILE" "$PUBLIC_SUFFIX_LIST_DAFSA"
rm "$DAT_FILE"

# update the first Version line in qt_attribution.json with the new SHA1 and date:
run_or_die sed -i -E -e "1,/\"Version\":/{ /\"Version\":/ { s/(Version\": )\".*?\"/\\1\"${VERSION}\"/ } }" "$ATTRIBUTION_JSON"

# update the first "PURL" line with the new SHA1:
run_or_die sed -i -E -e "1,/\"PURL\":/{ /\"PURL\":/ { s/@.*?\\?/@${GITSHA}\\?/ } }" "$ATTRIBUTION_JSON"


run_or_die git add "$PSL_DATA_CPP"
run_or_die git add "$PUBLIC_SUFFIX_LIST_DAFSA"
run_or_die git add "$ATTRIBUTION_JSON"

run_or_die git commit -m "Update public suffix list

Version ${VERSION}.

[ChangeLog][Third-Party Code] Updated the public suffix list to upstream
version ${VERSION}.

Pick-to: $PICK_TO_BRANCHES
" --edit

msg "Please use topic=publicsuffix-list-${VERSION} when pushing."
