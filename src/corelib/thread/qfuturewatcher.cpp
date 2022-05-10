// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qfuturewatcher.h"
#include "qfuturewatcher_p.h"

#include <QtCore/qcoreevent.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qthread.h>

QT_BEGIN_NAMESPACE

/*! \class QFutureWatcher
    \reentrant
    \since 4.4

    \inmodule QtCore
    \ingroup thread

    \brief The QFutureWatcher class allows monitoring a QFuture using signals
    and slots.

    QFutureWatcher provides information and notifications about a QFuture. Use
    the setFuture() function to start watching a particular QFuture. The
    future() function returns the future set with setFuture().

    For convenience, several of QFuture's functions are also available in
    QFutureWatcher: progressValue(), progressMinimum(), progressMaximum(),
    progressText(), isStarted(), isFinished(), isRunning(), isCanceled(),
    isSuspending(), isSuspended(), waitForFinished(), result(), and resultAt().
    The cancel(), setSuspended(), suspend(), resume(), and toggleSuspended() functions
    are slots in QFutureWatcher.

    Status changes are reported via the started(), finished(), canceled(),
    suspending(), suspended(), resumed(), resultReadyAt(), and resultsReadyAt()
    signals. Progress information is provided from the progressRangeChanged(),
    void progressValueChanged(), and progressTextChanged() signals.

    Throttling control is provided by the setPendingResultsLimit() function.
    When the number of pending resultReadyAt() or resultsReadyAt() signals
    exceeds the limit, the computation represented by the future will be
    throttled automatically. The computation will resume once the number of
    pending signals drops below the limit.

    Example: Starting a computation and getting a slot callback when it's
    finished:

    \snippet code/src_corelib_thread_qfuturewatcher.cpp 0

    Be aware that not all running asynchronous computations can be canceled or
    suspended. For example, the future returned by QtConcurrent::run() cannot be
    canceled; but the future returned by QtConcurrent::mappedReduced() can.

    QFutureWatcher<void> is specialized to not contain any of the result
    fetching functions. Any QFuture<T> can be watched by a
    QFutureWatcher<void> as well. This is useful if only status or progress
    information is needed; not the actual result data.

    \sa QFuture, {Qt Concurrent}
*/

/*! \fn template <typename T> QFutureWatcher<T>::QFutureWatcher(QObject *parent)

    Constructs a new QFutureWatcher with the given \a parent. Until a future is
    set with setFuture(), the functions isStarted(), isCanceled(), and
    isFinished() return \c true.
*/
QFutureWatcherBase::QFutureWatcherBase(QObject *parent)
    :QObject(*new QFutureWatcherBasePrivate, parent)
{ }

/*! \fn template <typename T> QFutureWatcher<T>::~QFutureWatcher()

    Destroys the QFutureWatcher.
*/

/*! \fn template <typename T> void QFutureWatcher<T>::cancel()

    Cancels the asynchronous computation represented by the future(). Note that
    the cancellation is asynchronous. Use waitForFinished() after calling
    cancel() when you need synchronous cancellation.

    Currently available results may still be accessed on a canceled QFuture,
    but new results will \e not become available after calling this function.
    Also, this QFutureWatcher will not deliver progress and result ready
    signals once canceled. This includes the progressValueChanged(),
    progressRangeChanged(), progressTextChanged(), resultReadyAt(), and
    resultsReadyAt() signals.

    Be aware that not all running asynchronous computations can be canceled.
    For example, the QFuture returned by QtConcurrent::run() cannot be
    canceled; but the QFuture returned by QtConcurrent::mappedReduced() can.
*/
void QFutureWatcherBase::cancel()
{
    futureInterface().cancel();
}

#if QT_DEPRECATED_SINCE(6, 0)
/*! \fn template <typename T> void QFutureWatcher<T>::setPaused(bool paused)

    \deprecated [6.6] Use setSuspended() instead.

    If \a paused is true, this function pauses the asynchronous computation
    represented by the future(). If the computation is already paused, this
    function does nothing. QFutureWatcher will not immediately stop delivering
    progress and result ready signals when the future is paused. At the moment
    of pausing there may still be computations that are in progress and cannot
    be stopped. Signals for such computations will still be delivered after
    pause.

    If \a paused is false, this function resumes the asynchronous computation.
    If the computation was not previously paused, this function does nothing.

    Be aware that not all computations can be paused. For example, the
    QFuture returned by QtConcurrent::run() cannot be paused; but the QFuture
    returned by QtConcurrent::mappedReduced() can.

    \sa suspend(), resume(), toggleSuspended()
*/
void QFutureWatcherBase::setPaused(bool paused)
{
    futureInterface().setSuspended(paused);
}

/*! \fn template <typename T> void QFutureWatcher<T>::pause()

    \deprecated
    Use suspend() instead.

    Pauses the asynchronous computation represented by the future(). This is a
    convenience method that simply calls setPaused(true).

    \sa resume()
*/
void QFutureWatcherBase::pause()
{
    futureInterface().setSuspended(true);
}

#endif // QT_DEPRECATED_SINCE(6, 0)

/*! \fn template <typename T> void QFutureWatcher<T>::setSuspended(bool suspend)

    \since 6.0

    If \a suspend is true, this function suspends the asynchronous computation
    represented by the future(). If the computation is already suspended, this
    function does nothing. QFutureWatcher will not immediately stop delivering
    progress and result ready signals when the future is suspended. At the moment
    of suspending there may still be computations that are in progress and cannot
    be stopped. Signals for such computations will still be delivered.

    If \a suspend is false, this function resumes the asynchronous computation.
    If the computation was not previously suspended, this function does nothing.

    Be aware that not all computations can be suspended. For example, the
    QFuture returned by QtConcurrent::run() cannot be suspended; but the QFuture
    returned by QtConcurrent::mappedReduced() can.

    \sa suspend(), resume(), toggleSuspended()
*/
void QFutureWatcherBase::setSuspended(bool suspend)
{
    futureInterface().setSuspended(suspend);
}

/*! \fn template <typename T> void QFutureWatcher<T>::suspend()

    \since 6.0

    Suspends the asynchronous computation represented by this future. This is a
    convenience method that simply calls setSuspended(true).

    \sa resume()
*/
void QFutureWatcherBase::suspend()
{
    futureInterface().setSuspended(true);
}

/*! \fn template <typename T> void QFutureWatcher<T>::resume()

    Resumes the asynchronous computation represented by the future(). This is
    a convenience method that simply calls setSuspended(false).

    \sa suspend()
*/

void QFutureWatcherBase::resume()
{
    futureInterface().setSuspended(false);
}

#if QT_DEPRECATED_SINCE(6, 0)
/*! \fn template <typename T> void QFutureWatcher<T>::togglePaused()

    \deprecated [6.0] Use toggleSuspended() instead.

    Toggles the paused state of the asynchronous computation. In other words,
    if the computation is currently paused, calling this function resumes it;
    if the computation is running, it is paused. This is a convenience method
    for calling setPaused(!isPaused()).

    \sa setSuspended(), suspend(), resume()
*/
void QFutureWatcherBase::togglePaused()
{
    futureInterface().toggleSuspended();
}
#endif // QT_DEPRECATED_SINCE(6, 0)

/*! \fn template <typename T> void QFutureWatcher<T>::toggleSuspended()

    \since 6.0

    Toggles the suspended state of the asynchronous computation. In other words,
    if the computation is currently suspending or suspended, calling this
    function resumes it; if the computation is running, it is suspended. This is a
    convenience method for calling setSuspended(!(isSuspending() || isSuspended())).

    \sa setSuspended(), suspend(), resume()
*/
void QFutureWatcherBase::toggleSuspended()
{
    futureInterface().toggleSuspended();
}

/*! \fn template <typename T> int QFutureWatcher<T>::progressValue() const

    Returns the current progress value, which is between the progressMinimum()
    and progressMaximum().

    \sa progressMinimum(), progressMaximum()
*/
int QFutureWatcherBase::progressValue() const
{
    return futureInterface().progressValue();
}

/*! \fn template <typename T> int QFutureWatcher<T>::progressMinimum() const

    Returns the minimum progressValue().

    \sa progressValue(), progressMaximum()
*/
int QFutureWatcherBase::progressMinimum() const
{
    return futureInterface().progressMinimum();
}

/*! \fn template <typename T> int QFutureWatcher<T>::progressMaximum() const

    Returns the maximum progressValue().

    \sa progressValue(), progressMinimum()
*/
int QFutureWatcherBase::progressMaximum() const
{
    return futureInterface().progressMaximum();
}

/*! \fn template <typename T> QString QFutureWatcher<T>::progressText() const

    Returns the (optional) textual representation of the progress as reported
    by the asynchronous computation.

    Be aware that not all computations provide a textual representation of the
    progress, and as such, this function may return an empty string.
*/
QString QFutureWatcherBase::progressText() const
{
    return futureInterface().progressText();
}

/*! \fn template <typename T> bool QFutureWatcher<T>::isStarted() const

    Returns \c true if the asynchronous computation represented by the future()
    has been started, or if no future has been set; otherwise returns \c false.
*/
bool QFutureWatcherBase::isStarted() const
{
    return futureInterface().queryState(QFutureInterfaceBase::Started);
}

/*! \fn template <typename T> bool QFutureWatcher<T>::isFinished() const

    Returns \c true if the asynchronous computation represented by the future()
    has finished, or if no future has been set; otherwise returns \c false.
*/
bool QFutureWatcherBase::isFinished() const
{
    return futureInterface().isFinished();
}

/*! \fn template <typename T> bool QFutureWatcher<T>::isRunning() const

    Returns \c true if the asynchronous computation represented by the future()
    is currently running; otherwise returns \c false.
*/
bool QFutureWatcherBase::isRunning() const
{
    return futureInterface().queryState(QFutureInterfaceBase::Running);
}

/*! \fn template <typename T> bool QFutureWatcher<T>::isCanceled() const

    Returns \c true if the asynchronous computation has been canceled with the
    cancel() function, or if no future has been set; otherwise returns \c false.

    Be aware that the computation may still be running even though this
    function returns \c true. See cancel() for more details.
*/
bool QFutureWatcherBase::isCanceled() const
{
    return futureInterface().queryState(QFutureInterfaceBase::Canceled);
}

#if QT_DEPRECATED_SINCE(6, 0)

/*! \fn template <typename T> bool QFutureWatcher<T>::isPaused() const

    \deprecated [6.0] Use isSuspending() or isSuspended() instead.

    Returns \c true if the asynchronous computation has been paused with the
    pause() function; otherwise returns \c false.

    Be aware that the computation may still be running even though this
    function returns \c true. See setPaused() for more details. To check
    if pause actually took effect, use isSuspended() instead.

    \sa setSuspended(), toggleSuspended(), isSuspended()
*/

bool QFutureWatcherBase::isPaused() const
{
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    return futureInterface().isPaused();
QT_WARNING_POP
}
#endif // QT_DEPRECATED_SINCE(6, 0)

/*! \fn template <typename T> bool QFutureWatcher<T>::isSuspending() const

    \since 6.0

    Returns \c true if the asynchronous computation has been suspended with the
    suspend() function, but the work is not yet suspended, and computation is still
    running. Returns \c false otherwise.

    To check if suspension is actually in effect, use isSuspended() instead.

    \sa setSuspended(), toggleSuspended(), isSuspended()
*/
bool QFutureWatcherBase::isSuspending() const
{
    return futureInterface().isSuspending();
}

/*! \fn template <typename T> bool QFutureWatcher<T>::isSuspended() const

    \since 6.0

    Returns \c true if a suspension of the asynchronous computation has been
    requested, and it is in effect, meaning that no more results or progress
    changes are expected.

    \sa suspended(), setSuspended(), isSuspending()
*/
bool QFutureWatcherBase::isSuspended() const
{
    return futureInterface().isSuspended();
}

/*! \fn template <typename T> void QFutureWatcher<T>::waitForFinished()

    Waits for the asynchronous computation to finish (including cancel()ed
    computations), i.e. until isFinished() returns \c true.
*/
void QFutureWatcherBase::waitForFinished()
{
    futureInterface().waitForFinished();
}

bool QFutureWatcherBase::event(QEvent *event)
{
    Q_D(QFutureWatcherBase);
    if (event->type() == QEvent::FutureCallOut) {
        QFutureCallOutEvent *callOutEvent = static_cast<QFutureCallOutEvent *>(event);
        d->sendCallOutEvent(callOutEvent);
        return true;
    }
    return QObject::event(event);
}

/*! \fn template <typename T> void QFutureWatcher<T>::setPendingResultsLimit(int limit)

    The setPendingResultsLimit() provides throttling control. When the number
    of pending resultReadyAt() or resultsReadyAt() signals exceeds the
    \a limit, the computation represented by the future will be throttled
    automatically. The computation will resume once the number of pending
    signals drops below the \a limit.
*/
void QFutureWatcherBase::setPendingResultsLimit(int limit)
{
    Q_D(QFutureWatcherBase);
    d->maximumPendingResultsReady = limit;
}

void QFutureWatcherBase::connectNotify(const QMetaMethod &signal)
{
    Q_D(QFutureWatcherBase);
    static const QMetaMethod resultReadyAtSignal = QMetaMethod::fromSignal(&QFutureWatcherBase::resultReadyAt);
    if (signal == resultReadyAtSignal)
        d->resultAtConnected.ref();
#ifndef QT_NO_DEBUG
    static const QMetaMethod finishedSignal = QMetaMethod::fromSignal(&QFutureWatcherBase::finished);
    if (signal == finishedSignal) {
        if (futureInterface().isRunning()) {
            //connections should be established before calling stFuture to avoid race.
            // (The future could finish before the connection is made.)
            qWarning("QFutureWatcher::connect: connecting after calling setFuture() is likely to produce race");
        }
    }
#endif
}

void QFutureWatcherBase::disconnectNotify(const QMetaMethod &signal)
{
    Q_D(QFutureWatcherBase);
    static const QMetaMethod resultReadyAtSignal = QMetaMethod::fromSignal(&QFutureWatcherBase::resultReadyAt);
    if (signal == resultReadyAtSignal)
        d->resultAtConnected.deref();
}

/*!
    \internal
*/
QFutureWatcherBasePrivate::QFutureWatcherBasePrivate()
    : maximumPendingResultsReady(QThread::idealThreadCount() * 2),
      resultAtConnected(0)
{ }

/*!
    \internal
*/
void QFutureWatcherBase::connectOutputInterface()
{
    futureInterface().d->connectOutputInterface(d_func());
}

/*!
    \internal
*/
void QFutureWatcherBase::disconnectOutputInterface(bool pendingAssignment)
{
    if (pendingAssignment) {
        Q_D(QFutureWatcherBase);
        d->pendingResultsReady.storeRelaxed(0);
    }

    futureInterface().d->disconnectOutputInterface(d_func());
}

void QFutureWatcherBasePrivate::postCallOutEvent(const QFutureCallOutEvent &callOutEvent)
{
    Q_Q(QFutureWatcherBase);

    if (callOutEvent.callOutType == QFutureCallOutEvent::ResultsReady) {
        if (pendingResultsReady.fetchAndAddRelaxed(1) >= maximumPendingResultsReady)
            q->futureInterface().d->internal_setThrottled(true);
    }

    QCoreApplication::postEvent(q, callOutEvent.clone());
}

void QFutureWatcherBasePrivate::callOutInterfaceDisconnected()
{
    QCoreApplication::removePostedEvents(q_func(), QEvent::FutureCallOut);
}

void QFutureWatcherBasePrivate::sendCallOutEvent(QFutureCallOutEvent *event)
{
    Q_Q(QFutureWatcherBase);

    switch (event->callOutType) {
        case QFutureCallOutEvent::Started:
            emit q->started();
        break;
        case QFutureCallOutEvent::Finished:
            emit q->finished();
        break;
        case QFutureCallOutEvent::Canceled:
            pendingResultsReady.storeRelaxed(0);
            emit q->canceled();
        break;
        case QFutureCallOutEvent::Suspending:
            if (q->futureInterface().isCanceled())
                break;
            emit q->suspending();
#if QT_DEPRECATED_SINCE(6, 0)
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
            emit q->paused();
QT_WARNING_POP
#endif
        break;
        case QFutureCallOutEvent::Suspended:
            if (q->futureInterface().isCanceled())
                break;
            emit q->suspended();
        break;
        case QFutureCallOutEvent::Resumed:
            if (q->futureInterface().isCanceled())
                break;
            emit q->resumed();
        break;
        case QFutureCallOutEvent::ResultsReady: {
            if (q->futureInterface().isCanceled())
                break;

            if (pendingResultsReady.fetchAndAddRelaxed(-1) <= maximumPendingResultsReady)
                q->futureInterface().setThrottled(false);

            const int beginIndex = event->index1;
            const int endIndex = event->index2;

            emit q->resultsReadyAt(beginIndex, endIndex);

            if (resultAtConnected.loadRelaxed() <= 0)
                break;

            for (int i = beginIndex; i < endIndex; ++i)
                emit q->resultReadyAt(i);

        } break;
        case QFutureCallOutEvent::Progress:
            if (q->futureInterface().isCanceled())
                break;

            emit q->progressValueChanged(event->index1);
            if (!event->text.isNull()) // ###
                emit q->progressTextChanged(event->text);
        break;
        case QFutureCallOutEvent::ProgressRange:
            emit q->progressRangeChanged(event->index1, event->index2);
        break;
        default: break;
    }
}


/*! \fn template <typename T> const T &QFutureWatcher<T>::result() const

    Returns the first result in the future(). If the result is not immediately
    available, this function will block and wait for the result to become
    available. This is a convenience method for calling resultAt(0).

    \sa resultAt()
*/

/*! \fn template <typename T> const T &QFutureWatcher<T>::resultAt(int index) const

    Returns the result at \a index in the future(). If the result is not
    immediately available, this function will block and wait for the result to
    become available.

    \sa result()
*/

/*! \fn template <typename T> void QFutureWatcher<T>::setFuture(const QFuture<T> &future)

    Starts watching the given \a future.

    If \a future has already started, the watcher will initially emit signals
    that bring their listeners up to date about the future's state. The
    following signals will, if applicable, be emitted in the given order:
    started(), progressRangeChanged(), progressValueChanged(),
    progressTextChanged(), resultsReadyAt(), resultReadyAt(), suspending(),
    suspended(), canceled(), and finished(). Of these, resultsReadyAt() and
    resultReadyAt() may be emitted several times to cover all available
    results. progressValueChanged() and progressTextChanged() will only be
    emitted once for the latest available progress value and text.

    To avoid a race condition, it is important to call this function
    \e after doing the connections.
*/

/*! \fn template <typename T> QFuture<T> QFutureWatcher<T>::future() const

    Returns the watched future.
*/

/*! \fn template <typename T> void QFutureWatcher<T>::started()

    This signal is emitted when this QFutureWatcher starts watching the future
    set with setFuture().
*/

/*!
    \fn template <typename T> void QFutureWatcher<T>::finished()
    This signal is emitted when the watched future finishes.
*/

/*!
    \fn template <typename T> void QFutureWatcher<T>::canceled()
    This signal is emitted if the watched future is canceled.
*/

/*! \fn template <typename T> void QFutureWatcher<T>::suspending()

    \since 6.0

    This signal is emitted when the state of the watched future is
    set to suspended.

    \note This signal only informs that suspension has been requested. It
    doesn't indicate that all background operations are stopped. Signals
    for computations that were in progress at the moment of suspending will
    still be delivered. To be informed when suspension actually
    took effect, use the suspended() signal.

    \sa setSuspended(), suspend(), suspended()
*/

#if QT_DEPRECATED_SINCE(6, 0)
/*! \fn template <typename T> void QFutureWatcher<T>::paused()

    \deprecated [6.0] Use suspending() instead.

    This signal is emitted when the state of the watched future is
    set to paused.

    \note This signal only informs that pause has been requested. It
    doesn't indicate that all background operations are stopped. Signals
    for computations that were in progress at the moment of pausing will
    still be delivered. To to be informed when pause() actually
    took effect, use the suspended() signal.

    \sa setSuspended(), suspend(), suspended()
*/
#endif // QT_DEPRECATED_SINCE(6, 0)

/*! \fn template <typename T> void QFutureWatcher<T>::suspended()

    \since 6.0

    This signal is emitted when suspend() took effect, meaning that there are
    no more running computations. After receiving this signal no more result
    ready or progress reporting signals are expected.

    \sa setSuspended(), suspend(), suspended()
*/

/*! \fn template <typename T> void QFutureWatcher<T>::resumed()
    This signal is emitted when the watched future is resumed.
*/

/*!
    \fn template <typename T> void QFutureWatcher<T>::progressRangeChanged(int minimum, int maximum)

    The progress range for the watched future has changed to \a minimum and
    \a maximum
*/

/*!
    \fn template <typename T> void QFutureWatcher<T>::progressValueChanged(int progressValue)

    This signal is emitted when the watched future reports progress,
    \a progressValue gives the current progress. In order to avoid overloading
    the GUI event loop, QFutureWatcher limits the progress signal emission
    rate. This means that listeners connected to this slot might not get all
    progress reports the future makes. The last progress update (where
    \a progressValue equals the maximum value) will always be delivered.
*/

/*! \fn template <typename T> void QFutureWatcher<T>::progressTextChanged(const QString &progressText)

    This signal is emitted when the watched future reports textual progress
    information, \a progressText.
*/

/*!
    \fn template <typename T> void QFutureWatcher<T>::resultReadyAt(int index)

    This signal is emitted when the watched future reports a ready result at
    \a index. If the future reports multiple results, the index will indicate
    which one it is. Results can be reported out-of-order. To get the result,
    call resultAt(index);
*/

/*!
    \fn template <typename T> void QFutureWatcher<T>::resultsReadyAt(int beginIndex, int endIndex);

    This signal is emitted when the watched future reports ready results.
    The results are indexed from \a beginIndex to \a endIndex.

*/

QT_END_NAMESPACE

#include "moc_qfuturewatcher.cpp"
