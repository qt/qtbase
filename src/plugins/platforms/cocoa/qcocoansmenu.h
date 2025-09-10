// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCOCOANSMENU_H
#define QCOCOANSMENU_H

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

#include <QtCore/private/qcore_mac_p.h>

QT_FORWARD_DECLARE_CLASS(QCocoaMenu);
QT_FORWARD_DECLARE_CLASS(QCocoaMenuItem);

QT_DECLARE_NAMESPACED_OBJC_INTERFACE(QCocoaNSMenuDelegate, NSObject <NSMenuDelegate>
+ (instancetype)sharedMenuDelegate;
- (NSMenuItem *)findItemInMenu:(NSMenu *)menu forKey:(NSString *)key modifiers:(NSUInteger)modifiers;
)

QT_DECLARE_NAMESPACED_OBJC_INTERFACE(QCocoaNSMenu, NSMenu
@property (readonly, nonatomic) QCocoaMenu *platformMenu;
- (instancetype)initWithPlatformMenu:(QCocoaMenu *)menu;
- (instancetype)initWithoutPlatformMenu:(NSString *)menu;
)

QT_DECLARE_NAMESPACED_OBJC_INTERFACE(QCocoaNSMenuItem, NSMenuItem
@property (nonatomic) QCocoaMenuItem *platformMenuItem;
+ (instancetype)separatorItemWithPlatformMenuItem:(QCocoaMenuItem *)menuItem;
- (instancetype)initWithPlatformMenuItem:(QCocoaMenuItem *)menuItem;
- (instancetype)init;
)

#endif // QCOCOANSMENU_H
