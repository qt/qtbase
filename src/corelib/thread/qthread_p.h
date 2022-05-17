// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTHREAD_P_H
#define QTHREAD_P_H

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
//

#include "qplatformdefs.h"
#include "QtCore/qthread.h"
#include "QtCore/qmutex.h"
#include "QtCore/qstack.h"
#if QT_CONFIG(thread)
#include "QtCore/qwaitcondition.h"
#endif
#include "QtCore/qmap.h"
#include "QtCore/qcoreapplication.h"
#include "private/qobject_p.h"

#include <algorithm>
#include <atomic>

QT_BEGIN_NAMESPACE

class QAbstractEventDispatcher;
class QEventLoop;

class QPostEvent
{
public:
    QObject *receiver;
    QEvent *event;
    int priority;
    inline QPostEvent()
        : receiver(nullptr), event(nullptr), priority(0)
    { }
    inline QPostEvent(QObject *r, QEvent *e, int p)
        : receiver(r), event(e), priority(p)
    { }
};
Q_DECLARE_TYPEINFO(QPostEvent, Q_RELOCATABLE_TYPE);

inline bool operator<(const QPostEvent &first, const QPostEvent &second)
{
    return first.priority > second.priority;
}

// This class holds the list of posted events.
//  The list has to be kept sorted by priority
class QPostEventList : public QList<QPostEvent>
{
public:
    // recursion == recursion count for sendPostedEvents()
    int recursion;

    // sendOffset == the current event to start sending
    qsizetype startOffset;
    // insertionOffset == set by sendPostedEvents to tell postEvent() where to start insertions
    qsizetype insertionOffset;

    QMutex mutex;

    inline QPostEventList() : QList<QPostEvent>(), recursion(0), startOffset(0), insertionOffset(0) { }

    void addEvent(const QPostEvent &ev)
    {
        int priority = ev.priority;
        if (isEmpty() ||
            constLast().priority >= priority ||
            insertionOffset >= size()) {
            // optimization: we can simply append if the last event in
            // the queue has higher or equal priority
            append(ev);
        } else {
            // insert event in descending priority order, using upper
            // bound for a given priority (to ensure proper ordering
            // of events with the same priority)
            QPostEventList::iterator at = std::upper_bound(begin() + insertionOffset, end(), ev);
            insert(at, ev);
        }
    }

private:
    //hides because they do not keep that list sorted. addEvent must be used
    using QList<QPostEvent>::append;
    using QList<QPostEvent>::insert;
};

#if QT_CONFIG(thread)

class Q_CORE_EXPORT QDaemonThread : public QThread
{
public:
    QDaemonThread(QObject *parent = nullptr);
    ~QDaemonThread();
};

class QThreadPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QThread)

public:
    QThreadPrivate(QThreadData *d = nullptr);
    ~QThreadPrivate();

    void setPriority(QThread::Priority prio);

    mutable QMutex mutex;
    QAtomicInt quitLockRef;

    bool running;
    bool finished;
    bool isInFinish; //when in QThreadPrivate::finish
    std::atomic<bool> interruptionRequested;

    bool exited;
    int returnCode;

    uint stackSize;
    std::underlying_type_t<QThread::Priority> priority;

#ifdef Q_OS_UNIX
    QWaitCondition thread_done;

    static void *start(void *arg);
    static void finish(void *);

#endif // Q_OS_UNIX

#ifdef Q_OS_WIN
    static unsigned int __stdcall start(void *) noexcept;
    static void finish(void *, bool lockAnyway = true) noexcept;

    Qt::HANDLE handle;
    unsigned int id;
    int waiters;
    bool terminationEnabled, terminatePending;
#endif // Q_OS_WIN
#ifdef Q_OS_WASM
    static int idealThreadCount;
#endif
    QThreadData *data;

    static QAbstractEventDispatcher *createEventDispatcher(QThreadData *data);

    void ref()
    {
        quitLockRef.ref();
    }

    void deref()
    {
        if (!quitLockRef.deref() && running) {
            QCoreApplication::instance()->postEvent(q_ptr, new QEvent(QEvent::Quit));
        }
    }

    QBindingStatus *bindingStatus()
    {
        auto statusOrPendingObjects = m_statusOrPendingObjects.loadAcquire();
        if (!(statusOrPendingObjects & 1))
            return (QBindingStatus *) statusOrPendingObjects;
        return nullptr;
    }

    void addObjectWithPendingBindingStatusChange(QObject *obj)
    {
        Q_ASSERT(!bindingStatus());
        auto pendingObjects = pendingObjectsWithBindingStatusChange();
        if (!pendingObjects) {
            pendingObjects = new std::vector<QObject *>();
            m_statusOrPendingObjects = quintptr(pendingObjects) | 1;
        }
        pendingObjects->push_back(obj);
    }

    std::vector<QObject *> *pendingObjectsWithBindingStatusChange()
    {
        auto statusOrPendingObjects = m_statusOrPendingObjects.loadAcquire();
        if (statusOrPendingObjects & 1)
            return reinterpret_cast<std::vector<QObject *> *>(statusOrPendingObjects - 1);
        return nullptr;
    }

    QAtomicInteger<quintptr> m_statusOrPendingObjects = 0;
#ifndef Q_OS_INTEGRITY
private:
    // Used in QThread(Private)::start to avoid racy access to QObject::objectName,
    // unset afterwards. On INTEGRITY we set the thread name before starting it.
    QString objectName;
#endif
};

#else // QT_CONFIG(thread)

class QThreadPrivate : public QObjectPrivate
{
public:
    QThreadPrivate(QThreadData *d = nullptr);
    ~QThreadPrivate();

    mutable QMutex mutex;
    QThreadData *data;
    QBindingStatus* m_bindingStatus;
    bool running = false;

    QBindingStatus* bindingStatus() { return m_bindingStatus; }
    void addObjectWithPendingBindingStatusChange(QObject *) {}
    std::vector<QObject *> * pendingObjectsWithBindingStatusChange() { return nullptr; }

    static void setCurrentThread(QThread *) { }
    static QAbstractEventDispatcher *createEventDispatcher(QThreadData *data);

    void ref() {}
    void deref() {}

    Q_DECLARE_PUBLIC(QThread)
};

#endif // QT_CONFIG(thread)

class QThreadData
{
public:
    QThreadData(int initialRefCount = 1);
    ~QThreadData();

    static Q_AUTOTEST_EXPORT QThreadData *current(bool createIfNecessary = true);
    static void clearCurrentThreadData();
    static QThreadData *get2(QThread *thread)
    { Q_ASSERT_X(thread != nullptr, "QThread", "internal error"); return thread->d_func()->data; }


    void ref();
    void deref();
    inline bool hasEventDispatcher() const
    { return eventDispatcher.loadRelaxed() != nullptr; }
    QAbstractEventDispatcher *createEventDispatcher();
    QAbstractEventDispatcher *ensureEventDispatcher()
    {
        QAbstractEventDispatcher *ed = eventDispatcher.loadRelaxed();
        if (Q_LIKELY(ed))
            return ed;
        return createEventDispatcher();
    }

    bool canWaitLocked()
    {
        QMutexLocker locker(&postEventList.mutex);
        return canWait;
    }

private:
    QAtomicInt _ref;

public:
    int loopLevel;
    int scopeLevel;

    QStack<QEventLoop *> eventLoops;
    QPostEventList postEventList;
    QAtomicPointer<QThread> thread;
    QAtomicPointer<void> threadId;
    QAtomicPointer<QAbstractEventDispatcher> eventDispatcher;
    QList<void *> tls;

    bool quitNow;
    bool canWait;
    bool isAdopted;
    bool requiresCoreApplication;
};

class QScopedScopeLevelCounter
{
    QThreadData *threadData;
public:
    inline QScopedScopeLevelCounter(QThreadData *threadData)
        : threadData(threadData)
    { ++threadData->scopeLevel; }
    inline ~QScopedScopeLevelCounter()
    { --threadData->scopeLevel; }
};

// thread wrapper for the main() thread
class QAdoptedThread : public QThread
{
    Q_DECLARE_PRIVATE(QThread)

public:
    QAdoptedThread(QThreadData *data = nullptr);
    ~QAdoptedThread();
    void init();

private:
#if QT_CONFIG(thread)
    void run() override;
#endif
};

QT_END_NAMESPACE

#endif // QTHREAD_P_H
