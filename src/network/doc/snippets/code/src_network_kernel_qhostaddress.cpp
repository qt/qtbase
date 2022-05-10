// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
Q_IPV6ADDR addr = hostAddr.toIPv6Address();
// addr contains 16 unsigned characters

for (int i = 0; i < 16; ++i) {
    // process addr[i]
}
//! [0]
