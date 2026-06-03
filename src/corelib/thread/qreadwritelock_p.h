// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Olivier Goffart <ogoffart@woboq.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QREADWRITELOCK_P_H
#define QREADWRITELOCK_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the implementation.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/private/qlocking_p.h>
#include <QtCore/private/qwaitcondition_p.h>
#include <QtCore/qreadwritelock.h>
#include <QtCore/qvarlengtharray.h>

QT_REQUIRE_CONFIG(thread);

QT_BEGIN_NAMESPACE

class QReadWriteLockPrivate
{
public:
    // export the State constants protected in QReadWriteLock
    static constexpr quintptr StateLockedForRead = QReadWriteLock::StateLockedForRead;
    static constexpr quintptr StateLockedForWrite = QReadWriteLock::StateLockedForWrite;
    static constexpr quintptr StateMask = QReadWriteLock::StateMask;
    static constexpr quintptr MultiplyLocked = QReadWriteLock::Counter;
    static constexpr quintptr IsRecursiveLock = MultiplyLocked << 1;
    static constexpr quintptr RecursivelyLockedForWrite =
            StateLockedForWrite | MultiplyLocked | IsRecursiveLock;

    explicit QReadWriteLockPrivate(bool isRecursive = false)
        : recursive(isRecursive) {}

    alignas(QtPrivate::IdealMutexAlignment) std::condition_variable writerCond;
    std::condition_variable readerCond;

    alignas(QtPrivate::IdealMutexAlignment) std::mutex mutex;
    int readerCount = 0;
    int writerCount = 0;
    int waitingReaders = 0;
    int waitingWriters = 0;
    const bool recursive;

    //Called with the mutex locked
    bool lockForWrite(std::unique_lock<std::mutex> &lock, QDeadlineTimer timeout);
    bool lockForRead(std::unique_lock<std::mutex> &lock, QDeadlineTimer timeout);
    void unlock();

    //memory management
    int id = 0;
    void release();
    static QReadWriteLockPrivate *allocate();

    // Recursive mutex handling
    Qt::HANDLE currentWriter = {};

    struct Reader {
        Qt::HANDLE handle;
        int recursionLevel;
    };

    QVarLengthArray<Reader, 16> currentReaders;

    // called with the mutex unlocked
    bool recursiveLockForWrite(QDeadlineTimer timeout);
    bool recursiveLockForRead(QDeadlineTimer timeout);
    void recursiveUnlock();

    /*!
     * \internal
     * Describes the state of the QReadWriteLock whose private is \a dd.
     *
     * Returns one of:
     * \list
     *   \li 0 (unlocked)
     *   \li \c IsRecursiveLock (still unlocked)
     *   \li \c StateLockedForRead
     *   \li \c{StateLockedForRead | IsRecursiveLock}
     *   \li \c StateLockedForWrite
     *   \li \c{StateLockedForWrite | IsRecursiveLock}
     *   \li \c{StateLockedForWrite | IsRecursiveLock | MultiplyLocked} = \c RecursivelyLockedForWrite
     * \endlist
     */
    static quintptr describeState(void *dd) noexcept
    {
        quintptr u = quintptr(dd);
        if (u < StateMask) {
            // non-recursive; unlocked or uncontended
            return u;
        }

        auto d = static_cast<QReadWriteLockPrivate *>(dd);
        const auto lock = qt_scoped_lock(d->mutex);
        u = 0;
        if (d->writerCount)
            u |= StateLockedForWrite;
        else if (d->readerCount)
            u |= StateLockedForRead;
        if (d->recursive) {
            u |= IsRecursiveLock;
            if (d->writerCount > 1) {
                u |= MultiplyLocked;
                Q_ASSERT(u == RecursivelyLockedForWrite);
            }
        } else {
            // non-recursive, quick check of the state
            Q_ASSERT(d->writerCount <= 1);
        }
        return u;
    }

    // used by QWaitCondition::wait
    template <typename Prep, typename DoWait>
    static bool waitConditionWait(QReadWriteLock *readWriteLock, Prep &&prep, DoWait &&doWait);
};
Q_DECLARE_TYPEINFO(QReadWriteLockPrivate::Reader, Q_PRIMITIVE_TYPE);

template <typename Prep, typename DoWait>
inline bool QReadWriteLockPrivate::waitConditionWait(QReadWriteLock *readWriteLock, Prep &&prep, DoWait &&doWait)
{
    if (!readWriteLock)
        return false;
    auto previousState = describeState(readWriteLock->d_ptr.loadAcquire());
    if (previousState == 0)     // unlocked
        return false;
    if (previousState == RecursivelyLockedForWrite) {
        qWarning("QWaitCondition: cannot wait on QReadWriteLocks with recursive lockForWrite()");
        return false;
    }

    prep();
    readWriteLock->unlock();
    bool returnValue = doWait();

    // relock
    if (previousState & StateLockedForWrite)
        readWriteLock->lockForWrite();
    else
        readWriteLock->lockForRead();

    return returnValue;
}

QT_END_NAMESPACE

#endif // QREADWRITELOCK_P_H
