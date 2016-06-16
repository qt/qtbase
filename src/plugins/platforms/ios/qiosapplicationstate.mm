/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qiosapplicationstate.h"

#include <qpa/qwindowsysteminterface.h>
#include <QtCore/qcoreapplication.h>

#include <QtGui/private/qguiapplication_p.h>

#import <UIKit/UIKit.h>

static Qt::ApplicationState qtApplicationState(UIApplicationState uiApplicationState)
{
    switch (uiApplicationState) {
    case UIApplicationStateActive:
        // The application is visible in front, and receiving events
        return Qt::ApplicationActive;
    case UIApplicationStateInactive:
        // The app is running in the foreground but is not receiving events. This
        // typically happens while transitioning to/from active/background, like
        // upon app launch or when receiving incoming calls.
        return Qt::ApplicationInactive;
    case UIApplicationStateBackground:
        // Normally the app would enter this state briefly before it gets
        // suspeded (you have five seconds, according to Apple).
        // You can request more time and start a background task, which would
        // normally map closer to Qt::ApplicationHidden. But since we have no
        // API for doing that yet, we handle this state as "about to be suspended".
        // Note: A screen-shot for the SpringBoard will also be taken after this
        // call returns.
        return Qt::ApplicationSuspended;
    }
}

static void handleApplicationStateChanged(UIApplicationState uiApplicationState)
{
    Qt::ApplicationState state = qtApplicationState(uiApplicationState);
    QWindowSystemInterface::handleApplicationStateChanged(state);
    QWindowSystemInterface::flushWindowSystemEvents();
}

QT_BEGIN_NAMESPACE

QIOSApplicationState::QIOSApplicationState()
{
    NSNotificationCenter *notificationCenter = [NSNotificationCenter defaultCenter];

    m_observers.push_back([notificationCenter addObserverForName:UIApplicationDidBecomeActiveNotification
        object:nil queue:[NSOperationQueue mainQueue] usingBlock:^(NSNotification *) {
            handleApplicationStateChanged(UIApplicationStateActive);
        }
    ]);

    m_observers.push_back([notificationCenter addObserverForName:UIApplicationWillResignActiveNotification
        object:nil queue:[NSOperationQueue mainQueue] usingBlock:^(NSNotification *) {
            // Note: UIApplication is still UIApplicationStateActive at this point,
            // but since there is no separate notification for the inactive state,
            // we report UIApplicationStateInactive now.
            handleApplicationStateChanged(UIApplicationStateInactive);
        }
    ]);

    m_observers.push_back([notificationCenter addObserverForName:UIApplicationDidEnterBackgroundNotification
        object:nil queue:[NSOperationQueue mainQueue] usingBlock:^(NSNotification *) {
            handleApplicationStateChanged(UIApplicationStateBackground);
        }
    ]);

    // Initialize correct startup state, which may not be the Qt default (inactive)
    UIApplicationState startupState = [UIApplication sharedApplication].applicationState;
    QGuiApplicationPrivate::applicationState = qtApplicationState(startupState);
}

QIOSApplicationState::~QIOSApplicationState()
{
    NSNotificationCenter *notificationCenter = [NSNotificationCenter defaultCenter];
    foreach (const NSObject* observer, m_observers)
        [notificationCenter removeObserver:observer];
}

QT_END_NAMESPACE

