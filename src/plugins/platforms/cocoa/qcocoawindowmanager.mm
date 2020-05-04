/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "qcocoawindowmanager.h"
#include "qcocoawindow.h"

#include <QtCore/private/qcore_mac_p.h>

#include <QtGui/qguiapplication.h>
#include <QtGui/qwindow.h>

QT_BEGIN_NAMESPACE

QCocoaWindowManager *QCocoaWindowManager::instance()
{
    static auto *instance = new QCocoaWindowManager;
    return instance;
}

QCocoaWindowManager::QCocoaWindowManager()
{
    if (NSApp) {
        initialize();
    } else {
        static auto applicationDidFinishLaunching(QMacNotificationObserver(nil,
            NSApplicationDidFinishLaunchingNotification, [this] { initialize(); }));
    }
}

void QCocoaWindowManager::initialize()
{
    Q_ASSERT(NSApp);

    // Whenever the modalWindow property of NSApplication changes we have a new
    // modal session running. Observing the app modal window instead of our own
    // event dispatcher sessions allows us to track session started by native
    // APIs as well. We need to check the initial state as well, in case there
    // is already a modal session running.
    static auto modalSessionObserver(QMacKeyValueObserver(
        NSApp, @"modalWindow", [this] { modalSessionChanged(); },
        NSKeyValueObservingOptionInitial | NSKeyValueObservingOptionNew));
}

void QCocoaWindowManager::modalSessionChanged()
{
    // Make sure that no window is overlapping the modal window. This can
    // happen for e.g. splash screens, which have the NSPopUpMenuWindowLevel.
    for (auto *window : QGuiApplication::topLevelWindows()) {
        auto *platformWindow = static_cast<QCocoaWindow*>(window->handle());
        if (!platformWindow)
            continue;

        auto naturalWindowLevel = platformWindow->windowLevel(window->flags());
        if (naturalWindowLevel > NSModalPanelWindowLevel) {
            NSWindow *nativeWindow = platformWindow->nativeWindow();
            if (NSApp.modalWindow) {
                // Lower window to that of the modal windows, but no less
                nativeWindow.level = NSModalPanelWindowLevel;
                if ([nativeWindow isVisible])
                    [nativeWindow orderBack:nil];
            } else {
                // Restore window's natural window level, whatever that was
                nativeWindow.level = naturalWindowLevel;
            }
        }
    }

    // Our worksWhenModal implementation is declarative and will normally be picked
    // up by AppKit when needed, but to make sure AppKit also reflects the state
    // in the window tag, so that the window can be ordered front by clicking it,
    // we need to explicitly call setWorksWhenModal.
    for (id window in NSApp.windows) {
        if ([window isKindOfClass:[QNSPanel class]]) {
            auto *panel = static_cast<QNSPanel *>(window);
            // Call setter to tell AppKit that our state has changed
            [panel setWorksWhenModal:panel.worksWhenModal];
        }
    }
}

static void initializeWindowManager() { Q_UNUSED(QCocoaWindowManager::instance()); }
Q_CONSTRUCTOR_FUNCTION(initializeWindowManager)

QT_END_NAMESPACE

