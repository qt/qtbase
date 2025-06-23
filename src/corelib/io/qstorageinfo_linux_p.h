// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2014 Ivan Komissarov <ABBAPOH@gmail.com>
// Copyright (C) 2016 Intel Corporation.
// Copyright (C) 2023 Ahmad Samir <a.samirh78@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QSTORAGEINFO_LINUX_P_H
#define QSTORAGEINFO_LINUX_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.
// This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qstorageinfo_p.h"

#include <sys/sysmacros.h> // makedev()

QT_BEGIN_NAMESPACE

using MountInfo = QStorageInfoPrivate::MountInfo;

// parseMountInfo() is called from:
// - QStorageInfoPrivate::initRootPath(), where a list of all mounted volumes is needed
// - QStorageInfoPrivate::mountedVolumes(), where some filesystem types are ignored
//   (see shouldIncludefs())
enum class FilterMountInfo {
    All,
    Filtered,
};

#ifdef QT_BUILD_INTERNAL
Q_AUTOTEST_EXPORT
#else
static
#endif
std::vector<MountInfo> doParseMountInfo(
        const QByteArray &mountinfo, FilterMountInfo filter = FilterMountInfo::All);

QT_END_NAMESPACE

#endif // QSTORAGEINFO_LINUX_P_H
