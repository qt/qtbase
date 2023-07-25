// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

// qfutureinterface.h included from qfuture.h
#include "qfuture.h"
#include "qfutureinterface_p.h"

#include <QtCore/qatomic.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qthread.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/private/qsimd_p.h> // for qYieldCpu()
#include <private/qthreadpool_p.h>
#include <private/qobject_p.h>

#ifdef interface
#  undef interface
#endif

// GCC 12 gets confused about QFutureInterfaceBase::state, for some non-obvious
// reason
//  warning: ‘unsigned int __atomic_or_fetch_4(volatile void*, unsigned int, int)’ writing 4 bytes into a region of size 0 overflows the destination [-Wstringop-overflow=]
QT_WARNING_DISABLE_GCC("-Wstringop-overflow")

QT_BEGIN_NAMESPACE

enum {
    MaxProgressEmitsPerSecond = 25
};

namespace {
class ThreadPoolThreadReleaser {
    QThreadPool *m_pool;
public:
    Q_NODISCARD_CTOR
    explicit ThreadPoolThreadReleaser(QThreadPool *pool)
        : m_pool(pool)
    { if (pool) pool->releaseThread(); }
    ~ThreadPoolThreadReleaser()
    { if (m_pool) m_pool->reserveThread(); }
};

const auto suspendingOrSuspended =
        QFutureInterfaceBase::Suspending | QFutureInterfaceBase::Suspended;

} // unnamed namespace

class QBasicFutureWatcher : public QObject, QFutureCallOutInterface
{
    Q_OBJECT
public:
    explicit QBasicFutureWatcher(QObject *parent = nullptr);
    ~QBasicFutureWatcher() override;

    void setFuture(QFutureInterfaceBase &fi);

    bool event(QEvent *event) override;

Q_SIGNALS:
    void finished();

private:
    QFutureInterfaceBase future;

    void postCallOutEvent(const QFutureCallOutEvent &event) override;
    void callOutInterfaceDisconnected() override;
};

void QBasicFutureWatcher::postCallOutEvent(const QFutureCallOutEvent &event)
{
    if (thread() == QThread::currentThread()) {
        // If we are in the same thread, don't queue up anything.
        std::unique_ptr<QFutureCallOutEvent> clonedEvent(event.clone());
        QCoreApplication::sendEvent(this, clonedEvent.get());
    } else {
        QCoreApplication::postEvent(this, event.clone());
    }
}

void QBasicFutureWatcher::callOutInterfaceDisconnected()
{
    QCoreApplication::removePostedEvents(this, QEvent::FutureCallOut);
}

/*
 * QBasicFutureWatcher is a more lightweight version of QFutureWatcher for internal use
 */
QBasicFutureWatcher::QBasicFutureWatcher(QObject *parent)
    : QObject(parent)
{
}

QBasicFutureWatcher::~QBasicFutureWatcher()
{
    future.d->disconnectOutputInterface(this);
}

void QBasicFutureWatcher::setFuture(QFutureInterfaceBase &fi)
{
    future = fi;
    future.d->connectOutputInterface(this);
}

bool QBasicFutureWatcher::event(QEvent *event)
{
    if (event->type() == QEvent::FutureCallOut) {
        QFutureCallOutEvent *callOutEvent = static_cast<QFutureCallOutEvent *>(event);
        if (callOutEvent->callOutType == QFutureCallOutEvent::Finished)
            emit finished();
        return true;
    }
    return QObject::event(event);
}

void QtPrivate::watchContinuationImpl(const QObject *context, QSlotObjectBase *slotObj,
                                      QFutureInterfaceBase &fi)
{
    Q_ASSERT(context);
    Q_ASSERT(slotObj);

    auto slot = SlotObjUniquePtr(slotObj);

    auto *watcher = new QBasicFutureWatcher;
    watcher->moveToThread(context->thread());
    // ### we're missing a convenient way to `QObject::connect()` to a `QSlotObjectBase`...
    QObject::connect(watcher, &QBasicFutureWatcher::finished,
                     // for the following, cf. QMetaObject::invokeMethodImpl():
                     // we know `slot` is a lambda returning `void`, so we can just
                     // `call()` with `obj` and `args[0]` set to `nullptr`:
                     watcher, [slot = std::move(slot)] {
                         void *args[] = { nullptr }; // for `void` return value
                         slot->call(nullptr, args);
                     });
    QObject::connect(watcher, &QBasicFutureWatcher::finished,
                     watcher, &QObject::deleteLater);
    QObject::connect(context, &QObject::destroyed,
                     watcher, &QObject::deleteLater);
    watcher->setFuture(fi);
}

QFutureCallOutInterface::~QFutureCallOutInterface()
    = default;

Q_IMPL_EVENT_COMMON(QFutureCallOutEvent)

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
    if (d && !d->refCount.deref())
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
    const auto adjusted = [&](int old) { return (old & ~from) | to; };
    int value = a.loadRelaxed();
    while (!a.testAndSetRelaxed(value, adjusted(value), value))
        qYieldCpu();
    return value;
}

void QFutureInterfaceBase::cancel()
{
    cancel(CancelMode::CancelOnly);
}

void QFutureInterfaceBase::cancel(QFutureInterfaceBase::CancelMode mode)
{
    QMutexLocker locker(&d->m_mutex);

    const auto oldState = d->state.loadRelaxed();

    switch (mode) {
    case CancelMode::CancelAndFinish:
        if ((oldState & Finished) && (oldState & Canceled))
            return;
        switch_from_to(d->state, suspendingOrSuspended | Running, Canceled | Finished);
        break;
    case CancelMode::CancelOnly:
        if (oldState & Canceled)
            return;
        switch_from_to(d->state, suspendingOrSuspended, Canceled);
        break;
    }

    // Cancel the continuations chain
    QFutureInterfaceBasePrivate *next = d->continuationData;
    while (next) {
        next->continuationState = QFutureInterfaceBasePrivate::Canceled;
        next = next->continuationData;
    }

    d->waitCondition.wakeAll();
    d->pausedWaitCondition.wakeAll();

    if (!(oldState & Canceled))
        d->sendCallOut(QFutureCallOutEvent(QFutureCallOutEvent::Canceled));
    if (mode == CancelMode::CancelAndFinish && !(oldState & Finished))
        d->sendCallOut(QFutureCallOutEvent(QFutureCallOutEvent::Finished));

    d->isValid = false;
}

void QFutureInterfaceBase::setSuspended(bool suspend)
{
    QMutexLocker locker(&d->m_mutex);
    if (suspend) {
        switch_on(d->state, Suspending);
        d->sendCallOut(QFutureCallOutEvent(QFutureCallOutEvent::Suspending));
    } else {
        switch_off(d->state, suspendingOrSuspended);
        d->pausedWaitCondition.wakeAll();
        d->sendCallOut(QFutureCallOutEvent(QFutureCallOutEvent::Resumed));
    }
}

void QFutureInterfaceBase::toggleSuspended()
{
    QMutexLocker locker(&d->m_mutex);
    if (d->state.loadRelaxed() & suspendingOrSuspended) {
        switch_off(d->state, suspendingOrSuspended);
        d->pausedWaitCondition.wakeAll();
        d->sendCallOut(QFutureCallOutEvent(QFutureCallOutEvent::Resumed));
    } else {
        switch_on(d->state, Suspending);
        d->sendCallOut(QFutureCallOutEvent(QFutureCallOutEvent::Suspending));
    }
}

void QFutureInterfaceBase::reportSuspended() const
{
    // Needs to be called when pause is in effect,
    // i.e. no more events will be reported.

    QMutexLocker locker(&d->m_mutex);
    const int state = d->state.loadRelaxed();
    if (!(state & Suspending) || (state & Suspended))
        return;

    switch_from_to(d->state, Suspending, Suspended);
    d->sendCallOut(QFutureCallOutEvent(QFutureCallOutEvent::Suspended));
}

void QFutureInterfaceBase::setThrottled(bool enable)
{
    QMutexLocker lock(&d->m_mutex);
    if (enable) {
        switch_on(d->state, Throttled);
    } else {
        switch_off(d->state, Throttled);
        if (!(d->state.loadRelaxed() & suspendingOrSuspended))
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

bool QFutureInterfaceBase::isSuspending() const
{
    return queryState(Suspending);
}

#if QT_DEPRECATED_SINCE(6, 0)
bool QFutureInterfaceBase::isPaused() const
{
    return queryState(static_cast<State>(suspendingOrSuspended));
}
#endif

bool QFutureInterfaceBase::isSuspended() const
{
    return queryState(Suspended);
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

bool QFutureInterfaceBase::isValid() const
{
    const QMutexLocker lock(&d->m_mutex);
    return d->isValid;
}

bool QFutureInterfaceBase::isRunningOrPending() const
{
    return queryState(static_cast<State>(Running | Pending));
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
        if (!(state & suspendingOrSuspended) || (state & Canceled))
            return;
    }

    QMutexLocker lock(&d->m_mutex);
    const int state = d->state.loadRelaxed();
    if (!(state & suspendingOrSuspended) || (state & Canceled))
        return;

    // decrease active thread count since this thread will wait.
    const ThreadPoolThreadReleaser releaser(d->pool());

    d->pausedWaitCondition.wait(&d->m_mutex);
}

void QFutureInterfaceBase::suspendIfRequested()
{
    const auto canSuspend = [] (int state) {
        // can suspend only if 1) in any suspend-related state; 2) not canceled
        return (state & suspendingOrSuspended) && !(state & Canceled);
    };

    // return early if possible to avoid taking the mutex lock.
    {
        const int state = d->state.loadRelaxed();
        if (!canSuspend(state))
            return;
    }

    QMutexLocker lock(&d->m_mutex);
    const int state = d->state.loadRelaxed();
    if (!canSuspend(state))
        return;

    // Note: expecting that Suspending and Suspended are mutually exclusive
    if (!(state & Suspended)) {
        // switch state in case this is the first invocation
        switch_from_to(d->state, Suspending, Suspended);
        d->sendCallOut(QFutureCallOutEvent(QFutureCallOutEvent::Suspended));
    }

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
    return d->m_progress ? d->m_progress->minimum : 0;
}

int QFutureInterfaceBase::progressMaximum() const
{
    const QMutexLocker lock(&d->m_mutex);
    return d->m_progress ? d->m_progress->maximum : 0;
}

int QFutureInterfaceBase::resultCount() const
{
    QMutexLocker lock(&d->m_mutex);
    return d->internal_resultCount();
}

QString QFutureInterfaceBase::progressText() const
{
    QMutexLocker locker(&d->m_mutex);
    return d->m_progress ? d->m_progress->text : QString();
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
    d->isValid = true;
}

void QFutureInterfaceBase::reportCanceled()
{
    cancel();
}

#ifndef QT_NO_EXCEPTIONS
void QFutureInterfaceBase::reportException(const QException &exception)
{
    try {
        exception.raise();
    } catch (...) {
        reportException(std::current_exception());
    }
}

#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
void QFutureInterfaceBase::reportException(std::exception_ptr exception)
#else
void QFutureInterfaceBase::reportException(const std::exception_ptr &exception)
#endif
{
    QMutexLocker locker(&d->m_mutex);
    if (d->state.loadRelaxed() & (Canceled|Finished))
        return;

    d->hasException = true;
    d->data.setException(exception);
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
    if (d->m_progress)
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

int QFutureInterfaceBase::loadState() const
{
    // Used from ~QPromise, so this check is needed
    if (!d)
        return QFutureInterfaceBase::State::NoState;
    return d->state.loadRelaxed();
}

void QFutureInterfaceBase::waitForResult(int resultIndex)
{
    if (d->hasException)
        d->data.m_exceptionStore.rethrowException();

    QMutexLocker lock(&d->m_mutex);
    if (!isRunningOrPending())
        return;
    lock.unlock();

    // To avoid deadlocks and reduce the number of threads used, try to
    // run the runnable in the current thread.
    d->pool()->d_func()->stealAndRunRunnable(d->runnable);

    lock.relock();

    const int waitIndex = (resultIndex == -1) ? INT_MAX : resultIndex;
    while (isRunningOrPending() && !d->internal_isResultReadyAt(waitIndex))
        d->waitCondition.wait(&d->m_mutex);

    if (d->hasException)
        d->data.m_exceptionStore.rethrowException();
}

void QFutureInterfaceBase::waitForFinished()
{
    QMutexLocker lock(&d->m_mutex);
    const bool alreadyFinished = isFinished();
    lock.unlock();

    if (!alreadyFinished) {
        d->pool()->d_func()->stealAndRunRunnable(d->runnable);

        lock.relock();

        while (!isFinished())
            d->waitCondition.wait(&d->m_mutex);
    }

    if (d->hasException)
        d->data.m_exceptionStore.rethrowException();
}

void QFutureInterfaceBase::reportResultsReady(int beginIndex, int endIndex)
{
    if (beginIndex == endIndex || (d->state.loadRelaxed() & (Canceled|Finished)))
        return;

    d->waitCondition.wakeAll();

    if (!d->m_progress) {
        if (d->internal_updateProgressValue(d->m_progressValue + endIndex - beginIndex) == false) {
            d->sendCallOut(QFutureCallOutEvent(QFutureCallOutEvent::ResultsReady,
                                               beginIndex,
                                               endIndex));
            return;
        }

        d->sendCallOuts(QFutureCallOutEvent(QFutureCallOutEvent::Progress,
                                            d->m_progressValue,
                                            QString()),
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

QThreadPool *QFutureInterfaceBase::threadPool() const
{
    return d->m_pool;
}

void QFutureInterfaceBase::setFilterMode(bool enable)
{
    QMutexLocker locker(&d->m_mutex);
    if (!hasException())
        resultStoreBase().setFilterMode(enable);
}

/*!
    \internal
    Sets the progress range's minimum and maximum values to \a minimum and
    \a maximum respectively.

    If \a maximum is smaller than \a minimum, \a minimum becomes the only
    legal value.

    The progress value is reset to be \a minimum.

    The progress range usage can be disabled by using setProgressRange(0, 0).
    In this case progress value is also reset to 0.

    The behavior of this method is mostly inspired by
    \l QProgressBar::setRange.
*/
void QFutureInterfaceBase::setProgressRange(int minimum, int maximum)
{
    QMutexLocker locker(&d->m_mutex);
    if (!d->m_progress)
        d->m_progress.reset(new QFutureInterfaceBasePrivate::ProgressData());
    d->m_progress->minimum = minimum;
    d->m_progress->maximum = qMax(minimum, maximum);
    d->sendCallOut(QFutureCallOutEvent(QFutureCallOutEvent::ProgressRange, minimum, maximum));
    d->m_progressValue = minimum;
}

void QFutureInterfaceBase::setProgressValue(int progressValue)
{
    setProgressValueAndText(progressValue, QString());
}

/*!
    \internal
    In case of the \a progressValue falling out of the progress range,
    this method has no effect.
    Such behavior is inspired by \l QProgressBar::setValue.
*/
void QFutureInterfaceBase::setProgressValueAndText(int progressValue,
                                                   const QString &progressText)
{
    QMutexLocker locker(&d->m_mutex);
    if (!d->m_progress)
        d->m_progress.reset(new QFutureInterfaceBasePrivate::ProgressData());

    const bool useProgressRange = (d->m_progress->maximum != 0) || (d->m_progress->minimum != 0);
    if (useProgressRange
        && ((progressValue < d->m_progress->minimum) || (progressValue > d->m_progress->maximum))) {
        return;
    }

    if (d->m_progressValue >= progressValue)
        return;

    if (d->state.loadRelaxed() & (Canceled|Finished))
        return;

    if (d->internal_updateProgress(progressValue, progressText)) {
        d->sendCallOut(QFutureCallOutEvent(QFutureCallOutEvent::Progress,
                                           d->m_progressValue,
                                           d->m_progress->text));
    }
}

QMutex &QFutureInterfaceBase::mutex() const
{
    return d->m_mutex;
}

bool QFutureInterfaceBase::hasException() const
{
    return d->hasException;
}

QtPrivate::ExceptionStore &QFutureInterfaceBase::exceptionStore()
{
    Q_ASSERT(d->hasException);
    return d->data.m_exceptionStore;
}

QtPrivate::ResultStoreBase &QFutureInterfaceBase::resultStoreBase()
{
    Q_ASSERT(!d->hasException);
    return d->data.m_results;
}

const QtPrivate::ResultStoreBase &QFutureInterfaceBase::resultStoreBase() const
{
    Q_ASSERT(!d->hasException);
    return d->data.m_results;
}

QFutureInterfaceBase &QFutureInterfaceBase::operator=(const QFutureInterfaceBase &other)
{
    QFutureInterfaceBase copy(other);
    swap(copy);
    return *this;
}

// ### Qt 7: inline
void QFutureInterfaceBase::swap(QFutureInterfaceBase &other) noexcept
{
    qSwap(d, other.d);
}

bool QFutureInterfaceBase::refT() const noexcept
{
    return d->refCount.refT();
}

bool QFutureInterfaceBase::derefT() const noexcept
{
    // Called from ~QFutureInterface
    return !d || d->refCount.derefT();
}

void QFutureInterfaceBase::reset()
{
    d->m_progressValue = 0;
    d->m_progress.reset();
    d->progressTime.invalidate();
    d->isValid = false;
}

void QFutureInterfaceBase::rethrowPossibleException()
{
    if (hasException())
        exceptionStore().rethrowException();
}

QFutureInterfaceBasePrivate::QFutureInterfaceBasePrivate(QFutureInterfaceBase::State initialState)
    : state(initialState)
{
    progressTime.invalidate();
}

QFutureInterfaceBasePrivate::~QFutureInterfaceBasePrivate()
{
    if (hasException)
        data.m_exceptionStore.~ExceptionStore();
    else
        data.m_results.~ResultStoreBase();
}

int QFutureInterfaceBasePrivate::internal_resultCount() const
{
    return hasException ? 0 : data.m_results.count(); // ### subtract canceled results.
}

bool QFutureInterfaceBasePrivate::internal_isResultReadyAt(int index) const
{
    return hasException ? false : (data.m_results.contains(index));
}

bool QFutureInterfaceBasePrivate::internal_waitForNextResult()
{
    if (hasException)
        return false;

    if (data.m_results.hasNextResult())
        return true;

    while ((state.loadRelaxed() & QFutureInterfaceBase::Running)
           && data.m_results.hasNextResult() == false)
        waitCondition.wait(&m_mutex);

    return !(state.loadRelaxed() & QFutureInterfaceBase::Canceled)
            && data.m_results.hasNextResult();
}

bool QFutureInterfaceBasePrivate::internal_updateProgressValue(int progress)
{
    if (m_progressValue >= progress)
        return false;

    m_progressValue = progress;

    if (progressTime.isValid() && m_progressValue != 0) // make sure the first and last steps are emitted.
        if (progressTime.elapsed() < (1000 / MaxProgressEmitsPerSecond))
            return false;

    progressTime.start();
    return true;

}

bool QFutureInterfaceBasePrivate::internal_updateProgress(int progress,
                                                          const QString &progressText)
{
    if (m_progressValue >= progress)
        return false;

    Q_ASSERT(m_progress);

    m_progressValue = progress;
    m_progress->text = progressText;

    if (progressTime.isValid() && m_progressValue != m_progress->maximum) // make sure the first and last steps are emitted.
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
        if (!(state.loadRelaxed() & suspendingOrSuspended))
            pausedWaitCondition.wakeAll();
    }
}

void QFutureInterfaceBasePrivate::sendCallOut(const QFutureCallOutEvent &callOutEvent)
{
    if (outputConnections.isEmpty())
        return;

    for (int i = 0; i < outputConnections.size(); ++i)
        outputConnections.at(i)->postCallOutEvent(callOutEvent);
}

void QFutureInterfaceBasePrivate::sendCallOuts(const QFutureCallOutEvent &callOutEvent1,
                                     const QFutureCallOutEvent &callOutEvent2)
{
    if (outputConnections.isEmpty())
        return;

    for (int i = 0; i < outputConnections.size(); ++i) {
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

    QVarLengthArray<std::unique_ptr<QFutureCallOutEvent>, 3> events;

    const auto currentState = state.loadRelaxed();
    if (currentState & QFutureInterfaceBase::Started) {
        events.emplace_back(new QFutureCallOutEvent(QFutureCallOutEvent::Started));
        if (m_progress) {
            events.emplace_back(new QFutureCallOutEvent(QFutureCallOutEvent::ProgressRange,
                                                        m_progress->minimum,
                                                        m_progress->maximum));
            events.emplace_back(new QFutureCallOutEvent(QFutureCallOutEvent::Progress,
                                                        m_progressValue,
                                                        m_progress->text));
        } else {
            events.emplace_back(new QFutureCallOutEvent(QFutureCallOutEvent::ProgressRange,
                                                        0,
                                                        0));
            events.emplace_back(new QFutureCallOutEvent(QFutureCallOutEvent::Progress,
                                                        m_progressValue,
                                                        QString()));
        }
    }

    if (!hasException) {
        QtPrivate::ResultIteratorBase it = data.m_results.begin();
        while (it != data.m_results.end()) {
            const int begin = it.resultIndex();
            const int end = begin + it.batchSize();
            events.emplace_back(new QFutureCallOutEvent(QFutureCallOutEvent::ResultsReady,
                                                        begin,
                                                        end));
            it.batchedAdvance();
        }
    }

    if (currentState & QFutureInterfaceBase::Suspended)
        events.emplace_back(new QFutureCallOutEvent(QFutureCallOutEvent::Suspended));
    else if (currentState & QFutureInterfaceBase::Suspending)
        events.emplace_back(new QFutureCallOutEvent(QFutureCallOutEvent::Suspending));

    if (currentState & QFutureInterfaceBase::Canceled)
        events.emplace_back(new QFutureCallOutEvent(QFutureCallOutEvent::Canceled));

    if (currentState & QFutureInterfaceBase::Finished)
        events.emplace_back(new QFutureCallOutEvent(QFutureCallOutEvent::Finished));

    outputConnections.append(interface);

    locker.unlock();
    for (auto &&event : events)
        interface->postCallOutEvent(*event);
}

void QFutureInterfaceBasePrivate::disconnectOutputInterface(QFutureCallOutInterface *interface)
{
    QMutexLocker lock(&m_mutex);
    const qsizetype index = outputConnections.indexOf(interface);
    if (index == -1)
        return;
    outputConnections.removeAt(index);

    interface->callOutInterfaceDisconnected();
}

void QFutureInterfaceBasePrivate::setState(QFutureInterfaceBase::State newState)
{
    state.storeRelaxed(newState);
}

void QFutureInterfaceBase::setContinuation(std::function<void(const QFutureInterfaceBase &)> func)
{
    setContinuation(std::move(func), nullptr);
}

void QFutureInterfaceBase::setContinuation(std::function<void(const QFutureInterfaceBase &)> func,
                                           QFutureInterfaceBasePrivate *continuationFutureData)
{
    QMutexLocker lock(&d->continuationMutex);

    // If the state is ready, run continuation immediately,
    // otherwise save it for later.
    if (isFinished()) {
        lock.unlock();
        func(*this);
        lock.relock();
    }
    // Unless the continuation has been cleaned earlier, we have to
    // store the move-only continuation, to guarantee that the associated
    // future's data stays alive.
    if (d->continuationState != QFutureInterfaceBasePrivate::Cleaned) {
        if (d->continuation) {
            qWarning() << "Adding a continuation to a future which already has a continuation. "
                          "The existing continuation is overwritten.";
        }
        d->continuation = std::move(func);
        d->continuationData = continuationFutureData;
    }
}

void QFutureInterfaceBase::cleanContinuation()
{
    if (!d)
        return;

    QMutexLocker lock(&d->continuationMutex);
    d->continuation = nullptr;
    d->continuationState = QFutureInterfaceBasePrivate::Cleaned;
    d->continuationData = nullptr;
}

void QFutureInterfaceBase::runContinuation() const
{
    QMutexLocker lock(&d->continuationMutex);
    if (d->continuation) {
        // Save the continuation in a local function, to avoid calling
        // a null std::function below, in case cleanContinuation() is
        // called from some other thread right after unlock() below.
        auto fn = std::move(d->continuation);
        lock.unlock();
        fn(*this);

        lock.relock();
        // Unless the continuation has been cleaned earlier, we have to
        // store the move-only continuation, to guarantee that the associated
        // future's data stays alive.
        if (d->continuationState != QFutureInterfaceBasePrivate::Cleaned)
            d->continuation = std::move(fn);
    }
}

bool QFutureInterfaceBase::isChainCanceled() const
{
    return isCanceled() || d->continuationState == QFutureInterfaceBasePrivate::Canceled;
}

void QFutureInterfaceBase::setLaunchAsync(bool value)
{
    d->launchAsync = value;
}

bool QFutureInterfaceBase::launchAsync() const
{
    return d->launchAsync;
}

namespace QtFuture {

QFuture<void> makeReadyVoidFuture()
{
    QFutureInterface<void> promise;
    promise.reportStarted();
    promise.reportFinished();

    return promise.future();
}

} // namespace QtFuture

QT_END_NAMESPACE

#include "qfutureinterface.moc"
