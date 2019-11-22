/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

// qfutureinterface.h included from qfuture.h
#include "qfuture.h"
#include "qfutureinterface_p.h"

#include <QtCore/qatomic.h>
#include <QtCore/qthread.h>
#include <private/qthreadpool_p.h>

#ifdef interface
#  undef interface
#endif

QT_BEGIN_NAMESPACE

enum {
    MaxProgressEmitsPerSecond = 25
};

namespace {
class ThreadPoolThreadReleaser {
    QThreadPool *m_pool;
public:
    explicit ThreadPoolThreadReleaser(QThreadPool *pool)
        : m_pool(pool)
    { if (pool) pool->releaseThread(); }
    ~ThreadPoolThreadReleaser()
    { if (m_pool) m_pool->reserveThread(); }
};
} // unnamed namespace


QFutureInterfaceBase::QFutureInterfaceBase(State initialState)
    : d(new QFutureInterfaceBasePrivate(initialState))
{ }

QFutureInterfaceBase::QFutureInterfaceBase(const QFutureInterfaceBase &other)
    : d(other.d)
{
    d->refCount.ref();
}

QFutureInterfaceBase::~QFutureInterfaceBase()
{
    if (!d->refCount.deref())
        delete d;
}

static inline int switch_on(QAtomicInt &a, int which)
{
    return a.fetchAndOrRelaxed(which) | which;
}

static inline int switch_off(QAtomicInt &a, int which)
{
    return a.fetchAndAndRelaxed(~which) & ~which;
}

static inline int switch_from_to(QAtomicInt &a, int from, int to)
{
    int newValue;
    int expected = a.loadRelaxed();
    do {
        newValue = (expected & ~from) | to;
    } while (!a.testAndSetRelaxed(expected, newValue, expected));
    return newValue;
}

void QFutureInterfaceBase::cancel()
{
    QMutexLocker locker(&d->m_mutex);
    if (d->state.loadRelaxed() & Canceled)
        return;

    switch_from_to(d->state, Paused, Canceled);
    d->waitCondition.wakeAll();
    d->pausedWaitCondition.wakeAll();
    d->sendCallOut(QFutureCallOutEvent(QFutureCallOutEvent::Canceled));
}

void QFutureInterfaceBase::setPaused(bool paused)
{
    QMutexLocker locker(&d->m_mutex);
    if (paused) {
        switch_on(d->state, Paused);
        d->sendCallOut(QFutureCallOutEvent(QFutureCallOutEvent::Paused));
    } else {
        switch_off(d->state, Paused);
        d->pausedWaitCondition.wakeAll();
        d->sendCallOut(QFutureCallOutEvent(QFutureCallOutEvent::Resumed));
    }
}

void QFutureInterfaceBase::togglePaused()
{
    QMutexLocker locker(&d->m_mutex);
    if (d->state.loadRelaxed() & Paused) {
        switch_off(d->state, Paused);
        d->pausedWaitCondition.wakeAll();
        d->sendCallOut(QFutureCallOutEvent(QFutureCallOutEvent::Resumed));
    } else {
        switch_on(d->state, Paused);
        d->sendCallOut(QFutureCallOutEvent(QFutureCallOutEvent::Paused));
    }
}

void QFutureInterfaceBase::setThrottled(bool enable)
{
    QMutexLocker lock(&d->m_mutex);
    if (enable) {
        switch_on(d->state, Throttled);
    } else {
        switch_off(d->state, Throttled);
        if (!(d->state.loadRelaxed() & Paused))
            d->pausedWaitCondition.wakeAll();
    }
}


bool QFutureInterfaceBase::isRunning() const
{
    return queryState(Running);
}

bool QFutureInterfaceBase::isStarted() const
{
    return queryState(Started);
}

bool QFutureInterfaceBase::isCanceled() const
{
    return queryState(Canceled);
}

bool QFutureInterfaceBase::isFinished() const
{
    return queryState(Finished);
}

bool QFutureInterfaceBase::isPaused() const
{
    return queryState(Paused);
}

bool QFutureInterfaceBase::isThrottled() const
{
    return queryState(Throttled);
}

bool QFutureInterfaceBase::isResultReadyAt(int index) const
{
    QMutexLocker lock(&d->m_mutex);
    return d->internal_isResultReadyAt(index);
}

bool QFutureInterfaceBase::waitForNextResult()
{
    QMutexLocker lock(&d->m_mutex);
    return d->internal_waitForNextResult();
}

void QFutureInterfaceBase::waitForResume()
{
    // return early if possible to avoid taking the mutex lock.
    {
        const int state = d->state.loadRelaxed();
        if (!(state & Paused) || (state & Canceled))
            return;
    }

    QMutexLocker lock(&d->m_mutex);
    const int state = d->state.loadRelaxed();
    if (!(state & Paused) || (state & Canceled))
        return;

    // decrease active thread count since this thread will wait.
    const ThreadPoolThreadReleaser releaser(d->pool());

    d->pausedWaitCondition.wait(&d->m_mutex);
}

int QFutureInterfaceBase::progressValue() const
{
    const QMutexLocker lock(&d->m_mutex);
    return d->m_progressValue;
}

int QFutureInterfaceBase::progressMinimum() const
{
    const QMutexLocker lock(&d->m_mutex);
    return d->m_progressMinimum;
}

int QFutureInterfaceBase::progressMaximum() const
{
    const QMutexLocker lock(&d->m_mutex);
    return d->m_progressMaximum;
}

int QFutureInterfaceBase::resultCount() const
{
    QMutexLocker lock(&d->m_mutex);
    return d->internal_resultCount();
}

QString QFutureInterfaceBase::progressText() const
{
    QMutexLocker locker(&d->m_mutex);
    return d->m_progressText;
}

bool QFutureInterfaceBase::isProgressUpdateNeeded() const
{
    QMutexLocker locker(&d->m_mutex);
    return !d->progressTime.isValid() || (d->progressTime.elapsed() > (1000 / MaxProgressEmitsPerSecond));
}

void QFutureInterfaceBase::reportStarted()
{
    QMutexLocker locker(&d->m_mutex);
    if (d->state.loadRelaxed() & (Started|Canceled|Finished))
        return;

    d->setState(State(Started | Running));
    d->sendCallOut(QFutureCallOutEvent(QFutureCallOutEvent::Started));
}

void QFutureInterfaceBase::reportCanceled()
{
    cancel();
}

#ifndef QT_NO_EXCEPTIONS
void QFutureInterfaceBase::reportException(const QException &exception)
{
    QMutexLocker locker(&d->m_mutex);
    if (d->state.loadRelaxed() & (Canceled|Finished))
        return;

    d->m_exceptionStore.setException(exception);
    switch_on(d->state, Canceled);
    d->waitCondition.wakeAll();
    d->pausedWaitCondition.wakeAll();
    d->sendCallOut(QFutureCallOutEvent(QFutureCallOutEvent::Canceled));
}
#endif

void QFutureInterfaceBase::reportFinished()
{
    QMutexLocker locker(&d->m_mutex);
    if (!isFinished()) {
        switch_from_to(d->state, Running, Finished);
        d->waitCondition.wakeAll();
        d->sendCallOut(QFutureCallOutEvent(QFutureCallOutEvent::Finished));
    }
}

void QFutureInterfaceBase::setExpectedResultCount(int resultCount)
{
    if (d->manualProgress == false)
        setProgressRange(0, resultCount);
    d->m_expectedResultCount = resultCount;
}

int QFutureInterfaceBase::expectedResultCount()
{
    return d->m_expectedResultCount;
}

bool QFutureInterfaceBase::queryState(State state) const
{
    return d->state.loadRelaxed() & state;
}

void QFutureInterfaceBase::waitForResult(int resultIndex)
{
    d->m_exceptionStore.throwPossibleException();

    QMutexLocker lock(&d->m_mutex);
    if (!isRunning())
        return;
    lock.unlock();

    // To avoid deadlocks and reduce the number of threads used, try to
    // run the runnable in the current thread.
    d->pool()->d_func()->stealAndRunRunnable(d->runnable);

    lock.relock();

    const int waitIndex = (resultIndex == -1) ? INT_MAX : resultIndex;
    while (isRunning() && !d->internal_isResultReadyAt(waitIndex))
        d->waitCondition.wait(&d->m_mutex);

    d->m_exceptionStore.throwPossibleException();
}

void QFutureInterfaceBase::waitForFinished()
{
    QMutexLocker lock(&d->m_mutex);
    const bool alreadyFinished = !isRunning();
    lock.unlock();

    if (!alreadyFinished) {
        d->pool()->d_func()->stealAndRunRunnable(d->runnable);

        lock.relock();

        while (isRunning())
            d->waitCondition.wait(&d->m_mutex);
    }

    d->m_exceptionStore.throwPossibleException();
}

void QFutureInterfaceBase::reportResultsReady(int beginIndex, int endIndex)
{
    if (beginIndex == endIndex || (d->state.loadRelaxed() & (Canceled|Finished)))
        return;

    d->waitCondition.wakeAll();

    if (d->manualProgress == false) {
        if (d->internal_updateProgress(d->m_progressValue + endIndex - beginIndex) == false) {
            d->sendCallOut(QFutureCallOutEvent(QFutureCallOutEvent::ResultsReady,
                                               beginIndex,
                                               endIndex));
            return;
        }

        d->sendCallOuts(QFutureCallOutEvent(QFutureCallOutEvent::Progress,
                                            d->m_progressValue,
                                            d->m_progressText),
                        QFutureCallOutEvent(QFutureCallOutEvent::ResultsReady,
                                            beginIndex,
                                            endIndex));
        return;
    }
    d->sendCallOut(QFutureCallOutEvent(QFutureCallOutEvent::ResultsReady, beginIndex, endIndex));
}

void QFutureInterfaceBase::setRunnable(QRunnable *runnable)
{
    d->runnable = runnable;
}

void QFutureInterfaceBase::setThreadPool(QThreadPool *pool)
{
    d->m_pool = pool;
}

void QFutureInterfaceBase::setFilterMode(bool enable)
{
    QMutexLocker locker(&d->m_mutex);
    resultStoreBase().setFilterMode(enable);
}

void QFutureInterfaceBase::setProgressRange(int minimum, int maximum)
{
    QMutexLocker locker(&d->m_mutex);
    d->m_progressMinimum = minimum;
    d->m_progressMaximum = maximum;
    d->sendCallOut(QFutureCallOutEvent(QFutureCallOutEvent::ProgressRange, minimum, maximum));
}

void QFutureInterfaceBase::setProgressValue(int progressValue)
{
    setProgressValueAndText(progressValue, QString());
}

void QFutureInterfaceBase::setProgressValueAndText(int progressValue,
                                                   const QString &progressText)
{
    QMutexLocker locker(&d->m_mutex);
    if (d->manualProgress == false)
        d->manualProgress = true;
    if (d->m_progressValue >= progressValue)
        return;

    if (d->state.loadRelaxed() & (Canceled|Finished))
        return;

    if (d->internal_updateProgress(progressValue, progressText)) {
        d->sendCallOut(QFutureCallOutEvent(QFutureCallOutEvent::Progress,
                                           d->m_progressValue,
                                           d->m_progressText));
    }
}

QMutex *QFutureInterfaceBase::mutex() const
{
    return &d->m_mutex;
}

QMutex &QFutureInterfaceBase::mutex(int) const
{
    return d->m_mutex;
}

QtPrivate::ExceptionStore &QFutureInterfaceBase::exceptionStore()
{
    return d->m_exceptionStore;
}

QtPrivate::ResultStoreBase &QFutureInterfaceBase::resultStoreBase()
{
    return d->m_results;
}

const QtPrivate::ResultStoreBase &QFutureInterfaceBase::resultStoreBase() const
{
    return d->m_results;
}

QFutureInterfaceBase &QFutureInterfaceBase::operator=(const QFutureInterfaceBase &other)
{
    other.d->refCount.ref();
    if (!d->refCount.deref())
        delete d;
    d = other.d;
    return *this;
}

bool QFutureInterfaceBase::refT() const
{
    return d->refCount.refT();
}

bool QFutureInterfaceBase::derefT() const
{
    return d->refCount.derefT();
}

QFutureInterfaceBasePrivate::QFutureInterfaceBasePrivate(QFutureInterfaceBase::State initialState)
    : refCount(1), m_progressValue(0), m_progressMinimum(0), m_progressMaximum(0),
      state(initialState),
      manualProgress(false), m_expectedResultCount(0), runnable(nullptr), m_pool(nullptr)
{
    progressTime.invalidate();
}

int QFutureInterfaceBasePrivate::internal_resultCount() const
{
    return m_results.count(); // ### subtract canceled results.
}

bool QFutureInterfaceBasePrivate::internal_isResultReadyAt(int index) const
{
    return (m_results.contains(index));
}

bool QFutureInterfaceBasePrivate::internal_waitForNextResult()
{
    if (m_results.hasNextResult())
        return true;

    while ((state.loadRelaxed() & QFutureInterfaceBase::Running) && m_results.hasNextResult() == false)
        waitCondition.wait(&m_mutex);

    return !(state.loadRelaxed() & QFutureInterfaceBase::Canceled) && m_results.hasNextResult();
}

bool QFutureInterfaceBasePrivate::internal_updateProgress(int progress,
                                                          const QString &progressText)
{
    if (m_progressValue >= progress)
        return false;

    m_progressValue = progress;
    m_progressText = progressText;

    if (progressTime.isValid() && m_progressValue != m_progressMaximum) // make sure the first and last steps are emitted.
        if (progressTime.elapsed() < (1000 / MaxProgressEmitsPerSecond))
            return false;

    progressTime.start();
    return true;
}

void QFutureInterfaceBasePrivate::internal_setThrottled(bool enable)
{
    // bail out if we are not changing the state
    if ((enable && (state.loadRelaxed() & QFutureInterfaceBase::Throttled))
        || (!enable && !(state.loadRelaxed() & QFutureInterfaceBase::Throttled)))
        return;

    // change the state
    if (enable) {
        switch_on(state, QFutureInterfaceBase::Throttled);
    } else {
        switch_off(state, QFutureInterfaceBase::Throttled);
        if (!(state.loadRelaxed() & QFutureInterfaceBase::Paused))
            pausedWaitCondition.wakeAll();
    }
}

void QFutureInterfaceBasePrivate::sendCallOut(const QFutureCallOutEvent &callOutEvent)
{
    if (outputConnections.isEmpty())
        return;

    for (int i = 0; i < outputConnections.count(); ++i)
        outputConnections.at(i)->postCallOutEvent(callOutEvent);
}

void QFutureInterfaceBasePrivate::sendCallOuts(const QFutureCallOutEvent &callOutEvent1,
                                     const QFutureCallOutEvent &callOutEvent2)
{
    if (outputConnections.isEmpty())
        return;

    for (int i = 0; i < outputConnections.count(); ++i) {
        QFutureCallOutInterface *interface = outputConnections.at(i);
        interface->postCallOutEvent(callOutEvent1);
        interface->postCallOutEvent(callOutEvent2);
    }
}

// This function connects an output interface (for example a QFutureWatcher)
// to this future. While holding the lock we check the state and ready results
// and add the appropriate callouts to the queue. In order to avoid deadlocks,
// the actual callouts are made at the end while not holding the lock.
void QFutureInterfaceBasePrivate::connectOutputInterface(QFutureCallOutInterface *interface)
{
    QMutexLocker locker(&m_mutex);

    if (state.loadRelaxed() & QFutureInterfaceBase::Started) {
        interface->postCallOutEvent(QFutureCallOutEvent(QFutureCallOutEvent::Started));
        interface->postCallOutEvent(QFutureCallOutEvent(QFutureCallOutEvent::ProgressRange,
                                                        m_progressMinimum,
                                                        m_progressMaximum));
        interface->postCallOutEvent(QFutureCallOutEvent(QFutureCallOutEvent::Progress,
                                                        m_progressValue,
                                                        m_progressText));
    }

    QtPrivate::ResultIteratorBase it = m_results.begin();
    while (it != m_results.end()) {
        const int begin = it.resultIndex();
        const int end = begin + it.batchSize();
        interface->postCallOutEvent(QFutureCallOutEvent(QFutureCallOutEvent::ResultsReady,
                                                        begin,
                                                        end));
        it.batchedAdvance();
    }

    if (state.loadRelaxed() & QFutureInterfaceBase::Paused)
        interface->postCallOutEvent(QFutureCallOutEvent(QFutureCallOutEvent::Paused));

    if (state.loadRelaxed() & QFutureInterfaceBase::Canceled)
        interface->postCallOutEvent(QFutureCallOutEvent(QFutureCallOutEvent::Canceled));

    if (state.loadRelaxed() & QFutureInterfaceBase::Finished)
        interface->postCallOutEvent(QFutureCallOutEvent(QFutureCallOutEvent::Finished));

    outputConnections.append(interface);
}

void QFutureInterfaceBasePrivate::disconnectOutputInterface(QFutureCallOutInterface *interface)
{
    QMutexLocker lock(&m_mutex);
    const int index = outputConnections.indexOf(interface);
    if (index == -1)
        return;
    outputConnections.removeAt(index);

    interface->callOutInterfaceDisconnected();
}

void QFutureInterfaceBasePrivate::setState(QFutureInterfaceBase::State newState)
{
    state.storeRelaxed(newState);
}

QT_END_NAMESPACE
