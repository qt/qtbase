#!/bin/bash
/****************************************************************************
**
** Copyright (C) 2023 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the utils of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
******************************************************************************/

# This is the Qt 6.2 version of the script. The way to update the
# publicsuffix-list is very different from Qt 6.5+, but this script is
# run the same way as in dev, papering over the differences in Qt
# branches.

#UPSTREAM=github.com:publicsuffix/list.git          # use this if you have a github account
UPSTREAM=https://github.com/publicsuffix/list.git  # and this if you don't

THIS="util/update_public_suffix_list.sh"
TOOL_DIR=util/publicSuffix
TOOL=$TOOL_DIR/publicSuffix
PUBLIC_SUFFIX_LIST_DAT_DIR="$1"
QURLTLDS_P_H=src/network/kernel/qurltlds_p.h
ATTRIBUTION_JSON=src/network/kernel/qt_attribution.json

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

if [ ! -x "$TOOL" ] ; then
    msg "$TOOL not found, trying to build it (you will need a working qmake in PATH)"
    check_or_die tool_dir -d "$TOOL_DIR"
    pushd "$TOOL_DIR"
    run_or_die qmake .
    run_or_die make
    popd
fi
check_or_die tool -x "$TOOL"
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
check_or_die output -w "$QURLTLDS_P_H"

GITSHA1=$(cd "$PUBLIC_SUFFIX_LIST_DAT_DIR" && git log -1 --format=format:%H)
TODAY=$(date +%Y-%m-%d)
msg "Using $INPUT @ $GITSHA1, fetched on $TODAY"

OUTPUT="$(mktemp)"
trap "rm $OUTPUT" EXIT
run_or_die "$TOOL" "$INPUT" "$OUTPUT"

# splice $OUTPUT into qurltlds_p_h between QT_BEGIN/END_NAMESPACE
run_or_die ed - "$QURLTLDS_P_H" <<EOF
/^QT_BEGIN_NAMESPACE/+2,/^QT_END_NAMESPACE/-2d
-r $OUTPUT
w
q
EOF

# update the first Version line in qt_attribution.json with the new SHA1 and date:
run_or_die sed -i -e "1,/\"Version\":/{ /\"Version\":/ {  s/[0-9a-fA-F]\{40\}/$GITSHA1/;   s/[0-9]\{4\}-[0-9]\{2\}-[0-9]\{2\}/$TODAY/ } }" "$ATTRIBUTION_JSON"

run_or_die git add "$QURLTLDS_P_H"
run_or_die git add "$ATTRIBUTION_JSON"

run_or_die git commit -m "Update public suffix list

Version $GITSHA1, fetched on
$TODAY.
" --edit

msg "Please use topic:publicsuffix-list-$GITSHA1 when pushing."
