/****************************************************************************
**
** Copyright (C) 2013 David Faure <faure+bluesystems@kde.org>
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2017 Intel Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qlockfile.h"
#include "qlockfile_p.h"

#include <QtCore/qthread.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qdeadlinetimer.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qfileinfo.h>

QT_BEGIN_NAMESPACE

namespace {
struct LockFileInfo
{
    qint64 pid;
    QString appname;
    QString hostname;
    QByteArray hostid;
    QByteArray bootid;
};
}

static bool getLockInfo_helper(const QString &fileName, LockFileInfo *info);

static QString machineName()
{
#ifdef Q_OS_WIN
    // we don't use QSysInfo because it tries to do name resolution
    return qEnvironmentVariable("COMPUTERNAME");
#else
    return QSysInfo::machineHostName();
#endif
}

/*!
    \class QLockFile
    \inmodule QtCore
    \brief The QLockFile class provides locking between processes using a file.
    \since 5.1

    A lock file can be used to prevent multiple processes from accessing concurrently
    the same resource. For instance, a configuration file on disk, or a socket, a port,
    a region of shared memory...

    Serialization is only guaranteed if all processes that access the shared resource
    use QLockFile, with the same file path.

    QLockFile supports two use cases:
    to protect a resource for a short-term operation (e.g. verifying if a configuration
    file has changed before saving new settings), and for long-lived protection of a
    resource (e.g. a document opened by a user in an editor) for an indefinite amount of time.

    When protecting for a short-term operation, it is acceptable to call lock() and wait
    until any running operation finishes.
    When protecting a resource over a long time, however, the application should always
    call setStaleLockTime(0) and then tryLock() with a short timeout, in order to
    warn the user that the resource is locked.

    If the process holding the lock crashes, the lock file stays on disk and can prevent
    any other process from accessing the shared resource, ever. For this reason, QLockFile
    tries to detect such a "stale" lock file, based on the process ID written into the file.
    To cover the situation that the process ID got reused meanwhile, the current process name is
    compared to the name of the process that corresponds to the process ID from the lock file.
    If the process names differ, the lock file is considered stale.
    Additionally, the last modification time of the lock file (30s by default, for the use case of a
    short-lived operation) is taken into account.
    If the lock file is found to be stale, it will be deleted.

    For the use case of protecting a resource over a long time, you should therefore call
    setStaleLockTime(0), and when tryLock() returns LockFailedError, inform the user
    that the document is locked, possibly using getLockInfo() for more details.

    \note On Windows, this class has problems detecting a stale lock if the
    machine's hostname contains characters outside the US-ASCII character set.
*/

/*!
    \enum QLockFile::LockError

    This enum describes the result of the last call to lock() or tryLock().

    \value NoError The lock was acquired successfully.
    \value LockFailedError The lock could not be acquired because another process holds it.
    \value PermissionError The lock file could not be created, for lack of permissions
                           in the parent directory.
    \value UnknownError Another error happened, for instance a full partition
                        prevented writing out the lock file.
*/

/*!
    Constructs a new lock file object.
    The object is created in an unlocked state.
    When calling lock() or tryLock(), a lock file named \a fileName will be created,
    if it doesn't already exist.

    \sa lock(), unlock()
*/
QLockFile::QLockFile(const QString &fileName)
    : d_ptr(new QLockFilePrivate(fileName))
{
}

/*!
    Destroys the lock file object.
    If the lock was acquired, this will release the lock, by deleting the lock file.
*/
QLockFile::~QLockFile()
{
    unlock();
}

/*!
    Sets \a staleLockTime to be the time in milliseconds after which
    a lock file is considered stale.
    The default value is 30000, i.e. 30 seconds.
    If your application typically keeps the file locked for more than 30 seconds
    (for instance while saving megabytes of data for 2 minutes), you should set
    a bigger value using setStaleLockTime().

    The value of \a staleLockTime is used by lock() and tryLock() in order
    to determine when an existing lock file is considered stale, i.e. left over
    by a crashed process. This is useful for the case where the PID got reused
    meanwhile, so one way to detect a stale lock file is by the fact that
    it has been around for a long time.

    \sa staleLockTime()
*/
void QLockFile::setStaleLockTime(int staleLockTime)
{
    Q_D(QLockFile);
    d->staleLockTime = staleLockTime;
}

/*!
    Returns the time in milliseconds after which
    a lock file is considered stale.

    \sa setStaleLockTime()
*/
int QLockFile::staleLockTime() const
{
    Q_D(const QLockFile);
    return d->staleLockTime;
}

/*!
    Returns \c true if the lock was acquired by this QLockFile instance,
    otherwise returns \c false.

    \sa lock(), unlock(), tryLock()
*/
bool QLockFile::isLocked() const
{
    Q_D(const QLockFile);
    return d->isLocked;
}

/*!
    Creates the lock file.

    If another process (or another thread) has created the lock file already,
    this function will block until that process (or thread) releases it.

    Calling this function multiple times on the same lock from the same
    thread without unlocking first is not allowed. This function will
    \e dead-lock when the file is locked recursively.

    Returns \c true if the lock was acquired, false if it could not be acquired
    due to an unrecoverable error, such as no permissions in the parent directory.

    \sa unlock(), tryLock()
*/
bool QLockFile::lock()
{
    return tryLock(-1);
}

/*!
    Attempts to create the lock file. This function returns \c true if the
    lock was obtained; otherwise it returns \c false. If another process (or
    another thread) has created the lock file already, this function will
    wait for at most \a timeout milliseconds for the lock file to become
    available.

    Note: Passing a negative number as the \a timeout is equivalent to
    calling lock(), i.e. this function will wait forever until the lock
    file can be locked if \a timeout is negative.

    If the lock was obtained, it must be released with unlock()
    before another process (or thread) can successfully lock it.

    Calling this function multiple times on the same lock from the same
    thread without unlocking first is not allowed, this function will
    \e always return false when attempting to lock the file recursively.

    \sa lock(), unlock()
*/
bool QLockFile::tryLock(int timeout)
{
    Q_D(QLockFile);
    QDeadlineTimer timer(qMax(timeout, -1));    // QDT only takes -1 as "forever"
    int sleepTime = 100;
    forever {
        d->lockError = d->tryLock_sys();
        switch (d->lockError) {
        case NoError:
            d->isLocked = true;
            return true;
        case PermissionError:
        case UnknownError:
            return false;
        case LockFailedError:
            if (!d->isLocked && d->isApparentlyStale()) {
                if (Q_UNLIKELY(QFileInfo(d->fileName).lastModified() > QDateTime::currentDateTime()))
                    qInfo("QLockFile: Lock file '%ls' has a modification time in the future", qUtf16Printable(d->fileName));
                // Stale lock from another thread/process
                // Ensure two processes don't remove it at the same time
                QLockFile rmlock(d->fileName + QLatin1String(".rmlock"));
                if (rmlock.tryLock()) {
                    if (d->isApparentlyStale() && d->removeStaleLock())
                        continue;
                }
            }
            break;
        }

        int remainingTime = timer.remainingTime();
        if (remainingTime == 0)
            return false;
        else if (uint(sleepTime) > uint(remainingTime))
            sleepTime = remainingTime;

        QThread::msleep(sleepTime);
        if (sleepTime < 5 * 1000)
            sleepTime *= 2;
    }
    // not reached
    return false;
}

/*!
    \fn void QLockFile::unlock()
    Releases the lock, by deleting the lock file.

    Calling unlock() without locking the file first, does nothing.

    \sa lock(), tryLock()
*/

/*!
    Retrieves information about the current owner of the lock file.

    If tryLock() returns \c false, and error() returns LockFailedError,
    this function can be called to find out more information about the existing
    lock file:
    \list
    \li the PID of the application (returned in \a pid)
    \li the \a hostname it's running on (useful in case of networked filesystems),
    \li the name of the application which created it (returned in \a appname),
    \endlist

    Note that tryLock() automatically deleted the file if there is no
    running application with this PID, so LockFailedError can only happen if there is
    an application with this PID (it could be unrelated though).

    This can be used to inform users about the existing lock file and give them
    the choice to delete it. After removing the file using removeStaleLockFile(),
    the application can call tryLock() again.

    This function returns \c true if the information could be successfully retrieved, false
    if the lock file doesn't exist or doesn't contain the expected data.
    This can happen if the lock file was deleted between the time where tryLock() failed
    and the call to this function. Simply call tryLock() again if this happens.
*/
bool QLockFile::getLockInfo(qint64 *pid, QString *hostname, QString *appname) const
{
    Q_D(const QLockFile);
    LockFileInfo info;
    if (!getLockInfo_helper(d->fileName, &info))
        return false;
    if (pid)
        *pid = info.pid;
    if (hostname)
        *hostname = info.hostname;
    if (appname)
        *appname = info.appname;
    return true;
}

QByteArray QLockFilePrivate::lockFileContents() const
{
    // Use operator% from the fast builder to avoid multiple memory allocations.
    return QByteArray::number(QCoreApplication::applicationPid()) % '\n'
            % processNameByPid(QCoreApplication::applicationPid()).toUtf8() % '\n'
            % machineName().toUtf8() % '\n'
            % QSysInfo::machineUniqueId() % '\n'
            % QSysInfo::bootUniqueId() % '\n';
}

static bool getLockInfo_helper(const QString &fileName, LockFileInfo *info)
{
    QFile reader(fileName);
    if (!reader.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QByteArray pidLine = reader.readLine();
    pidLine.chop(1);
    if (pidLine.isEmpty())
        return false;
    QByteArray appNameLine = reader.readLine();
    appNameLine.chop(1);
    QByteArray hostNameLine = reader.readLine();
    hostNameLine.chop(1);

    // prior to Qt 5.10, only the lines above were recorded
    QByteArray hostId = reader.readLine();
    hostId.chop(1);
    QByteArray bootId = reader.readLine();
    bootId.chop(1);

    bool ok;
    info->appname = QString::fromUtf8(appNameLine);
    info->hostname = QString::fromUtf8(hostNameLine);
    info->hostid = hostId;
    info->bootid = bootId;
    info->pid = pidLine.toLongLong(&ok);
    return ok && info->pid > 0;
}

bool QLockFilePrivate::isApparentlyStale() const
{
    LockFileInfo info;
    if (getLockInfo_helper(fileName, &info)) {
        bool sameHost = info.hostname.isEmpty() || info.hostname == machineName();
        if (!info.hostid.isEmpty()) {
            // Override with the host ID, if we know it.
            QByteArray ourHostId = QSysInfo::machineUniqueId();
            if (!ourHostId.isEmpty())
                sameHost = (ourHostId == info.hostid);
        }

        if (sameHost) {
            if (!info.bootid.isEmpty()) {
                // If we've rebooted, then the lock is definitely stale.
                if (info.bootid != QSysInfo::bootUniqueId())
                    return true;
            }
            if (!isProcessRunning(info.pid, info.appname))
                return true;
        }
    }

    const qint64 age = QFileInfo(fileName).lastModified().msecsTo(QDateTime::currentDateTimeUtc());
    return staleLockTime > 0 && qAbs(age) > staleLockTime;
}

/*!
    Attempts to forcefully remove an existing lock file.

    Calling this is not recommended when protecting a short-lived operation: QLockFile
    already takes care of removing lock files after they are older than staleLockTime().

    This method should only be called when protecting a resource for a long time, i.e.
    with staleLockTime(0), and after tryLock() returned LockFailedError, and the user
    agreed on removing the lock file.

    Returns \c true on success, false if the lock file couldn't be removed. This happens
    on Windows, when the application owning the lock is still running.
*/
bool QLockFile::removeStaleLockFile()
{
    Q_D(QLockFile);
    if (d->isLocked) {
        qWarning("removeStaleLockFile can only be called when not holding the lock");
        return false;
    }
    return d->removeStaleLock();
}

/*!
    Returns the lock file error status.

    If tryLock() returns \c false, this function can be called to find out
    the reason why the locking failed.
*/
QLockFile::LockError QLockFile::error() const
{
    Q_D(const QLockFile);
    return d->lockError;
}

QT_END_NAMESPACE
