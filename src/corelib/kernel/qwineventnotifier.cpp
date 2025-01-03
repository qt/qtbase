// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwineventnotifier_p.h"

#include "qcoreapplication.h"
#include "qthread.h"
#include <QPointer>

QT_BEGIN_NAMESPACE

/*!
    \class QWinEventNotifier
    \inmodule QtCore
    \since 5.0
    \brief The QWinEventNotifier class provides support for the Windows Wait functions.

    The QWinEventNotifier class makes it possible to use the wait
    functions on windows in a asynchronous manner. With this class,
    you can register a HANDLE to an event and get notification when
    that event becomes signalled. The state of the event is not modified
    in the process so if it is a manual reset event you will need to
    reset it after the notification.

    Once you have created a event object using Windows API such as
    CreateEvent() or OpenEvent(), you can create an event notifier to
    monitor the event handle. If the event notifier is enabled, it will
    emit the activated() signal whenever the corresponding event object
    is signalled.

    The setEnabled() function allows you to disable as well as enable the
    event notifier. It is generally advisable to explicitly enable or
    disable the event notifier. A disabled notifier does nothing when the
    event object is signalled (the same effect as not creating the
    event notifier).  Use the isEnabled() function to determine the
    notifier's current status.

    Finally, you can use the setHandle() function to register a new event
    object, and the handle() function to retrieve the event handle.

    \b{Further information:}
    Although the class is called QWinEventNotifier, it can be used for
    certain other objects which are so-called synchronization
    objects, such as Processes, Threads, Waitable timers.

    \warning This class is only available on Windows.
*/

/*!
    \fn void QWinEventNotifier::activated(HANDLE hEvent)

    This signal is emitted whenever the event notifier is enabled and
    the corresponding HANDLE is signalled.

    The state of the event is not modified in the process, so if it is a
    manual reset event, you will need to reset it after the notification.

    The object is passed in the \a hEvent parameter.

    \sa handle()
*/

/*!
    Constructs an event notifier with the given \a parent.
*/

QWinEventNotifier::QWinEventNotifier(QObject *parent)
  : QObject(*new QWinEventNotifierPrivate, parent)
{}

/*!
    Constructs an event notifier with the given \a parent. It enables
    the notifier, and watches for the event \a hEvent.

    The notifier is enabled by default, i.e. it emits the activated() signal
    whenever the corresponding event is signalled. However, it is generally
    advisable to explicitly enable or disable the event notifier.

    \sa setEnabled(), isEnabled()
*/

QWinEventNotifier::QWinEventNotifier(HANDLE hEvent, QObject *parent)
 : QObject(*new QWinEventNotifierPrivate(hEvent, false), parent)
{
    setEnabled(true);
}

/*!
    Destroys this notifier.
*/

QWinEventNotifier::~QWinEventNotifier()
{
    setEnabled(false);
}

/*!
    Register the HANDLE \a hEvent. The old HANDLE will be automatically
    unregistered.

    \b Note: The notifier will be disabled as a side effect and needs
    to be re-enabled.

    \sa handle(), setEnabled()
*/

void QWinEventNotifier::setHandle(HANDLE hEvent)
{
    Q_D(QWinEventNotifier);
    setEnabled(false);
    d->handleToEvent = hEvent;
}

/*!
    Returns the HANDLE that has been registered in the notifier.

    \sa setHandle()
*/

HANDLE  QWinEventNotifier::handle() const
{
    Q_D(const QWinEventNotifier);
    return d->handleToEvent;
}

/*!
    Returns \c true if the notifier is enabled; otherwise returns \c false.

    \sa setEnabled()
*/

bool QWinEventNotifier::isEnabled() const
{
    Q_D(const QWinEventNotifier);
    return d->enabled;
}

/*!
    If \a enable is true, the notifier is enabled; otherwise the notifier
    is disabled.

    \sa isEnabled(), activated()
*/

void QWinEventNotifier::setEnabled(bool enable)
{
    Q_D(QWinEventNotifier);
    if (d->enabled == enable)                        // no change
        return;
    d->enabled = enable;

    if (Q_UNLIKELY(thread() != QThread::currentThread())) {
        qWarning("QWinEventNotifier: Event notifiers cannot be enabled or disabled from another thread");
        return;
    }

    if (enable) {
        // It is possible that the notifier was disabled after an event was already
        // posted. In that case we set a state that indicates that such an obsolete
        // event shall be ignored.
        d->winEventActPosted.testAndSetRelaxed(QWinEventNotifierPrivate::Posted,
                                               QWinEventNotifierPrivate::IgnorePosted);
        // The notifier can't be registered, if 'enabled' flag was false.
        // The code in the else branch ensures that.
        Q_ASSERT(!d->registered);
        SetThreadpoolWait(d->waitObject, d->handleToEvent, NULL);
        d->registered = true;
    } else if (d->registered) {
        // Stop waiting for an event. However, there may be a callback queued
        // already after the call.
        SetThreadpoolWait(d->waitObject, NULL, NULL);
        // So, to avoid a race condition after a possible call to
        // setEnabled(true), wait for a possibly outstanding callback
        // to complete.
        WaitForThreadpoolWaitCallbacks(d->waitObject, TRUE);
        d->registered = false;
    }
}

/*!
    \reimp
*/

bool QWinEventNotifier::event(QEvent * e)
{
    Q_D(QWinEventNotifier);

    switch (e->type()) {
    case QEvent::ThreadChange:
        if (d->enabled) {
            QMetaObject::invokeMethod(this, "setEnabled", Qt::QueuedConnection,
                                      Q_ARG(bool, true));
            setEnabled(false);
        }
        break;
    case QEvent::WinEventAct:
        // Emit notification, but only if the event has not been invalidated
        // since by the notifier being disabled, even if it was re-enabled
        // again.
        if (d->winEventActPosted.fetchAndStoreRelaxed(QWinEventNotifierPrivate::NotPosted)
            == QWinEventNotifierPrivate::Posted && d->enabled) {
            // Clear the flag, as the wait object is implicitly unregistered
            // when the callback is queued.
            d->registered = false;

            QPointer<QWinEventNotifier> alive(this);
            emit activated(d->handleToEvent, QPrivateSignal());

            if (alive && d->enabled && !d->registered) {
                SetThreadpoolWait(d->waitObject, d->handleToEvent, NULL);
                d->registered = true;
            }
        }
        return true;
    default:
        break;
    }
    return QObject::event(e);
}

QWinEventNotifierPrivate::QWinEventNotifierPrivate(HANDLE h, bool e)
    : handleToEvent(h), enabled(e), registered(false)
{
    waitObject = CreateThreadpoolWait(waitCallback, this, NULL);
    if (waitObject == NULL)
        qErrnoWarning("QWinEventNotifier:: CreateThreadpollWait failed.");
}

QWinEventNotifierPrivate::~QWinEventNotifierPrivate()
{
    CloseThreadpoolWait(waitObject);
}

void QWinEventNotifierPrivate::waitCallback(PTP_CALLBACK_INSTANCE instance, PVOID context,
                                            PTP_WAIT wait, TP_WAIT_RESULT waitResult)
{
    Q_UNUSED(instance);
    Q_UNUSED(wait);
    Q_UNUSED(waitResult);
    QWinEventNotifierPrivate *nd = reinterpret_cast<QWinEventNotifierPrivate *>(context);

    // Do not post an event, if an event is already in the message queue. Note
    // that an event that was previously invalidated will be reactivated.
    if (nd->winEventActPosted.fetchAndStoreRelaxed(QWinEventNotifierPrivate::Posted)
        == QWinEventNotifierPrivate::NotPosted) {
        QCoreApplication::postEvent(nd->q_func(), new QEvent(QEvent::WinEventAct));
    }
}

QT_END_NAMESPACE
