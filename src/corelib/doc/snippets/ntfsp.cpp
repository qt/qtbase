// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause


//! [0]
extern Q_CORE_EXPORT int qt_ntfs_permission_lookup;
//! [0]

//! [1]
qt_ntfs_permission_lookup++; // turn checking on
qt_ntfs_permission_lookup--; // turn it off again
//! [1]

