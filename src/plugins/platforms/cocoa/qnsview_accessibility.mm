// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

// This file is included from qnsview.mm, and only used to organize the code

#include "qcocoaaccessibility.h"
#include "qcocoaaccessibilityelement.h"
#include "qcocoaintegration.h"

#include <QtGui/qaccessible.h>

#include <AppKit/NSAccessibility.h>

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
