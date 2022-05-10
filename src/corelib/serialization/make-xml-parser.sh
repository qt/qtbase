#!/bin/sh
# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

me=$(dirname $0)
mkdir -p $me/out
(cd $me/out && ../../../../util/qlalr/qlalr --qt --no-debug --no-lines ../qxmlstream.g)

for f in $me/out/*.h; do
    n=$(basename $f)
    cp $f $n
done

git diff .

