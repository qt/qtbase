/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtCore module of the Qt Toolkit.
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qwineventnotifier.h"

#include "qeventdispatcher_win_p.h"
#include "qcoreapplication.h"

#include <private/qthread_p.h>

QT_BEGIN_NAMESPACE

/*!
    \class QWinEventNotifier
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
  : QObject(parent), handleToEvent(0), enabled(false)
{}

/*!
    Constructs an event notifier with the given \a parent. It enables
    the \a notifier, and watches for the event \a hEvent.

    The notifier is enabled by default, i.e. it emits the activated() signal
    whenever the corresponding event is signalled. However, it is generally
    advisable to explicitly enable or disable the event notifier.

    \sa setEnabled(), isEnabled()
*/

QWinEventNotifier::QWinEventNotifier(HANDLE hEvent, QObject *parent)
 : QObject(parent), handleToEvent(hEvent), enabled(false)
{
    Q_D(QObject);
    QEventDispatcherWin32 *eventDispatcher = qobject_cast<QEventDispatcherWin32 *>(d->threadData->eventDispatcher);
    Q_ASSERT_X(eventDispatcher, "QWinEventNotifier::QWinEventNotifier()",
               "Cannot create a win event notifier without a QEventDispatcherWin32");
    eventDispatcher->registerEventNotifier(this);
    enabled = true;
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
    setEnabled(false);
    handleToEvent = hEvent;
}

/*!
    Returns the HANDLE that has been registered in the notifier.

    \sa setHandle()
*/

HANDLE  QWinEventNotifier::handle() const
{
    return handleToEvent;
}

/*!
    Returns true if the notifier is enabled; otherwise returns false.

    \sa setEnabled()
*/

bool QWinEventNotifier::isEnabled() const
{
    return enabled;
}

/*!
    If \a enable is true, the notifier is enabled; otherwise the notifier
    is disabled.

    \sa isEnabled(), activated()
*/

void QWinEventNotifier::setEnabled(bool enable)
{
    if (enabled == enable)                        // no change
        return;
    enabled = enable;

    Q_D(QObject);
    QEventDispatcherWin32 *eventDispatcher = qobject_cast<QEventDispatcherWin32 *>(d->threadData->eventDispatcher);
    if (!eventDispatcher) // perhaps application is shutting down
        return;

    if (enabled)
        eventDispatcher->registerEventNotifier(this);
    else
        eventDispatcher->unregisterEventNotifier(this);
}

/*!
    \reimp
*/

bool QWinEventNotifier::event(QEvent * e)
{
    if (e->type() == QEvent::ThreadChange) {
        if (enabled) {
            QMetaObject::invokeMethod(this, "setEnabled", Qt::QueuedConnection,
                                      Q_ARG(bool, enabled));
            setEnabled(false);
        }
    }
    QObject::event(e);                        // will activate filters
    if (e->type() == QEvent::WinEventAct) {
        emit activated(handleToEvent);
        return true;
    }
    return false;
}

QT_END_NAMESPACE
