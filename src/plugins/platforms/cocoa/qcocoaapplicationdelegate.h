// Copyright (C) 2018 The Qt Company Ltd.
// Copyright (c) 2007-2008, Apple, Inc.
// SPDX-License-Identifier: BSD-3-Clause

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of qapplication_*.cpp, qwidget*.cpp, qcolor_x11.cpp, qfiledialog.cpp
// and many other.  This header file may change from version to version
// without notice, or even be removed.
//
// We mean it.
//

#ifndef QCOCOAAPPLICATIONDELEGATE_H
#define QCOCOAAPPLICATIONDELEGATE_H

#include <qglobal.h>
#include <private/qcore_mac_p.h>

#include "qcocoansmenu.h"

#import <AppKit/NSApplication.h>

QT_DECLARE_NAMESPACED_OBJC_INTERFACE(QCocoaApplicationDelegate, NSObject <NSApplicationDelegate>
@property (nonatomic, retain) NSMenu *dockMenu;
+ (instancetype)sharedDelegate;
- (void)setReflectionDelegate:(NSObject<NSApplicationDelegate> *)oldDelegate;
- (void)removeAppleEventHandlers;
- (bool)inLaunch;
)

#if defined(__OBJC__)
@interface QCocoaApplicationDelegate (MenuAPI)
- (void)qt_itemFired:(QCocoaNSMenuItem *)item;
@end
#endif

#endif // QCOCOAAPPLICATIONDELEGATE_H
