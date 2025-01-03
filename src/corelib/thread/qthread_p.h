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
// It's used in a virtual in QCoreApplication, so ELFVERSION:ignore-next
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

    void addEvent(const QPostEvent &ev);

private:
    //hides because they do not keep that list sorted. addEvent must be used
    using QList<QPostEvent>::append;
    using QList<QPostEvent>::insert;
};

namespace QtPrivate {

/* BindingStatusOrList is basically a QBiPointer (as found in declarative)
   with some helper methods to manipulate the list. BindingStatusOrList starts
   its life in a null state and supports the following transitions

                        0 state (initial)
                       /                \
                      /                  \
                     v                    v
             pending object list----------->binding status
    Note that binding status is the final state, and we never transition away
    from it
*/
class BindingStatusOrList
{
    Q_DISABLE_COPY_MOVE(BindingStatusOrList)
public:
    using List = std::vector<QObject *>;

    constexpr BindingStatusOrList() noexcept : data(0) {}
    explicit BindingStatusOrList(QBindingStatus *status) noexcept :
        data(encodeBindingStatus(status)) {}
    explicit BindingStatusOrList(List *list) noexcept : data(encodeList(list)) {}

    // requires external synchronization:
    QBindingStatus *addObjectUnlessAlreadyStatus(QObject *object);
    void removeObject(QObject *object);
    void setStatusAndClearList(QBindingStatus *status) noexcept;


    static bool isBindingStatus(quintptr data) noexcept
    {
        return !isNull(data) && !isList(data);
    }
    static bool isList(quintptr data) noexcept { return data & 1; }
    static bool isNull(quintptr data) noexcept { return data == 0; }

    // thread-safe:
    QBindingStatus *bindingStatus() const noexcept
    {
        // synchronizes-with the store-release in setStatusAndClearList():
        const auto d = data.load(std::memory_order_acquire);
        if (isBindingStatus(d))
            return reinterpret_cast<QBindingStatus *>(d);
        else
            return nullptr;
    }

    // requires external synchronization:
    List *list() const noexcept
    {
        return decodeList(data.load(std::memory_order_relaxed));
    }

private:
    static List *decodeList(quintptr ptr) noexcept
    {
        if (isList(ptr))
            return reinterpret_cast<List *>(ptr & ~1);
        else
            return nullptr;
    }

    static quintptr encodeBindingStatus(QBindingStatus *status) noexcept
    {
        return quintptr(status);
    }

    static quintptr encodeList(List *list) noexcept
    {
        return quintptr(list) | 1;
    }

    std::atomic<quintptr> data;
};

} // namespace QtPrivate

#if QT_CONFIG(thread)

class Q_CORE_EXPORT QDaemonThread : public QThread
{
public:
    QDaemonThread(QObject *parent = nullptr);
    ~QDaemonThread();
};

class Q_AUTOTEST_EXPORT QThreadPrivate : public QObjectPrivate
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
        return m_statusOrPendingObjects.bindingStatus();
    }

    /* Returns nullptr if the object has been added, or the binding status
       if that one has been set in the meantime
    */
    QBindingStatus *addObjectWithPendingBindingStatusChange(QObject *obj);
    void removeObjectWithPendingBindingStatusChange(QObject *obj);

    // manipulating m_statusOrPendingObjects requires mutex to be locked
    QtPrivate::BindingStatusOrList m_statusOrPendingObjects = {};
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
    QBindingStatus *addObjectWithPendingBindingStatusChange(QObject *) { return nullptr; }
    void removeObjectWithPendingBindingStatusChange(QObject *) {}

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
