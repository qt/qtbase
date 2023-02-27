// Copyright (C) 2013 David Faure <faure+bluesystems@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QLOCKFILE_P_H
#define QLOCKFILE_P_H

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

#include <QtCore/private/qglobal_p.h>
#include <QtCore/qlockfile.h>
#include <QtCore/qfile.h>

#include <qplatformdefs.h>

#ifdef Q_OS_WIN
#include <io.h>
#include <qt_windows.h>
#endif

QT_BEGIN_NAMESPACE

class QLockFilePrivate
{
public:
    QLockFilePrivate(const QString &fn)
        : fileName(fn)
    {
    }
    QLockFile::LockError tryLock_sys();
    bool removeStaleLock();
    QByteArray lockFileContents() const;
    // Returns \c true if the lock belongs to dead PID, or is old.
    // The attempt to delete it will tell us if it was really stale or not, though.
    bool isApparentlyStale() const;

    // used in dbusmenu
    Q_CORE_EXPORT static QString processNameByPid(qint64 pid);
    static bool isProcessRunning(qint64 pid, const QString &appname);

    QString fileName;

#ifdef Q_OS_WIN
    Qt::HANDLE fileHandle = INVALID_HANDLE_VALUE;
#else
    int fileHandle = -1;
#endif

    std::chrono::milliseconds staleLockTime = std::chrono::seconds{30};
    QLockFile::LockError lockError = QLockFile::NoError;
    bool isLocked = false;

    static int getLockFileHandle(QLockFile *f)
    {
        int fd;
#ifdef Q_OS_WIN
        // Use of this function on Windows WILL leak a file descriptor.
        fd = _open_osfhandle(intptr_t(f->d_func()->fileHandle), 0);
#else
        fd = f->d_func()->fileHandle;
#endif
        QT_LSEEK(fd, 0, SEEK_SET);
        return fd;
    }
};

QT_END_NAMESPACE

#endif /* QLOCKFILE_P_H */
