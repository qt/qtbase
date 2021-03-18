/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtTest module of the Qt Toolkit.
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

#include "qtestutil_macos_p.h"

#include "QtCore/private/qcore_mac_p.h"

#import <AppKit/AppKit.h>

QT_BEGIN_NAMESPACE

namespace QTestPrivate {

    /*! \internal

        Disables restoration of previously saved window state. This causes tests
        to start with a clean slate and prevents the "previous restore failed"
        dialog from showing if there was a test crash.
    */
    void disableWindowRestore() {
        [[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"ApplePersistenceIgnoreState"];
    }

    bool macCrashReporterWillShowDialog()
    {
        auto dialogType = QCFType<CFStringRef>(CFPreferencesCopyAppValue(
            CFSTR("DialogType"), CFSTR("com.apple.CrashReporter")));

        auto stringCompare = [](CFStringRef str1, CFStringRef str2) -> bool {
            return CFStringCompare(str1, str2, kCFCompareCaseInsensitive) == kCFCompareEqualTo;
        };

        if (!dialogType || stringCompare(dialogType, CFSTR("basic"))) {
            // The default (basic) dialog type only shows up if the
            // application is 'user visible', as indicated by the
            // activation policy.
            auto *runningApp = NSRunningApplication.currentApplication;
            return runningApp && runningApp.activationPolicy == NSApplicationActivationPolicyRegular;
        } else if (stringCompare(dialogType, CFSTR("developer"))
                || stringCompare(dialogType, CFSTR("crashreport"))) {
            // While in developer mode the dialog will show for all
            // crashed applications, including backgrounded ones.
            return true;
        } else {
            // Finally, 'server' or 'none' will result in no dialog
            return false;
        }
    }

    /*! \internal
        \class AppNapDisabler
        \brief Disables App Nap by registereing a bacground activity.

        App Nap remains disabled as long as the AppNapDisabler instance
        exists.
    */

    /*! \internal
        Creates an AppNapDisabler instance and starts a NSActivityBackground activity.
    */
    AppNapDisabler::AppNapDisabler()
    {
        m_activity = [[NSProcessInfo processInfo] beginActivityWithOptions:NSActivityBackground
                                                                    reason:@"Qt Auto Test"];
        [m_activity retain];
    }

    /*! \internal
        Destroys the AppNapDisabler instance and ends the NSActivityBackground activity.
    */
    AppNapDisabler::~AppNapDisabler()
    {
        [[NSProcessInfo processInfo] endActivity:m_activity];
        [m_activity release];
    }
}

QT_END_NAMESPACE
