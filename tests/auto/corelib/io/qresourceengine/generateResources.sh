#!/bin/sh
# Copyright (C) 2016 Intel Corporation.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
count=`awk '/ZERO_FILE_LEN/ { print $3 }' tst_qresourceengine.cpp`
dd if=/dev/zero of=zero.txt bs=1 count=$count
rcc --binary -o uncompressed.rcc --no-compress compressed.qrc
rcc --binary -o zlib.rcc --compress-algo zlib --compress 9 compressed.qrc
rcc --binary -o zstd.rcc --compress-algo zstd --compress 19 compressed.qrc
rm zero.txt
