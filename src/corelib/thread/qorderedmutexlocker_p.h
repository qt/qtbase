/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QORDEREDMUTEXLOCKER_P_H
#define QORDEREDMUTEXLOCKER_P_H

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
#include <QtCore/qmutex.h>

#include <functional>

QT_BEGIN_NAMESPACE

#if QT_CONFIG(thread)

/*
  Locks 2 mutexes in a defined order, avoiding a recursive lock if
  we're trying to lock the same mutex twice.
*/
class QOrderedMutexLocker
{
public:
    QOrderedMutexLocker(QBasicMutex *m1, QBasicMutex *m2)
        : mtx1((m1 == m2) ? m1 : (std::less<QBasicMutex *>()(m1, m2) ? m1 : m2)),
          mtx2((m1 == m2) ?  nullptr : (std::less<QBasicMutex *>()(m1, m2) ? m2 : m1)),
          locked(false)
    {
        relock();
    }

    Q_DISABLE_COPY(QOrderedMutexLocker)

    void swap(QOrderedMutexLocker &other) noexcept
    {
        qSwap(this->mtx1, other.mtx1);
        qSwap(this->mtx2, other.mtx2);
        qSwap(this->locked, other.locked);
    }

    QOrderedMutexLocker  &operator=(QOrderedMutexLocker &&other) noexcept {
        QOrderedMutexLocker moved(std::move(other));
        swap(moved);
        return *this;
    }

    QOrderedMutexLocker(QOrderedMutexLocker &&other) noexcept
        : mtx1(qExchange(other.mtx1, nullptr))
        , mtx2(qExchange(other.mtx2, nullptr))
        , locked(qExchange(other.locked, false))
    {}

    ~QOrderedMutexLocker()
    {
        unlock();
    }

    void relock()
    {
        if (!locked) {
            if (mtx1) mtx1->lock();
            if (mtx2) mtx2->lock();
            locked = true;
        }
    }

    /*!
        \internal
        Can be called if the mutexes have been unlocked manually, and sets the
        state of the QOrderedMutexLocker to unlocked.
        The caller is expected to have unlocked both of them if they
        are not the same. Calling this method when the QOrderedMutexLocker is
        unlocked or when the provided mutexes have not actually been unlocked is
        UB.
     */
    void dismiss()
    {
        Q_ASSERT(locked);
        locked = false;
    }

    void unlock()
    {
        if (locked) {
            if (mtx2) mtx2->unlock();
            if (mtx1) mtx1->unlock();
            locked = false;
        }
    }

    static bool relock(QBasicMutex *mtx1, QBasicMutex *mtx2)
    {
        // mtx1 is already locked, mtx2 not... do we need to unlock and relock?
        if (mtx1 == mtx2)
            return false;
        if (std::less<QBasicMutex *>()(mtx1, mtx2)) {
            mtx2->lock();
            return true;
        }
        if (!mtx2->tryLock()) {
            mtx1->unlock();
            mtx2->lock();
            mtx1->lock();
        }
        return true;
    }

private:
    QBasicMutex *mtx1, *mtx2;
    bool locked;
};

class QBasicMutexLocker
{
public:
    inline explicit QBasicMutexLocker(QBasicMutex *m) QT_MUTEX_LOCK_NOEXCEPT
        : m(m), isLocked(true)
    {
        m->lock();
    }
    inline ~QBasicMutexLocker() { if (isLocked) unlock(); }

    inline void unlock() noexcept
    {
        isLocked = false;
        m->unlock();
    }

    inline void relock() QT_MUTEX_LOCK_NOEXCEPT
    {
        isLocked = true;
        m->lock();
    }

private:
    Q_DISABLE_COPY(QBasicMutexLocker)

    QBasicMutex *m;
    bool isLocked;
};

#else

class QOrderedMutexLocker
{
public:
    Q_DISABLE_COPY(QOrderedMutexLocker)
    QOrderedMutexLocker(QBasicMutex *, QBasicMutex *) {}
    QOrderedMutexLocker(QOrderedMutexLocker &&) = default;
    QOrderedMutexLocker& operator=(QOrderedMutexLocker &&other) = default;
    ~QOrderedMutexLocker() {}

    void relock() {}
    void unlock() {}
    void dismiss() {}

    static bool relock(QBasicMutex *, QBasicMutex *) { return false; }
};

using QBasicMutexLocker = QMutexLocker;

#endif


QT_END_NAMESPACE

#endif
