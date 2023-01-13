// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause


//! [0]
extern Q_CORE_EXPORT int qt_ntfs_permission_lookup;
//! [0]

//! [1]
qt_ntfs_permission_lookup++; // turn checking on
qt_ntfs_permission_lookup--; // turn it off again
//! [1]

//! [raii]
void complexFunction()
{
    QNtfsPermissionCheckGuard permissionGuard;  // check is enabled

    // do complex things here that need permission check enabled

}   // as the guard goes out of scope the check is disabled
//! [raii]

//! [free-funcs]
qAreNtfsPermissionChecksEnabled();   // check status
qEnableNtfsPermissionChecks();       // turn checking on
qDisableNtfsPermissionChecks();      // turn it off again
//! [free-funcs]
