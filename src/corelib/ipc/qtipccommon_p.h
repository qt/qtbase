// Copyright (C) 2022 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTIPCCOMMON_P_H
#define QTIPCCOMMON_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qtipccommon.h"
#include <private/qglobal_p.h>
#include <private/qtcore-config_p.h>

#if QT_CONFIG(sharedmemory) || QT_CONFIG(systemsemaphore)

#if defined(Q_OS_UNIX)
#  include <qfile.h>
#  include <private/qcore_unix_p.h>
#endif

QT_BEGIN_NAMESPACE

namespace QtIpcCommon {
enum class IpcType {
    SharedMemory,
    SystemSemaphore
};

static constexpr bool isIpcSupported(IpcType ipcType, QNativeIpcKey::Type type)
{
    switch (type) {
    case QNativeIpcKey::Type::SystemV:
        break;

    case QNativeIpcKey::Type::PosixRealtime:
        if (ipcType == IpcType::SharedMemory)
            return QT_CONFIG(posix_shm);
        return QT_CONFIG(posix_sem);

    case QNativeIpcKey::Type::Windows:
#ifdef Q_OS_WIN
        return true;
#else
        return false;
#endif
    }

    if (ipcType == IpcType::SharedMemory)
        return QT_CONFIG(sysv_shm);
    return QT_CONFIG(sysv_sem);
}

Q_AUTOTEST_EXPORT QString
legacyPlatformSafeKey(const QString &key, IpcType ipcType,
                      QNativeIpcKey::Type type = QNativeIpcKey::legacyDefaultTypeForOs());

#ifdef Q_OS_UNIX
// Convenience function to create the file if needed
inline int createUnixKeyFile(const QByteArray &fileName)
{
    int fd = qt_safe_open(fileName.constData(), O_EXCL | O_CREAT | O_RDWR, 0640);
    if (fd < 0) {
        if (errno == EEXIST)
            return 0;
        return -1;
    } else {
        close(fd);
    }
    return 1;

}
#endif // Q_OS_UNIX
} // namespace QtIpcCommon

QT_END_NAMESPACE

#endif // QT_CONFIG(sharedmemory) || QT_CONFIG(systemsemaphore)


#endif // QTIPCCOMMON_P_H
