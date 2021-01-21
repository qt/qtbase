/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

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

#import <AppKit/AppKit.h>

#include "qcocoahelpers.h"

QT_FORWARD_DECLARE_CLASS(QCocoaMenu);
QT_FORWARD_DECLARE_CLASS(QCocoaMenuItem);

@interface QT_MANGLE_NAMESPACE(QCocoaNSMenuDelegate) : NSObject <NSMenuDelegate>
+ (instancetype)sharedMenuDelegate;
- (NSMenuItem *)findItemInMenu:(NSMenu *)menu forKey:(NSString *)key modifiers:(NSUInteger)modifiers;
@end

@interface QT_MANGLE_NAMESPACE(QCocoaNSMenu) : NSMenu
@property (readonly, nonatomic) QCocoaMenu *platformMenu;
- (instancetype)initWithPlatformMenu:(QCocoaMenu *)menu;
@end

@interface QT_MANGLE_NAMESPACE(QCocoaNSMenuItem) : NSMenuItem
@property (nonatomic) QCocoaMenuItem *platformMenuItem;
+ (instancetype)separatorItemWithPlatformMenuItem:(QCocoaMenuItem *)menuItem;
- (instancetype)initWithPlatformMenuItem:(QCocoaMenuItem *)menuItem;
- (instancetype)init;
@end

QT_NAMESPACE_ALIAS_OBJC_CLASS(QCocoaNSMenu);
QT_NAMESPACE_ALIAS_OBJC_CLASS(QCocoaNSMenuItem);
QT_NAMESPACE_ALIAS_OBJC_CLASS(QCocoaNSMenuDelegate);

#endif // QCOCOANSMENU_H
