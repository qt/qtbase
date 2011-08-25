/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include "qsystemlock.h"
#include "qsystemlock_p.h"

#include <qdebug.h>

/*! \class QSystemLocker

    \brief The QSystemLocker class is a convenience class that simplifies
    locking and unlocking system locks.

    The purpose of QSystemLocker is to simplify QSystemLock locking and
    unlocking.  Locking and unlocking a QSystemLock in complex functions and
    statements or in exception handling code is error-prone and difficult to
    debug.  QSystemLocker can be used in such situations to ensure that the
    state of the locks is always well-defined.

    QSystemLocker should be created within a function where a QSystemLock needs
    to be locked. The system lock is locked when QSystemLocker is created.  If
    locked, the system lock will be unlocked when the QSystemLocker is
    destroyed.  QSystemLocker can be unlocked with unlock() and relocked with
    relock().

    \sa QSystemLock
 */

/*! \fn QSystemLocker::QSystemLocker()

    Constructs a QSystemLocker and locks \a lock. The \a lock will be
    unlocked when the QSystemLocker is destroyed. If lock is zero,
    QSystemLocker does nothing.

    \sa QSystemLock::lock()
  */

/*! \fn QSystemLocker::~QSystemLocker()

    Destroys the QSystemLocker and unlocks it if it was
    locked in the constructor.

    \sa QSystemLock::unlock()
 */

/*! \fn QSystemLocker::systemLock()

    Returns a pointer to the lock that was locked in the constructor.
 */

/*! \fn QSystemLocker::relock()

    Relocks an unlocked locker.

    \sa unlock()
  */

/*! \fn QSystemLocker::unlock()

    Unlocks this locker. You can use relock() to lock it again.
    It does not need to be locked when destroyed.

    \sa relock()
 */

/*! \class QSystemLock

    \brief The QSystemLock class provides a system wide lock
    that can be used between threads or processes.

    The purpose of a QSystemLocker is to protect an object that can be
    accessed by multiple threads or processes such as shared memory or a file.

    For example, say there is a method which prints a message to a log file:

    void log(const QString &logText)
    {
        QSystemLock systemLock(QLatin1String("logfile"));
        systemLock.lock();
        QFile file(QDir::temp() + QLatin1String("/log"));
        if (file.open(QIODevice::Append)) {
            QTextStream out(&file);
            out << logText;
        }
        systemLock.unlock();
    }

    If this is called from two separate processes the resulting log file is
    guaranteed to contain both lines.

    When you call lock(), other threads or processes that try to call lock()
    with the same key will block until the thread or process that got the lock
    calls unlock().

    A non-blocking alternative to lock() is tryLock().
 */

/*!
    Constructs a new system lock with \a key. The lock is created in an
    unlocked state.

    \sa lock(), key().
 */
QSystemLock::QSystemLock(const QString &key)
{
    d = new QSystemLockPrivate;
    setKey(key);
}

/*!
    Destroys a system lock.

    warning: This will not unlock the system lock if it has been locked.
*/
QSystemLock::~QSystemLock()
{
    d->cleanHandle();
    delete d;
}

/*!
    Sets a new key to this system lock.

    \sa key()
 */
void QSystemLock::setKey(const QString &key)
{
    if (key == d->key)
        return;
    d->cleanHandle();
    d->lockCount = 0;
    d->key = key;
    // cache the file name so it doesn't have to be generated all the time.
    d->fileName = d->makeKeyFileName();
    d->error = QSystemLock::NoError;
    d->errorString = QString();
    d->handle();
}

/*!
    Returns the key assigned to this system lock

    \sa setKey()
 */
QString QSystemLock::key() const
{
    return d->key;
}

/*!
    Locks the system lock.  Lock \a mode can either be ReadOnly or ReadWrite.
    If a mode is ReadOnly, attempts by other processes to obtain
    ReadOnly locks will succeed, and ReadWrite attempts will block until
    all of the ReadOnly locks are unlocked.  If locked as ReadWrite, all
    other attempts to lock will block until the lock is unlocked.  A given
    QSystemLock can be locked multiple times without blocking, and will
    only be unlocked after a corresponding number of unlock()
    calls are made.  Returns true on success; otherwise returns false.

    \sa unlock(), tryLock()
 */
bool QSystemLock::lock(LockMode mode)
{
    if (d->lockCount > 0 && mode == ReadOnly && d->lockedMode == ReadWrite) {
        qWarning() << "QSystemLock::lock readwrite lock on top of readonly lock.";
        return false;
    }
    return d->modifySemaphore(QSystemLockPrivate::Lock, mode);
}

/*!
    Unlocks the system lock.
    Returns true on success; otherwise returns false.

    \sa lock()
  */
bool QSystemLock::unlock()
{
    if (d->lockCount == 0) {
        qWarning() << "QSystemLock::unlock: unlock with no lock.";
        return false;
    }
    return d->modifySemaphore(QSystemLockPrivate::Unlock, d->lockedMode);
}

/*!
    Returns the type of error that occurred last or NoError.

    \sa errorString()
 */
QSystemLock::SystemLockError QSystemLock::error() const
{
    return d->error;
}

/*!
    Returns the human-readable message appropriate to the current error
    reported by error(). If no suitable string is available, an empty
    string is returned.

    \sa error()
 */
QString QSystemLock::errorString() const
{
    return d->errorString;
}

