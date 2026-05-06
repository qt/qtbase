// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qioseventdispatcher.h"
#include "qiosapplicationdelegate.h"
#include "qiosglobal.h"

#if defined(Q_OS_VISIONOS)
#include "qiosswiftintegration.h"
#endif

#include <QtCore/qprocessordetection.h>
#include <QtCore/private/qcoreapplication_p.h>
#include <QtCore/private/qsystemerror_p.h>
#include <QtCore/private/qthread_p.h>
#include <QtCore/private/qiosrunloopintegration_p.h>

#include <qpa/qwindowsysteminterface.h>

#import <Foundation/NSArray.h>
#import <Foundation/NSString.h>
#import <Foundation/NSProcessInfo.h>
#import <Foundation/NSThread.h>
#import <Foundation/NSNotification.h>

#import <UIKit/UIApplication.h>

using namespace QT_PREPEND_NAMESPACE(QtPrivate);

QT_BEGIN_NAMESPACE
QT_USE_NAMESPACE

QIOSEventDispatcher *QIOSEventDispatcher::create()
{
    if (isQtApplication() && [UIApplication qt_rootLevelRunLoopIntegration])
        return new QIOSJumpingEventDispatcher;

    return new QIOSEventDispatcher;
}

QIOSEventDispatcher::QIOSEventDispatcher(QObject *parent)
    : QEventDispatcherCoreFoundation(parent)
{
    // We want all delivery of events from the system to be handled synchronously
    QWindowSystemInterface::setSynchronousWindowSystemEvents(true);
}

bool QIOSEventDispatcher::isQtApplication()
{
    // Determines whether the app was started via our qt_main_wrapper
    // in the entrypoint static library
    static const bool result = [UIApplication.class respondsToSelector:
        @selector(qt_rootLevelRunLoopIntegration)];
    return result;
}

/*!
    Override of the CoreFoundation posted events runloop source callback
    so that we can send window system (QPA) events in addition to sending
    normal Qt events.
*/
bool QIOSEventDispatcher::processPostedEvents()
{
    // Don't send window system events if the base CF dispatcher has determined
    // that events should not be sent for this pass of the runloop source.
    if (!QEventDispatcherCoreFoundation::processPostedEvents())
        return false;

    QT_APPLE_SCOPED_LOG_ACTIVITY(lcEventDispatcher().isDebugEnabled(), "sendWindowSystemEvents");
    QEventLoop::ProcessEventsFlags flags
        = QEventLoop::ProcessEventsFlags(m_processEvents.flags.loadRelaxed());
    qCDebug(lcEventDispatcher) << "Sending window system events for" << flags;
    QWindowSystemInterface::sendWindowSystemEvents(flags);

    return true;
}

QIOSJumpingEventDispatcher::QIOSJumpingEventDispatcher(QObject *parent)
    : QIOSEventDispatcher(parent)
    , m_processEventLevel(0)
    , m_runLoopExitObserver(this, &QIOSJumpingEventDispatcher::handleRunLoopExit, kCFRunLoopExit)
{
}

bool __attribute__((returns_twice)) QIOSJumpingEventDispatcher::processEvents(QEventLoop::ProcessEventsFlags flags)
{
    if ([UIApplication qt_applicationAboutToTerminate]) {
        qCDebug(lcEventDispatcher) << "Detected QEventLoop exec after application termination";
        // Re-issue exit, and return immediately
        qApp->exit([UIApplication qt_applicationWillTerminateExitCode]);
        return false;
    }

    const bool rootLevelProcessEvents = !m_processEventLevel;

    ++m_processEventLevel;
    bool processedEvents = false;

    if (rootLevelProcessEvents && (flags & QEventLoop::EventLoopExec)) {
        QT_APPLE_SCOPED_LOG_ACTIVITY(lcEventDispatcher().isDebugEnabled(), "processEvents");
        qCDebug(lcEventDispatcher) << "Processing events with flags" << flags;

        m_runLoopExitObserver.addToMode(kCFRunLoopCommonModes);
        processedEvents = [UIApplication qt_eventDispatcherEnteredProcessEvents];
        m_runLoopExitObserver.removeFromMode(kCFRunLoopCommonModes);
    } else {
        processedEvents = QEventDispatcherCoreFoundation::processEvents(flags);
    }

    --m_processEventLevel;
    return processedEvents;
}

void QIOSJumpingEventDispatcher::handleRunLoopExit(CFRunLoopActivity activity)
{
    Q_UNUSED(activity);
    Q_ASSERT(activity == kCFRunLoopExit);

    if (m_processEventLevel == 1 && !currentEventLoop()->isRunning())
        [UIApplication qt_eventDispatcherInterruptEventLoopExec];
}

QT_END_NAMESPACE
