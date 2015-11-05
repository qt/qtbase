/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qcfsocketnotifier_p.h"
#include <QtCore/qcoreapplication.h>
#include <QtCore/qsocketnotifier.h>
#include <QtCore/qthread.h>

QT_BEGIN_NAMESPACE

/**************************************************************************
    Socket Notifiers
 *************************************************************************/
void qt_mac_socket_callback(CFSocketRef s, CFSocketCallBackType callbackType, CFDataRef,
                            const void *, void *info)
{

    QCFSocketNotifier *cfSocketNotifier = static_cast<QCFSocketNotifier *>(info);
    int nativeSocket = CFSocketGetNative(s);
    MacSocketInfo *socketInfo = cfSocketNotifier->macSockets.value(nativeSocket);
    QEvent notifierEvent(QEvent::SockAct);

    // There is a race condition that happen where we disable the notifier and
    // the kernel still has a notification to pass on. We then get this
    // notification after we've successfully disabled the CFSocket, but our Qt
    // notifier is now gone. The upshot is we have to check the notifier
    // every time.
    if (callbackType == kCFSocketReadCallBack) {
        if (socketInfo->readNotifier && socketInfo->readEnabled) {
            socketInfo->readEnabled = false;
            QCoreApplication::sendEvent(socketInfo->readNotifier, &notifierEvent);
        }
    } else if (callbackType == kCFSocketWriteCallBack) {
        if (socketInfo->writeNotifier && socketInfo->writeEnabled) {
            socketInfo->writeEnabled = false;
            QCoreApplication::sendEvent(socketInfo->writeNotifier, &notifierEvent);
        }
    }

    if (cfSocketNotifier->maybeCancelWaitForMoreEvents)
        cfSocketNotifier->maybeCancelWaitForMoreEvents(cfSocketNotifier->eventDispatcher);
}

/*
    Adds a loop source for the given socket to the current run loop.
*/
CFRunLoopSourceRef qt_mac_add_socket_to_runloop(const CFSocketRef socket)
{
    CFRunLoopSourceRef loopSource = CFSocketCreateRunLoopSource(kCFAllocatorDefault, socket, 0);
    if (!loopSource)
        return 0;

    CFRunLoopAddSource(CFRunLoopGetMain(), loopSource, kCFRunLoopCommonModes);
    return loopSource;
}

/*
    Removes the loop source for the given socket from the current run loop.
*/
void qt_mac_remove_socket_from_runloop(const CFSocketRef socket, CFRunLoopSourceRef runloop)
{
    Q_ASSERT(runloop);
    CFRunLoopRemoveSource(CFRunLoopGetMain(), runloop, kCFRunLoopCommonModes);
    CFSocketDisableCallBacks(socket, kCFSocketReadCallBack);
    CFSocketDisableCallBacks(socket, kCFSocketWriteCallBack);
}

QCFSocketNotifier::QCFSocketNotifier()
    : eventDispatcher(0)
    , maybeCancelWaitForMoreEvents(0)
    , enableNotifiersObserver(0)
{

}

QCFSocketNotifier::~QCFSocketNotifier()
{

}

void QCFSocketNotifier::setHostEventDispatcher(QAbstractEventDispatcher *hostEventDispacher)
{
    eventDispatcher = hostEventDispacher;
}

void QCFSocketNotifier::setMaybeCancelWaitForMoreEventsCallback(MaybeCancelWaitForMoreEventsFn callBack)
{
    maybeCancelWaitForMoreEvents = callBack;
}

void QCFSocketNotifier::registerSocketNotifier(QSocketNotifier *notifier)
{
    Q_ASSERT(notifier);
    int nativeSocket = notifier->socket();
    int type = notifier->type();
#ifndef QT_NO_DEBUG
    if (nativeSocket < 0 || nativeSocket > FD_SETSIZE) {
        qWarning("QSocketNotifier: Internal error");
        return;
    } else if (notifier->thread() != eventDispatcher->thread()
               || eventDispatcher->thread() != QThread::currentThread()) {
        qWarning("QSocketNotifier: socket notifiers cannot be enabled from another thread");
        return;
    }
#endif

    if (type == QSocketNotifier::Exception) {
        qWarning("QSocketNotifier::Exception is not supported on iOS");
        return;
    }

    // Check if we have a CFSocket for the native socket, create one if not.
    MacSocketInfo *socketInfo = macSockets.value(nativeSocket);
    if (!socketInfo) {
        socketInfo = new MacSocketInfo();

        // Create CFSocket, specify that we want both read and write callbacks (the callbacks
        // are enabled/disabled later on).
        const int callbackTypes = kCFSocketReadCallBack | kCFSocketWriteCallBack;
        CFSocketContext context = {0, this, 0, 0, 0};
        socketInfo->socket = CFSocketCreateWithNative(kCFAllocatorDefault, nativeSocket, callbackTypes, qt_mac_socket_callback, &context);
        if (CFSocketIsValid(socketInfo->socket) == false) {
            qWarning("QEventDispatcherMac::registerSocketNotifier: Failed to create CFSocket");
            return;
        }

        CFOptionFlags flags = CFSocketGetSocketFlags(socketInfo->socket);
        // QSocketNotifier doesn't close the socket upon destruction/invalidation
        flags &= ~kCFSocketCloseOnInvalidate;
        // Expicitly disable automatic re-enable, as we do that manually on each runloop pass
        flags &= ~(kCFSocketAutomaticallyReenableWriteCallBack | kCFSocketAutomaticallyReenableReadCallBack);
        CFSocketSetSocketFlags(socketInfo->socket, flags);

        macSockets.insert(nativeSocket, socketInfo);
    }

    if (type == QSocketNotifier::Read) {
        Q_ASSERT(socketInfo->readNotifier == 0);
        socketInfo->readNotifier = notifier;
        socketInfo->readEnabled = false;
    } else if (type == QSocketNotifier::Write) {
        Q_ASSERT(socketInfo->writeNotifier == 0);
        socketInfo->writeNotifier = notifier;
        socketInfo->writeEnabled = false;
    }

    if (!enableNotifiersObserver) {
        // Create a run loop observer which enables the socket notifiers on each
        // pass of the run loop, before any sources are processed.
        CFRunLoopObserverContext context = {};
        context.info = this;
        enableNotifiersObserver = CFRunLoopObserverCreate(kCFAllocatorDefault, kCFRunLoopBeforeSources,
                                                          true, 0, enableSocketNotifiers, &context);
        Q_ASSERT(enableNotifiersObserver);
        CFRunLoopAddObserver(CFRunLoopGetMain(), enableNotifiersObserver, kCFRunLoopCommonModes);
    }
}

void QCFSocketNotifier::unregisterSocketNotifier(QSocketNotifier *notifier)
{
    Q_ASSERT(notifier);
    int nativeSocket = notifier->socket();
    int type = notifier->type();
#ifndef QT_NO_DEBUG
    if (nativeSocket < 0 || nativeSocket > FD_SETSIZE) {
        qWarning("QSocketNotifier: Internal error");
        return;
    } else if (notifier->thread() != eventDispatcher->thread() || eventDispatcher->thread() != QThread::currentThread()) {
        qWarning("QSocketNotifier: socket notifiers cannot be disabled from another thread");
        return;
    }
#endif

    if (type == QSocketNotifier::Exception) {
        qWarning("QSocketNotifier::Exception is not supported on iOS");
        return;
    }
    MacSocketInfo *socketInfo = macSockets.value(nativeSocket);
    if (!socketInfo) {
        qWarning("QEventDispatcherMac::unregisterSocketNotifier: Tried to unregister a not registered notifier");
        return;
    }

    // Decrement read/write counters and disable callbacks if necessary.
    if (type == QSocketNotifier::Read) {
        Q_ASSERT(notifier == socketInfo->readNotifier);
        socketInfo->readNotifier = 0;
        socketInfo->readEnabled = false;
        CFSocketDisableCallBacks(socketInfo->socket, kCFSocketReadCallBack);
    } else if (type == QSocketNotifier::Write) {
        Q_ASSERT(notifier == socketInfo->writeNotifier);
        socketInfo->writeNotifier = 0;
        socketInfo->writeEnabled = false;
        CFSocketDisableCallBacks(socketInfo->socket, kCFSocketWriteCallBack);
    }

    // Remove CFSocket from runloop if this was the last QSocketNotifier.
    if (socketInfo->readNotifier == 0 && socketInfo->writeNotifier == 0) {
        unregisterSocketInfo(socketInfo);
        delete socketInfo;
        macSockets.remove(nativeSocket);
    }
}

void QCFSocketNotifier::removeSocketNotifiers()
{
    // Remove CFSockets from the runloop.
    foreach (MacSocketInfo *socketInfo, macSockets) {
        unregisterSocketInfo(socketInfo);
        delete socketInfo;
    }

    macSockets.clear();

    destroyRunLoopObserver();
}

void QCFSocketNotifier::destroyRunLoopObserver()
{
    if (!enableNotifiersObserver)
        return;

    CFRunLoopObserverInvalidate(enableNotifiersObserver);
    CFRelease(enableNotifiersObserver);
    enableNotifiersObserver = 0;
}

void QCFSocketNotifier::unregisterSocketInfo(MacSocketInfo *socketInfo)
{
    if (socketInfo->runloop) {
        if (CFSocketIsValid(socketInfo->socket))
            qt_mac_remove_socket_from_runloop(socketInfo->socket, socketInfo->runloop);
        CFRunLoopSourceInvalidate(socketInfo->runloop);
        CFRelease(socketInfo->runloop);
    }
    CFSocketInvalidate(socketInfo->socket);
    CFRelease(socketInfo->socket);
}

void QCFSocketNotifier::enableSocketNotifiers(CFRunLoopObserverRef ref, CFRunLoopActivity activity, void *info)
{
    Q_UNUSED(ref);
    Q_UNUSED(activity);

    QCFSocketNotifier *that = static_cast<QCFSocketNotifier *>(info);

    foreach (MacSocketInfo *socketInfo, that->macSockets) {
        if (!CFSocketIsValid(socketInfo->socket))
            continue;

        if (!socketInfo->runloop) {
            // Add CFSocket to runloop.
            if (!(socketInfo->runloop = qt_mac_add_socket_to_runloop(socketInfo->socket))) {
                qWarning("QEventDispatcherMac::registerSocketNotifier: Failed to add CFSocket to runloop");
                CFSocketInvalidate(socketInfo->socket);
                continue;
            }

            if (!socketInfo->readNotifier)
                CFSocketDisableCallBacks(socketInfo->socket, kCFSocketReadCallBack);
            if (!socketInfo->writeNotifier)
                CFSocketDisableCallBacks(socketInfo->socket, kCFSocketWriteCallBack);
        }

        if (socketInfo->readNotifier && !socketInfo->readEnabled) {
            socketInfo->readEnabled = true;
            CFSocketEnableCallBacks(socketInfo->socket, kCFSocketReadCallBack);
        }
        if (socketInfo->writeNotifier && !socketInfo->writeEnabled) {
            socketInfo->writeEnabled = true;
            CFSocketEnableCallBacks(socketInfo->socket, kCFSocketWriteCallBack);
        }
    }
}

QT_END_NAMESPACE

