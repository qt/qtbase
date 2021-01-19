/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QCFSOCKETNOTIFIER_P_H
#define QCFSOCKETNOTIFIER_P_H

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

#include <QtCore/private/qglobal_p.h>
#include <QtCore/qabstracteventdispatcher.h>
#include <QtCore/qhash.h>

#include <CoreFoundation/CoreFoundation.h>

QT_BEGIN_NAMESPACE

struct MacSocketInfo {
    MacSocketInfo() : socket(0), runloop(0), readNotifier(0), writeNotifier(0),
        readEnabled(false), writeEnabled(false) {}
    CFSocketRef socket;
    CFRunLoopSourceRef runloop;
    QObject *readNotifier;
    QObject *writeNotifier;
    bool readEnabled;
    bool writeEnabled;
};
typedef QHash<int, MacSocketInfo *> MacSocketHash;

typedef void (*MaybeCancelWaitForMoreEventsFn)(QAbstractEventDispatcher *hostEventDispacher);

// The CoreFoundationSocketNotifier class implements socket notifiers support using
// CFSocket for event dispatchers running on top of the Core Foundation run loop system.
// (currently Mac and iOS)
//
// The principal functions are registerSocketNotifier() and unregisterSocketNotifier().
//
// setHostEventDispatcher() should be called at startup.
// removeSocketNotifiers() should be called at shutdown.
//
class Q_CORE_EXPORT QCFSocketNotifier
{
public:
    QCFSocketNotifier();
    ~QCFSocketNotifier();
    void setHostEventDispatcher(QAbstractEventDispatcher *hostEventDispacher);
    void setMaybeCancelWaitForMoreEventsCallback(MaybeCancelWaitForMoreEventsFn callBack);
    void registerSocketNotifier(QSocketNotifier *notifier);
    void unregisterSocketNotifier(QSocketNotifier *notifier);
    void removeSocketNotifiers();

private:
    void destroyRunLoopObserver();

    static void unregisterSocketInfo(MacSocketInfo *socketInfo);
    static void enableSocketNotifiers(CFRunLoopObserverRef ref, CFRunLoopActivity activity, void *info);

    MacSocketHash macSockets;
    QAbstractEventDispatcher *eventDispatcher;
    MaybeCancelWaitForMoreEventsFn maybeCancelWaitForMoreEvents;
    CFRunLoopObserverRef enableNotifiersObserver;

    friend void qt_mac_socket_callback(CFSocketRef, CFSocketCallBackType, CFDataRef, const void *, void *);
};

QT_END_NAMESPACE

#endif
