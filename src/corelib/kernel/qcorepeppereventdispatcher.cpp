/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qcorepeppereventdispatcher_p.h"
#include "qcoreapplication_p.h"
#include "qdebug.h"

QT_BEGIN_NAMESPACE
    
Q_LOGGING_CATEGORY(QT_PLATFORM_PEPPER_COREEVENTDISPATHCER, "qt.platform.pepper.coreeventdispatcher");
extern void *qtPepperInstance;

QCorePepperEventDispatcher::QCorePepperEventDispatcher(QObject *parent)
    : QAbstractEventDispatcher(*new QCorePepperEventDispatcherPrivate, parent)
{
    qCDebug(QT_PLATFORM_PEPPER_COREEVENTDISPATHCER) << "create QCorePepperEventDispatcher";
    qDebug() << "QCorePepperEventDispatcher";

}

QCorePepperEventDispatcher::QCorePepperEventDispatcher(QCorePepperEventDispatcherPrivate &dd, QObject *parent)
    : QAbstractEventDispatcher(dd, parent)
{ }

QCorePepperEventDispatcher::~QCorePepperEventDispatcher()
{
    Q_D(QCorePepperEventDispatcher);
    qCDebug(QT_PLATFORM_PEPPER_COREEVENTDISPATHCER) << "destroy QCorePepperEventDispatcher";
    d->messageLoop.PostQuit(true /*destroy*/);
}

bool QCorePepperEventDispatcher::processEvents(QEventLoop::ProcessEventsFlags flags)
{
    Q_D(QCorePepperEventDispatcher);
    Q_UNUSED(flags);
    qCDebug(QT_PLATFORM_PEPPER_COREEVENTDISPATHCER) << "processEvents";

    // Schedule a call to d->processEvents; there may be pending events
    // on the event loop queue.
    d->scheduleProcessEvents();

    // Start the native message loop. The Run() call will block until
    // a Quit() is called.
    d->messageLoop.AttachToCurrentThread();
    d->messageLoop.Run();

    qCDebug(QT_PLATFORM_PEPPER_COREEVENTDISPATHCER) << "processEvents done";

    return false;
}

bool QCorePepperEventDispatcher::hasPendingEvents()
{
    return true;
}

void QCorePepperEventDispatcher::registerSocketNotifier(QSocketNotifier *notifier)
{
    Q_UNUSED(notifier);
    qCWarning(QT_PLATFORM_PEPPER_COREEVENTDISPATHCER) << "Unimplemented: registerSocketNotifier";
}

void QCorePepperEventDispatcher::unregisterSocketNotifier(QSocketNotifier *notifier)
{
    Q_UNUSED(notifier);
    qCWarning(QT_PLATFORM_PEPPER_COREEVENTDISPATHCER) << "Unimplemented: unregisterSocketNotifier";
}

void QCorePepperEventDispatcher::registerTimer(int timerId, int interval, Qt::TimerType timerType, QObject *object)
{
    Q_UNUSED(timerId);
    Q_UNUSED(interval);
    Q_UNUSED(timerType);
    Q_UNUSED(object);
    qCWarning(QT_PLATFORM_PEPPER_COREEVENTDISPATHCER) << "Unimplemented: registerTimer";
}

bool QCorePepperEventDispatcher::unregisterTimer(int timerId)
{
    Q_UNUSED(timerId);
    qCWarning(QT_PLATFORM_PEPPER_COREEVENTDISPATHCER) << "Unimplemented: unregisterTimer";
    return false;
}

bool QCorePepperEventDispatcher::unregisterTimers(QObject *object)
{
    Q_UNUSED(object);
    qCWarning(QT_PLATFORM_PEPPER_COREEVENTDISPATHCER) << "Unimplemented: unregisterTimers";
    return false;
}

QList<QCorePepperEventDispatcher::TimerInfo>
QCorePepperEventDispatcher::registeredTimers(QObject *object) const
{
    Q_UNUSED(object);
    qCWarning(QT_PLATFORM_PEPPER_COREEVENTDISPATHCER) << "Unimplemented: registeredTimers";
    return QList<QCorePepperEventDispatcher::TimerInfo>();
}

int QCorePepperEventDispatcher::remainingTime(int timerId)
{
    Q_UNUSED(timerId);    
    qCWarning(QT_PLATFORM_PEPPER_COREEVENTDISPATHCER) << "Unimplemented: remainingTime";
    return 0;
}

void QCorePepperEventDispatcher::wakeUp()
{
    Q_D(QCorePepperEventDispatcher);
    qCDebug(QT_PLATFORM_PEPPER_COREEVENTDISPATHCER) << "wakeUp";
    // Called on postEvent etc.
    d->scheduleProcessEvents();
}

void QCorePepperEventDispatcher::interrupt()
{
    Q_D(QCorePepperEventDispatcher);
    qCDebug(QT_PLATFORM_PEPPER_COREEVENTDISPATHCER) << "interrupt";
    d->messageLoop.PostQuit(false /*dont destroy*/);
}

void QCorePepperEventDispatcher::flush()
{ 
    Q_D(QCorePepperEventDispatcher);
    qCDebug(QT_PLATFORM_PEPPER_COREEVENTDISPATHCER) << "flush";
    d->scheduleProcessEvents();
}

void QCorePepperEventDispatcher::startingUp()
{ 
    qCDebug(QT_PLATFORM_PEPPER_COREEVENTDISPATHCER) << "startingUp";
    QAbstractEventDispatcher::startingUp();
}

void QCorePepperEventDispatcher::closingDown()
{
    qCDebug(QT_PLATFORM_PEPPER_COREEVENTDISPATHCER) << "closingDown";
    QAbstractEventDispatcher::closingDown();
}

bool QCorePepperEventDispatcher::event(QEvent *e)
{
    return QAbstractEventDispatcher::event(e);
}

void QCorePepperEventDispatcher::sendPostedEvents()
{
    Q_D(QCorePepperEventDispatcher);
    qCDebug(QT_PLATFORM_PEPPER_COREEVENTDISPATHCER) << "sendPostedEvents";

    QCoreApplicationPrivate::sendPostedEvents(0, 0, d->threadData);
}

QCorePepperEventDispatcherPrivate::QCorePepperEventDispatcherPrivate()
    :messageLoop(reinterpret_cast<pp::Instance *>(qtPepperInstance))
    ,callbackFactory(this)
{

}

QCorePepperEventDispatcherPrivate::~QCorePepperEventDispatcherPrivate()
{

}

void QCorePepperEventDispatcherPrivate::scheduleProcessEvents()
{
    qCDebug(QT_PLATFORM_PEPPER_COREEVENTDISPATHCER) << "scheduleProcessEvents";
    pp::CompletionCallback processEvents =
        callbackFactory.NewCallback(&QCorePepperEventDispatcherPrivate::processEvents);
    int32_t result = messageLoop.PostWork(processEvents);
    if (result != PP_OK)
        qCDebug(QT_PLATFORM_PEPPER_COREEVENTDISPATHCER) << "scheduleProcessEvents PostWork error" << result;
}

void QCorePepperEventDispatcherPrivate::processEvents(int32_t status)
{
    Q_UNUSED(status);
    qCDebug(QT_PLATFORM_PEPPER_COREEVENTDISPATHCER) << "processEvents";

    QCoreApplicationPrivate::sendPostedEvents(0, 0, threadData);
}


QT_END_NAMESPACE
