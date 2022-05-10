// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
    QOperatingSystemVersion::current() >= QOperatingSystemVersion(QOperatingSystemVersion::IOS, 9)
//! [0]

//! [1]
    auto current = QOperatingSystemVersion::current();
    if (current >= QOperatingSystemVersion::OSXYosemite ||
        current >= QOperatingSystemVersion(QOperatingSystemVersion::IOS, 8)) {
        // returns true on macOS >= 10.10 and iOS >= 8.0, but false on macOS < 10.10 and iOS < 8.0
    }
//! [1]
