/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
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
#include <qpa/qplatformintegration.h>

#include <QtGui/private/qaccessible2_p.h>
#include <QtCore/QDebug>

#import <AppKit/NSAccessibility.h>

@implementation QNSView (QNSViewAccessibility)

// The QNSView is a container that the user does not interact directly with:
// Remove it from the user-visible accessibility tree.
- (BOOL)accessibilityIsIgnored {
    return YES;
}

- (id)accessibilityAttributeValue:(NSString *)attribute {

    // activate accessibility updates
    QGuiApplicationPrivate::platformIntegration()->accessibility()->setActive(true);

    if ([attribute isEqualToString:NSAccessibilityRoleAttribute]) {
        if (m_window->accessibleRoot())
            return QCocoaAccessible::macRole(m_window->accessibleRoot());
        return NSAccessibilityUnknownRole;
    } else if ([attribute isEqualToString:NSAccessibilityRoleDescriptionAttribute]) {
        return NSAccessibilityRoleDescriptionForUIElement(self);
    } else if ([attribute isEqualToString:NSAccessibilityChildrenAttribute]) {
        if (!m_window->accessibleRoot())
            return [super accessibilityAttributeValue:attribute];
        return QCocoaAccessible::unignoredChildren(self, m_window->accessibleRoot());
    } else {
        return [super accessibilityAttributeValue:attribute];
    }
}

- (id)accessibilityHitTest:(NSPoint)point {
    if (!m_window->accessibleRoot())
        return [super accessibilityHitTest:point];

    QAccessibleInterface *childInterface = m_window->accessibleRoot()->childAt(point.x, qt_mac_flipYCoordinate(point.y));
    // No child found, meaning we hit the NSView
    if (!childInterface) {
        return [super accessibilityHitTest:point];
    }

    // Hit a child, forward to child accessible interface.
    QAccessible::Id childAxid = QAccessible::uniqueId(childInterface);
    // FIXME: parent could be wrong
    QCocoaAccessibleElement *accessibleElement = [QCocoaAccessibleElement createElementWithId:childAxid parent:self ];
    [accessibleElement autorelease];
    return [accessibleElement accessibilityHitTest:point];
}

- (id)accessibilityFocusedUIElement {
    if (!m_window->accessibleRoot())
        return [super accessibilityFocusedUIElement];

    QAccessibleInterface *childInterface = m_window->accessibleRoot()->focusChild();
    if (childInterface) {
        QAccessible::Id childAxid = QAccessible::uniqueId(childInterface);
        // FIXME: parent could be wrong
        QCocoaAccessibleElement *accessibleElement = [QCocoaAccessibleElement createElementWithId:childAxid parent:self];
        [accessibleElement autorelease];
        return accessibleElement;
    }

    // should not happen
    return nil;
}

@end
