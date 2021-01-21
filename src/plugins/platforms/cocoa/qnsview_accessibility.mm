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

// This file is included from qnsview.mm, and only used to organize the code

#include "qcocoaaccessibility.h"
#include "qcocoaaccessibilityelement.h"
#include "qcocoaintegration.h"

#include <QtGui/qaccessible.h>

#import <AppKit/NSAccessibility.h>

@implementation QNSView (Accessibility)

- (id)childAccessibleElement
{
    QCocoaWindow *platformWindow = self.platformWindow;
    if (!platformWindow || !platformWindow->window()->accessibleRoot())
        return nil;

    QAccessible::Id childId = QAccessible::uniqueId(platformWindow->window()->accessibleRoot());
    return [QMacAccessibilityElement elementWithId:childId];
}

// The QNSView is a container that the user does not interact directly with:
// Remove it from the user-visible accessibility tree.
- (BOOL)accessibilityIsIgnored
{
    return YES;
}

- (id)accessibilityAttributeValue:(NSString *)attribute
{
    // activate accessibility updates
    QCocoaIntegration::instance()->accessibility()->setActive(true);

    if ([attribute isEqualToString:NSAccessibilityChildrenAttribute])
        return NSAccessibilityUnignoredChildrenForOnlyChild([self childAccessibleElement]);
    else
        return [super accessibilityAttributeValue:attribute];
}

- (id)accessibilityHitTest:(NSPoint)point
{
    return [[self childAccessibleElement] accessibilityHitTest:point];
}

- (id)accessibilityFocusedUIElement
{
    return [[self childAccessibleElement] accessibilityFocusedUIElement];
}

@end
