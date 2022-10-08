// Copyright (C) 2022 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qtipccommon_p.h"

#include <qcryptographichash.h>
#include <qdir.h>

#if defined(Q_OS_DARWIN)
#  include "private/qcore_mac_p.h"
#  if !defined(SHM_NAME_MAX)
     // Based on PSEMNAMLEN in XNU's posix_sem.c, which would
     // indicate the max length is 31, _excluding_ the zero
     // terminator. But in practice (possibly due to an off-
     // by-one bug in the kernel) the usable bytes are only 30.
#    define SHM_NAME_MAX 30
#  endif
#endif

#if QT_CONFIG(sharedmemory) || QT_CONFIG(systemsemaphore)

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

/*!
    \internal

    Legacy: this exists for compatibility with QSharedMemory and
    QSystemSemaphore between 4.4 and 6.6.

    Generate a string from the key which can be any unicode string into
    the subset that the win/unix kernel allows.

    On Unix this will be a file name
*/
QString QtIpcCommon::legacyPlatformSafeKey(const QString &key, QtIpcCommon::IpcType ipcType)
{
    if (key.isEmpty())
        return QString();

    QByteArray hex = QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Sha1).toHex();

#if defined(Q_OS_DARWIN) && defined(QT_POSIX_IPC)
    if (qt_apple_isSandboxed()) {
        // Sandboxed applications on Apple platforms require the shared memory name
        // to be in the form <application group identifier>/<custom identifier>.
        // Since we don't know which application group identifier the user wants
        // to apply, we instead document that requirement, and use the key directly.
        return key;
    } else {
        // The shared memory name limit on Apple platforms is very low (30 characters),
        // so we can't use the logic below of combining the prefix, key, and a hash,
        // to ensure a unique and valid name. Instead we use the first part of the
        // hash, which should still long enough to avoid collisions in practice.
        return u'/' + hex.left(SHM_NAME_MAX - 1);
    }
#endif

    QString result;
    result.reserve(1 + 18 + key.size() + 40);
    switch (ipcType) {
    case IpcType::SharedMemory:
        result += "qipc_sharedmemory_"_L1;
        break;
    case IpcType::SystemSemaphore:
        result += "qipc_systemsem_"_L1;
        break;
    }

    for (QChar ch : key) {
        if ((ch >= u'a' && ch <= u'z') ||
           (ch >= u'A' && ch <= u'Z'))
           result += ch;
    }
    result.append(QLatin1StringView(hex));

#ifdef Q_OS_WIN
    return result;
#elif defined(QT_POSIX_IPC)
    return u'/' + result;
#else
    return QDir::tempPath() + u'/' + result;
#endif
}

QT_END_NAMESPACE

#endif // QT_CONFIG(sharedmemory) || QT_CONFIG(systemsemaphore)
