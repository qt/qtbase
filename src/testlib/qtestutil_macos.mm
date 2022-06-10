// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
    void disableWindowRestore()
    {
        [NSUserDefaults.standardUserDefaults registerDefaults:@{
            @"ApplePersistenceIgnoreState" : @YES
        }];
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
        \brief Disables App Nap by registering a background activity.

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
