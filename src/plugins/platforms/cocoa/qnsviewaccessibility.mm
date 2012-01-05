/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <Carbon/Carbon.h>

#include "qnsview.h"
#include "qcocoahelpers.h"
#include "qcocoaaccessibility.h"
#include "qcocoaaccessibilityelement.h"

#include "QAccessibleActionInterface"
#include <QtCore/QDebug>

#import <AppKit/NSAccessibility.h>

@implementation QNSView (QNSViewAccessibility)

- (BOOL)accessibilityIsIgnored {
    return NO;
}

- (id)accessibilityAttributeValue:(NSString *)attribute {
    if ([attribute isEqualToString:NSAccessibilityRoleAttribute]) {
        if (m_accessibleRoot)
            return macRole(m_accessibleRoot->role());
        return NSAccessibilityUnknownRole;
    } else if ([attribute isEqualToString:NSAccessibilityRoleDescriptionAttribute]) {
        return NSAccessibilityRoleDescriptionForUIElement(self);
    } else if ([attribute isEqualToString:NSAccessibilityChildrenAttribute]) {
        if (!m_accessibleRoot)
            return [super accessibilityAttributeValue:attribute];

        // Create QCocoaAccessibleElements for each child if the
        // root accessible interface.
        int numKids = m_accessibleRoot->childCount();
        NSMutableArray *kids = [NSMutableArray arrayWithCapacity:numKids];
        for (int i = 0; i < numKids; ++i) {
            [kids addObject:[QCocoaAccessibleElement elementWithIndex:i parent:self accessibleInterface:(void*)m_accessibleRoot->child(i)]];
        }

        return NSAccessibilityUnignoredChildren(kids);
    } else {
        return [super accessibilityAttributeValue:attribute];
    }
}

- (id)accessibilityHitTest:(NSPoint)point {
    if (!m_accessibleRoot)
        return [super accessibilityHitTest:point];
    NSPoint windowPoint = [[self window] convertScreenToBase:point];

    QAccessibleInterface *childInterface = m_accessibleRoot->childAt(point.x, qt_mac_flipYCoordinate(point.y));
    // No child found, meaning we hit the NSView
    if (!childInterface) {
        return [super accessibilityHitTest:point];
    }

    // Hit a child, forward to child accessible interface.
    int childIndex = m_accessibleRoot->indexOfChild(childInterface);
    QCocoaAccessibleElement *accessibleElement = [QCocoaAccessibleElement elementWithIndex:childIndex -1 parent:self accessibleInterface: childInterface];
    return [accessibleElement accessibilityHitTest:point];
}

@end
