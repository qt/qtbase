#!/bin/bash
# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
# Qt-Security score:insignificant reason:build-tool-containing-no-compiled-source

# run this script in the qtbase srcdir with qlalr in PATH

function msg() {
    echo "$@" 1>&2
}

function die() {
    msg "$@"
    exit 1
}

cd src/tools/qlalr || die "not in qtbase toplevel srcdir?"
exec qlalr \
     --qt \
     --no-debug \
     --no-lines \
     --use-pragma-once \
     lalr.g
