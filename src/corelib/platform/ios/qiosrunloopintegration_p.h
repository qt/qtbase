// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QIOSRUNLOOPINTEGRATION_P_H
#define QIOSRUNLOOPINTEGRATION_P_H

#import <UIKit/UIApplication.h>

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. This header file may change
// from version to version without notice, or even be removed.
//
// We mean it.
//

@interface UIApplication (QtRunLoopIntegration)
+ (bool)qt_rootLevelRunLoopIntegration;
+ (bool)qt_eventDispatcherEnteredProcessEvents;
+ (void)qt_eventDispatcherInterruptEventLoopExec;
+ (bool)qt_applicationAboutToTerminate;
+ (char)qt_applicationWillTerminateExitCode;
@end

#endif // QIOSRUNLOOPINTEGRATION_P_H
