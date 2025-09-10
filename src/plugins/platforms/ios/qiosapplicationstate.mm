// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qiosapplicationstate.h"

#include "qiosglobal.h"
#include "qiosintegration.h"

#include <qpa/qwindowsysteminterface.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/private/qcore_mac_p.h>

#include <QtGui/private/qguiapplication_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

static void qRegisterApplicationStateNotifications()
{
    NSNotificationCenter *notificationCenter = [NSNotificationCenter defaultCenter];
    NSOperationQueue *mainQueue = [NSOperationQueue mainQueue];

    // Map between notifications and corresponding application state. Note that
    // there's no separate notification for moving to UIApplicationStateInactive,
    // so we use UIApplicationWillResignActiveNotification as an intermediate.
    using NotificationMap = QMap<NSNotificationName, UIApplicationState>;
    static auto notifications = qt_apple_isApplicationExtension() ? NotificationMap{
        { NSExtensionHostWillEnterForegroundNotification, UIApplicationStateInactive },
        { NSExtensionHostDidBecomeActiveNotification, UIApplicationStateActive },
        { NSExtensionHostWillResignActiveNotification, UIApplicationStateInactive },
        { NSExtensionHostDidEnterBackgroundNotification, UIApplicationStateBackground },
    } : NotificationMap{
        { UIApplicationWillEnterForegroundNotification, UIApplicationStateInactive },
        { UIApplicationDidBecomeActiveNotification, UIApplicationStateActive },
        { UIApplicationWillResignActiveNotification, UIApplicationStateInactive },
        { UIApplicationDidEnterBackgroundNotification, UIApplicationStateBackground },
    };

    for (auto i = notifications.constBegin(); i != notifications.constEnd(); ++i) {
        [notificationCenter addObserverForName:i.key() object:nil queue:mainQueue
            usingBlock:^void(NSNotification *notification) {
                NSRange nameRange = NSMakeRange(2, notification.name.length - 14);
                QString reason = QString::fromNSString([notification.name substringWithRange:nameRange]);
                QIOSApplicationState::handleApplicationStateChanged(i.value(), reason);
        }];
    }

    if (qt_apple_isApplicationExtension()) {
        // Extensions are not allowed to access UIApplication, so we assume the state is active
        QIOSApplicationState::handleApplicationStateChanged(UIApplicationStateActive,
            "Extension loaded, assuming state is active"_L1);
    } else {
        // Initialize correct startup state, which may not be the Qt default (inactive)
        UIApplicationState startupState = qt_apple_sharedApplication().applicationState;
        QIOSApplicationState::handleApplicationStateChanged(startupState, "Application loaded"_L1);
    }
}
Q_CONSTRUCTOR_FUNCTION(qRegisterApplicationStateNotifications)

QIOSApplicationState::QIOSApplicationState()
{
    if (!qt_apple_isApplicationExtension()) {
        UIApplicationState startupState = qt_apple_sharedApplication().applicationState;
        QIOSApplicationState::handleApplicationStateChanged(startupState, "Application launched"_L1);
    }
}

void QIOSApplicationState::handleApplicationStateChanged(UIApplicationState uiState, const QString &reason)
{
    Qt::ApplicationState oldState = QGuiApplication::applicationState();
    Qt::ApplicationState newState = toQtApplicationState(uiState);
    qCDebug(lcQpaApplication) << qPrintable(reason) << "- moving from" << oldState << "to" << newState;

    if (QIOSIntegration *integration = QIOSIntegration::instance()) {
        emit integration->applicationState.applicationStateWillChange(oldState, newState);
        QWindowSystemInterface::handleApplicationStateChanged(newState);
        emit integration->applicationState.applicationStateDidChange(oldState, newState);
        qCDebug(lcQpaApplication) << "done moving to" << newState;
    } else {
        qCDebug(lcQpaApplication) << "no platform integration yet, setting state directly";
        QGuiApplicationPrivate::applicationState = newState;
    }
}

Qt::ApplicationState QIOSApplicationState::toQtApplicationState(UIApplicationState state)
{
    switch (state) {
    case UIApplicationStateActive: return Qt::ApplicationActive;
    case UIApplicationStateInactive: return Qt::ApplicationInactive;
    case UIApplicationStateBackground: return Qt::ApplicationSuspended;
    }
}

#include "moc_qiosapplicationstate.cpp"

QT_END_NAMESPACE
