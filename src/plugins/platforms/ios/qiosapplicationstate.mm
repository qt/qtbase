/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
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

