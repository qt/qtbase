// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

// qfutureinterface.h included from qfuture.h
#include "qfuture.h"
#include "qfutureinterface_p.h"

#include <QtCore/qatomic.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qthread.h>
#include <QtCore/qvarlengtharray.h>
#include <private/qthreadpool_p.h>
#include <private/qobject_p.h>

#include <climits> // For INT_MAX

// GCC 12 gets confused about QFutureInterfaceBase::state, for some non-obvious
// reason
//  warning: ‘unsigned int __atomic_or_fetch_4(volatile void*, unsigned int, int)’ writing 4 bytes into a region of size 0 overflows the destination [-Wstringop-overflow=]
QT_WARNING_DISABLE_GCC("-Wstringop-overflow")

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(lcQFutureContinuations, "qt.core.qfuture.continuations")

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

namespace QtPrivate {

void qfutureWarnIfUnusedResults(qsizetype numResults)
{
    if (numResults > 1) {
        qCWarning(lcQFutureContinuations,
                  "Parent future has %" PRIdQSIZETYPE " result(s), but only the first result "
                  "will be handled in the continuation.",
                  numResults);
    }
}

} // namespace QtPrivate

class QObjectContinuationWrapper : public QObject
{
    Q_OBJECT
public:
    explicit QObjectContinuationWrapper(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

signals:
    void run();
};

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

void QFutureInterfaceBasePrivate::cancelImpl(QFutureInterfaceBase::CancelMode mode,
                                             CancelOptions options)
{
    QMutexLocker locker(&m_mutex);

    const auto oldState = state.loadRelaxed();

    switch (mode) {
    case QFutureInterfaceBase::CancelMode::CancelAndFinish:
        if ((oldState & QFutureInterfaceBase::Finished)
            && (oldState & QFutureInterfaceBase::Canceled)) {
            return;
        }
        switch_from_to(state, suspendingOrSuspended | QFutureInterfaceBase::Running,
                       QFutureInterfaceBase::Canceled | QFutureInterfaceBase::Finished);
        break;
    case QFutureInterfaceBase::CancelMode::CancelOnly:
        if (oldState & QFutureInterfaceBase::Canceled)
            return;
        switch_from_to(state, suspendingOrSuspended, QFutureInterfaceBase::Canceled);
        break;
    }

    if (options & CancelOption::CancelContinuations) {
        // Cancel the continuations chain
        QMutexLocker continuationLocker(&continuationMutex);
        QFutureInterfaceBasePrivate *next = continuationData;
        while (next) {
            QMutexLocker nextLocker(&next->continuationMutex);
            if (next->continuationType == QFutureInterfaceBase::ContinuationType::Then) {
                next->continuationState = QFutureInterfaceBasePrivate::Canceled;
                next = next->continuationData;
            } else {
                break;
            }
        }
    }

    waitCondition.wakeAll();
    pausedWaitCondition.wakeAll();

    if (!(oldState & QFutureInterfaceBase::Canceled))
        sendCallOut(QFutureCallOutEvent(QFutureCallOutEvent::Canceled));
    if (mode == QFutureInterfaceBase::CancelMode::CancelAndFinish
        && !(oldState & QFutureInterfaceBase::Finished)) {
        sendCallOut(QFutureCallOutEvent(QFutureCallOutEvent::Finished));
    }

    isValid = false;
}

void QFutureInterfaceBase::cancel()
{
    cancel(CancelMode::CancelOnly);
}

void QFutureInterfaceBase::cancelChain()
{
    cancelChain(CancelMode::CancelOnly);
}

void QFutureInterfaceBase::cancel(QFutureInterfaceBase::CancelMode mode)
{
    d->cancelImpl(mode, QFutureInterfaceBasePrivate::CancelOption::CancelContinuations);
}

void QFutureInterfaceBase::cancelChain(QFutureInterfaceBase::CancelMode mode)
{
    // go up through the list of continuations, cancelling each of them
    {
        QMutexLocker locker(&d->continuationMutex);
        QFutureInterfaceBasePrivate *prev = d->nonConcludedParent;
        while (prev) {
            // Do not cancel continuations, because we're going bottom-to-top
            prev->cancelImpl(mode, QFutureInterfaceBasePrivate::CancelOption::None);
            QMutexLocker prevLocker(&prev->continuationMutex);
            prev = prev->nonConcludedParent;
        }
    }
    // finally, cancel self and all next continuations
    d->cancelImpl(mode, QFutureInterfaceBasePrivate::CancelOption::CancelContinuations);
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
        QFutureCallOutInterface *iface = outputConnections.at(i);
        iface->postCallOutEvent(callOutEvent1);
        iface->postCallOutEvent(callOutEvent2);
    }
}

// This function connects an output interface (for example a QFutureWatcher)
// to this future. While holding the lock we check the state and ready results
// and add the appropriate callouts to the queue.
void QFutureInterfaceBasePrivate::connectOutputInterface(QFutureCallOutInterface *iface)
{
    QMutexLocker locker(&m_mutex);

    const auto currentState = state.loadRelaxed();
    if (currentState & QFutureInterfaceBase::Started) {
        iface->postCallOutEvent(QFutureCallOutEvent(QFutureCallOutEvent::Started));
        if (m_progress) {
            iface->postCallOutEvent(QFutureCallOutEvent(QFutureCallOutEvent::ProgressRange,
                                                        m_progress->minimum,
                                                        m_progress->maximum));
            iface->postCallOutEvent(QFutureCallOutEvent(QFutureCallOutEvent::Progress,
                                                        m_progressValue,
                                                        m_progress->text));
        } else {
            iface->postCallOutEvent(QFutureCallOutEvent(QFutureCallOutEvent::ProgressRange,
                                                        0,
                                                        0));
            iface->postCallOutEvent(QFutureCallOutEvent(QFutureCallOutEvent::Progress,
                                                        m_progressValue,
                                                        QString()));
        }
    }

    if (!hasException) {
        QtPrivate::ResultIteratorBase it = data.m_results.begin();
        while (it != data.m_results.end()) {
            const int begin = it.resultIndex();
            const int end = begin + it.batchSize();
            iface->postCallOutEvent(QFutureCallOutEvent(QFutureCallOutEvent::ResultsReady,
                                                        begin,
                                                        end));
            it.batchedAdvance();
        }
    }

    if (currentState & QFutureInterfaceBase::Suspended)
        iface->postCallOutEvent(QFutureCallOutEvent(QFutureCallOutEvent::Suspended));
    else if (currentState & QFutureInterfaceBase::Suspending)
        iface->postCallOutEvent(QFutureCallOutEvent(QFutureCallOutEvent::Suspending));

    if (currentState & QFutureInterfaceBase::Canceled)
        iface->postCallOutEvent(QFutureCallOutEvent(QFutureCallOutEvent::Canceled));

    if (currentState & QFutureInterfaceBase::Finished)
        iface->postCallOutEvent(QFutureCallOutEvent(QFutureCallOutEvent::Finished));

    outputConnections.append(iface);
}

void QFutureInterfaceBasePrivate::disconnectOutputInterface(QFutureCallOutInterface *iface)
{
    QMutexLocker lock(&m_mutex);
    const qsizetype index = outputConnections.indexOf(iface);
    if (index == -1)
        return;
    outputConnections.removeAt(index);

    iface->callOutInterfaceDisconnected();
}

void QFutureInterfaceBasePrivate::setState(QFutureInterfaceBase::State newState)
{
    state.storeRelaxed(newState);
}

void QFutureInterfaceBase::setContinuation(std::function<void (const QFutureInterfaceBase &)> func,
                                           void *continuationFutureData, ContinuationType type)
{
    auto *futureData = static_cast<QFutureInterfaceBasePrivate *>(continuationFutureData);

    QMutexLocker lock(&d->continuationMutex);

    // If the state is ready, run continuation immediately,
    // otherwise save it for later.
    if (isFinished()) {
        d->continuationExecuted = true;
        lock.unlock();
        func(*this);
        lock.relock();
    }
    // Unless the continuation has been cleaned earlier, we have to
    // store the move-only continuation, to guarantee that the associated
    // future's data stays alive.
    if (d->continuationState != QFutureInterfaceBasePrivate::Cleaned) {
        if (d->continuation) {
            qWarning("Adding a continuation to a future which already has a continuation. "
                     "The existing continuation is overwritten.");
            if (d->continuationData)
                d->continuationData->nonConcludedParent = nullptr;
        }
        d->continuation = std::move(func);
        if (futureData) {
            futureData->continuationType = type;
            futureData->nonConcludedParent = d;
        }
        d->continuationData = futureData;
        Q_ASSERT_X(!futureData || futureData->continuationType != ContinuationType::Unknown,
                   "setContinuation", "Make sure to provide a correct continuation type!");
    }
}

/*
    For continuations with context we expect all the needed data to be captured
    directly by the continuation data, because this simplifies the slot
    invocation. That's why func has no parameters.

    We pass continuation data as a QVariant, because we need to keep the
    QFutureInterface<T> for the entire lifetime of the continuation, but we
    cannot pass a template type T as a parameter.
*/
void QFutureInterfaceBase::setContinuation(const QObject *context, std::function<void()> func,
                                           const QVariant &continuationFuture,
                                           ContinuationType type)
{
    Q_ASSERT(context);

    using FuncType = void();
    using Prototype = typename QtPrivate::Callable<FuncType>::Function;
    auto slotObj = QtPrivate::makeCallableObject<Prototype>(std::move(func));

    auto slot = QtPrivate::SlotObjUniquePtr(slotObj);

    auto *watcher = new QObjectContinuationWrapper;
    watcher->moveToThread(context->thread());

   // We need to protect acccess to the watcher. The context object (and in turn, the watcher)
   // could be destroyed while the continuation that emits the signal is running. We have to
   // prevent that.
   // The mutex has to be recursive, because the continuation itself could delete the context
   // object (and thus the watcher), which will try to lock the mutex from the same thread twice.
    auto watcherMutex = std::make_shared<QRecursiveMutex>();
    const auto destroyWatcher = [watcherMutex, watcher]() mutable {
        QMutexLocker lock(watcherMutex.get());
        delete watcher;
    };

    // ### we're missing a convenient way to `QObject::connect()` to a `QSlotObjectBase`...
    QObject::connect(watcher, &QObjectContinuationWrapper::run,
                     // for the following, cf. QMetaObject::invokeMethodImpl():
                     // we know `slot` is a lambda returning `void`, so we can just
                     // `call()` with `obj` and `args[0]` set to `nullptr`:
                     context, [slot = std::move(slot)] {
                         void *args[] = { nullptr }; // for `void` return value
                         slot->call(nullptr, args);
                     });
    QObject::connect(watcher, &QObjectContinuationWrapper::run, watcher, destroyWatcher);

    // We need to connect to destroyWatcher here, instead of delete or deleteLater().
    // If the continuation is called from a separate thread, emit watcher->run() can't detect that
    // the watcher has been deleted in the separate thread, causing a race condition and potential
    // heap-use-after-free issue inside QObject::doActivate. destroyWatcher forces the deletion of
    // the watcher to occur after emit watcher->run() completes and prevents the race condition.
    QObject::connect(context, &QObject::destroyed, watcher, destroyWatcher);

    // Extract a QFutureInterfaceBasePrivate pointer from the QVariant. We rely
    // on the fact that QVariant contains QFutureInterface<T>.
    QFutureInterfaceBasePrivate *continuationFutureData = nullptr;
    if (continuationFuture.isValid()) {
        Q_ASSERT(QLatin1StringView(continuationFuture.typeName())
                         .startsWith(QLatin1StringView("QFutureInterface")));
        const auto continuationPtr =
                static_cast<const QFutureInterfaceBase *>(continuationFuture.constData());
        continuationFutureData = continuationPtr->d;
    }

    // Capture continuationFuture so that it lives as long as the continuation,
    // and the continuation data remains valid.
    setContinuation([watcherMutex = std::move(watcherMutex),
                     watcher = QPointer(watcher), continuationFuture]
                    (const QFutureInterfaceBase &parentData)
    {
        Q_UNUSED(parentData);
        Q_UNUSED(continuationFuture);
        QMutexLocker lock(watcherMutex.get());
        if (watcher)
            emit watcher->run();
    }, continuationFutureData, type);
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
    if (d->continuation && !d->continuationExecuted) {
        // If we run the next continuation, then this future is concluded, so
        // we wouldn't need to revisit it in the cancelChain()
        if (d->continuationData)
            d->continuationData->nonConcludedParent = nullptr;
        // Save the continuation in a local function, to avoid calling
        // a null std::function below, in case cleanContinuation() is
        // called from some other thread right after unlock() below.
        d->continuationExecuted = true;
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
