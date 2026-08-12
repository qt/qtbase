// Copyright (C) 2013 David Faure <faure+bluesystems@kde.org>
// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2017 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "qlockfile.h"
#include "qlockfile_p.h"

#include <QtCore/qthread.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qdeadlinetimer.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qfileinfo.h>

#include <qplatformdefs.h>

#ifdef Q_OS_WIN
#include <io.h>
#include <qt_windows.h>
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

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
    \ingroup io
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
    call setStaleLockTime(0ms) and then tryLock() with a short timeout, in order to
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

    \section1 Limitations

    QLockFile's implementation is usually safe, for the majority of
    environments and filesystems. There are a few situations in which it can
    incorrectly conclude a lock file is stale when it isn't, fail to detect a
    stale file, or race when locking. This section documents those issues.

    There are two main mitigation strategies: choosing a suitable, non-zero
    staleLockTime() and choosing a regular, local filesystem for storing lock
    files (such as \c{/var/lock} if running as root on Unix systems or the
    \l{QStandardPaths::RuntimeLocation}{runtime location}). That may not be
    possible for certain use-cases of QLockFile, such as when using QLockFile
    to indicate a file path provided by the user is being edited.

    \section2 Non-persistent machine IDs

    QLockFile uses the machine's unique ID to differentiate a lock by the
    current machine and one by a process running on a different host. If the
    machine ID changes over time (such as on systems with ephemeral storage,
    which must generate a new ID at every boot), QLockFile will be unable to
    detect that a lock file is stale after a reboot.

    Conversely, if two different machines have colliding machine IDs (for
    example, a cloned system image or restoration from backups) and provide a
    network path to QLockFile, the class may conclude the lock is stale when it
    actually is not.

    \section2 Absence of native locking

    QLockFile uses operating-system specific calls to indicate to other
    processes and threads that the lock is alive (not stale), even past the
    staleLockTime() setting. This functionality may be absent for files on some
    networked or virtual filesystems (such as FUSE on Linux and \macos), when
    using different filesystems to access the same file if the native lock
    isn't carried through, or in certain environments that block the necessary
    system calls.

    If the native locking support is absent, QLockFile will rely solely on the
    existence, modification time, and contents of the lock file itself. In this
    case, if the process currently locking the resource keeps it past
    staleLockTime(), QLockFile will steal the lock.

    \section2 Networked filesystems

    It is unspecified whether the native file-locking is supported in networked
    environments: with some implementations, it is supported for all clients
    accessing the filesystem; for others the local system may support locking
    for its own processes but will not share the locking over the network; and
    for yet others there is no native locking even inside one host. Moreover,
    it is possible that some clients participate in networked locking and some
    others do not, for the same file. If locking is not supported across the
    network, QLockFile will observe the limitations described above for the
    absence of native locking.

    Additionally, if the lock file contains the identification of a different
    host, QLockFile will be unable to confirm the process holding a lock is
    still running, and will need to wait staleLockTime() to recover from an
    unclean exit.

    \section2 Accessing different actual files through the same path

    It is possible for two processes to have different views of the filesystem,
    causing an identical file path to be different files in the filesystem. In
    this case, the two QLockFile objects will likely succeed at creating the
    lock, but will not be mutually exclusive. This may cause conflicts if the
    resource the lock file is protecting is still shared between them.

    This situation is most often encountered with containers (see below), but
    is not exclusive to them.

    \section2 Files shared with containers

    With some container implementations, it is possible to hide the existence
    of some processes (for example, Linux's "PID namespace" feature). If a
    process is running inside of such a container but shares the machine ID of
    the host system or another container, two processes in different containers
    (or the host) will make incorrect determinations on whether the locking
    process is still running. Moreover, some container controllers may replace
    the boot ID inside of the container, causing the launched processes to
    conclude the lock file is always stale, regardless of how fresh its
    timestamp is.

    With some other implementations, containers may have ephemeral storage or
    intentionally create a new machine ID to avoid collision. In this case, the
    application will experience the problems described above for non-persistent
    machine IDs.

    However, the native file-locking usually works (subject to filesystem and
    environment limitations as discussed above), so even if QLockFile did
    conclude the file is apparently stale, it won't steal a lock file that is
    natively locked.

    \section2 Accesses not using the same protocol

    QLockFile cannot interoperate with modifications to the lock file performed
    outside of the protocol implemented by this class. This includes removal of
    the lock file by other tools, such as tmpwatch and similar, but also some
    virus-scanning or similar tools.

    \section2 Clock skew

    QLockFile relies on the time stamp of the lock file being accurate. If the
    clock jumps forward, QLockFile may conclude a lock file has become stale
    when it hasn't, and vice-versa for jumping backwards.

    For this reason, it is recommended all systems keep network-synchronized
    time and perform this synchronization early in their boot process. This is
    particularly important for networked filesystems.
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
 * Returns the file name of the lock file
 */
QString QLockFile::fileName() const
{
    return d_ptr->fileName;
}

/*!
    \fn void QLockFile::setStaleLockTime(int staleLockTime)

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

    This is an overloaded function, equivalent to calling:
    \code
    setStaleLockTime(std::chrono::milliseconds{staleLockTime});
    \endcode

    \sa staleLockTime()
*/

/*!
    \since 6.2

    Sets the interval after which a lock file is considered stale to \a staleLockTime.
    The default value is 30s.

    If your application typically keeps the file locked for more than 30 seconds
    (for instance while saving megabytes of data for 2 minutes), you should set
    a bigger value using setStaleLockTime().

    The value of staleLockTime() is used by lock() and tryLock() in order
    to determine when an existing lock file is considered stale, i.e. left over
    by a crashed process. This is useful for the case where the PID got reused
    meanwhile, so one way to detect a stale lock file is by the fact that
    it has been around for a long time.

    Setting this value to 0 or negative will disable the verification of
    timestamps on lock files. QLockFile will still detect a stale lock if its
    contents show that the locking process is no longer running.

    \sa staleLockTime()
*/
void QLockFile::setStaleLockTime(std::chrono::milliseconds staleLockTime)
{
    Q_D(QLockFile);
    d->staleLockTime = staleLockTime;
}

/*!
    \fn int QLockFile::staleLockTime() const

    Returns the time in milliseconds after which
    a lock file is considered stale.

    \sa setStaleLockTime()
*/

/*! \fn std::chrono::milliseconds QLockFile::staleLockTimeAsDuration() const
    \overload
    \since 6.2

    Returns a std::chrono::milliseconds object which denotes the time after
    which a lock file is considered stale.

    \sa setStaleLockTime()
*/
std::chrono::milliseconds QLockFile::staleLockTimeAsDuration() const
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
    return tryLock(std::chrono::milliseconds::max());
}

/*!
    \fn bool QLockFile::tryLock(int timeout)

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

/*!
    \overload
    \since 6.2

    Attempts to create the lock file. This function returns \c true if the
    lock was obtained; otherwise it returns \c false. If another process (or
    another thread) has created the lock file already, this function will
    wait for at most \a timeout for the lock file to become available.

    If the lock was obtained, it must be released with unlock()
    before another process (or thread) can successfully lock it.

    Calling this function multiple times on the same lock from the same
    thread without unlocking first is not allowed, this function will
    \e always return false when attempting to lock the file recursively.

    \sa lock(), unlock()
*/
bool QLockFile::tryLock(std::chrono::milliseconds timeout)
{
    using namespace std::chrono_literals;
    using Msec = std::chrono::milliseconds;

    Q_D(QLockFile);
    QLockFilePrivate::LockFileInfo current(QLockFilePrivate::LockFileInfo::Current{});

    QDeadlineTimer timer(timeout < 0ms ? Msec::max() : timeout);

    Msec sleepTime = 100ms;
    while (true) {
        d->lockError = d->tryLock_sys(current);
        switch (d->lockError) {
        case NoError:
            d->isLocked = true;
            return true;
        case PermissionError:
        case UnknownError:
            return false;
        case LockFailedError:
            if (!d->isLocked && d->isApparentlyStale(current)) {
                if (Q_UNLIKELY(QFileInfo(d->fileName).lastModified(QTimeZone::UTC) > QDateTime::currentDateTimeUtc()))
                    qInfo("QLockFile: Lock file '%ls' has a modification time in the future", qUtf16Printable(d->fileName));
                // Stale lock from another thread/process
                // Ensure two processes don't remove it at the same time
                QLockFile rmlock(d->fileName + ".rmlock"_L1);
                if (rmlock.tryLock()) {
                    if (d->isApparentlyStale(current) && d->removeStaleLock())
                        continue;
                }
            }
            break;
        }

        auto remainingTime = std::chrono::duration_cast<Msec>(timer.remainingTimeAsDuration());
        if (remainingTime == 0ms)
            return false;

        if (sleepTime > remainingTime)
            sleepTime = remainingTime;

        QThread::sleep(sleepTime);
        if (sleepTime < 5s)
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
    std::optional opt = QLockFilePrivate::getLockInfo_helper(d->fileName);
    if (!opt)
        return false;
    QLockFilePrivate::LockFileInfo &info = *opt;
    if (pid)
        *pid = info.pid;
    if (hostname)
        *hostname = info.hostname;
    if (appname)
        *appname = info.appname;
    return true;
}

QLockFilePrivate::QLockFilePrivate(const QString &fn)
    : fileName(fn)
{
}

QLockFilePrivate::~QLockFilePrivate()
    = default;

QByteArray QLockFilePrivate::LockFileInfo::asFileContents() const
{
    // Use operator% from the fast builder to avoid multiple memory allocations.
    return QByteArray::number(pid) % '\n'
            % appname.toUtf8() % '\n'
            % hostname.toUtf8() % '\n'
            % hostid % '\n'
            % bootid % '\n';
}

QLockFilePrivate::LockFileInfo::LockFileInfo(Current)
    : pid(QCoreApplication::applicationPid()),
      appname(processNameByPid(pid)),
      hostname(machineName()),
      hostid(QSysInfo::machineUniqueId()),
      bootid(QSysInfo::bootUniqueId())
{
}

std::optional<QLockFilePrivate::LockFileInfo> QLockFilePrivate::getLockInfo_helper(const QString &fileName)
{
    std::optional<QLockFilePrivate::LockFileInfo> info;
    int fd = openNewFileDescriptor(fileName);
    if (fd < 0)
        return info;
    QFile reader;
    if (!reader.open(fd, QFile::ReadOnly | QFile::Text, QFile::AutoCloseHandle)) {
        QT_CLOSE(fd);
        return info;
    }

    bool ok;
    QByteArray pidLine = reader.readLine();
    pidLine.chop(1);
    qint64 pid = pidLine.toLongLong(&ok);
    if (!ok || pid <= 0)
        return info;
    QByteArray appNameLine = reader.readLine();
    appNameLine.chop(1);
    QByteArray hostNameLine = reader.readLine();
    hostNameLine.chop(1);

    // prior to Qt 5.10, only the lines above were recorded
    QByteArray hostId = reader.readLine();
    hostId.chop(1);
    QByteArray bootId = reader.readLine();
    bootId.chop(1);

    info.emplace();
    info->appname = QString::fromUtf8(appNameLine);
    info->hostname = QString::fromUtf8(hostNameLine);
    info->hostid = std::move(hostId);
    info->bootid = std::move(bootId);
    info->pid = pid;
    return info;
}

bool QLockFilePrivate::isApparentlyStale(const LockFileInfo &current) const
{
    // check the file's mtime first (cheaper)
    using namespace std::chrono;
    if (staleLockTime > 0ms) {
        const QDateTime lastMod = QFileInfo(fileName).lastModified(QTimeZone::UTC);
        const milliseconds age{lastMod.msecsTo(QDateTime::currentDateTimeUtc())};
        if (abs(age) > staleLockTime)
            return true;
    }

    if (std::optional opt = getLockInfo_helper(fileName)) {
        LockFileInfo &info = *opt;
        bool sameHost = info.hostname.isEmpty() || info.hostname == current.hostname;
        if (!info.hostid.isEmpty()) {
            // Override with the host ID, if we know it.
            if (!current.hostid.isEmpty())
                sameHost = (current.hostid == info.hostid);
        }

        if (sameHost) {
            if (!info.bootid.isEmpty()) {
                // If we've rebooted, then the lock is definitely stale.
                if (info.bootid != current.bootid)
                    return true;
            }
            if (!isProcessRunning(info.pid, info.appname))
                return true;
        }
    }

    // not stale
    return false;
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
