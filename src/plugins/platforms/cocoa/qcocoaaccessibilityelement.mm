/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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
#include "qcocoaaccessibilityelement.h"
#include "qcocoaaccessibility.h"
#include "qcocoahelpers.h"

#include <QAccessible>
#include "QAccessibleActionInterface"

#import <AppKit/NSAccessibility.h>

#ifndef QT_NO_COCOA_ACCESSIBILITY

static QAccessibleInterface *acast(void *ptr)
{
    return reinterpret_cast<QAccessibleInterface *>(ptr);
}

@implementation QCocoaAccessibleElement

- (id)initWithInterface:(void *)anQAccessibleInterface parent:(id)aParent
{
    self = [super init];
    if (self) {
        accessibleInterface = anQAccessibleInterface;
        role = QCocoaAccessible::macRole(acast(accessibleInterface));
        parent = aParent;
    }

    return self;
}

+ (QCocoaAccessibleElement *)elementWithInterface:(void *)anQAccessibleInterface parent:(id)aParent
{
    return [[[self alloc] initWithInterface:anQAccessibleInterface parent:aParent] autorelease];
}

- (void)dealloc {
    [super dealloc];
    delete acast(accessibleInterface);
}

- (BOOL)isEqual:(id)object {
    if ([object isKindOfClass:[QCocoaAccessibleElement class]]) {
        QCocoaAccessibleElement *other = object;
        return acast(other->accessibleInterface)->object() == acast(accessibleInterface)->object();
    } else {
        return NO;
    }
}

- (NSUInteger)hash {
    return qHash(acast(accessibleInterface)->object());
}

//
// accessibility protocol
//

// attributes

- (NSArray *)accessibilityAttributeNames {
    static NSArray *defaultAttributes = nil;
    if (defaultAttributes == nil) {
        defaultAttributes = [[NSArray alloc] initWithObjects:
        NSAccessibilityRoleAttribute,
        NSAccessibilityRoleDescriptionAttribute,
        NSAccessibilityChildrenAttribute,
        NSAccessibilityFocusedAttribute,
        NSAccessibilityParentAttribute,
        NSAccessibilityWindowAttribute,
        NSAccessibilityTopLevelUIElementAttribute,
        NSAccessibilityPositionAttribute,
        NSAccessibilitySizeAttribute,
        NSAccessibilityDescriptionAttribute,
        NSAccessibilityEnabledAttribute,
        nil];
    }

    NSMutableArray *attributes = [[NSMutableArray alloc] initWithCapacity : [defaultAttributes count]];
    [attributes addObjectsFromArray : defaultAttributes];

    if (QCocoaAccessible::hasValueAttribute(acast(accessibleInterface))) {
        [attributes addObject : NSAccessibilityValueAttribute];
    }

    return attributes;
}

- (id)accessibilityAttributeValue:(NSString *)attribute {
    if ([attribute isEqualToString:NSAccessibilityRoleAttribute]) {
        return role;
    } else if ([attribute isEqualToString:NSAccessibilityRoleDescriptionAttribute]) {
        return NSAccessibilityRoleDescription(role, nil);
    } else if ([attribute isEqualToString:NSAccessibilityChildrenAttribute]) {
        int numKids = acast(accessibleInterface)->childCount();

        NSMutableArray *kids = [NSMutableArray arrayWithCapacity:numKids];
        for (int i = 0; i < numKids; ++i) {
            QAccessibleInterface *childInterface = acast(accessibleInterface)->child(i);
            [kids addObject:[QCocoaAccessibleElement elementWithInterface:(void*)childInterface parent:self]];
        }

        return NSAccessibilityUnignoredChildren(kids);
    } else if ([attribute isEqualToString:NSAccessibilityFocusedAttribute]) {
        // Just check if the app thinks we're focused.
        id focusedElement = [NSApp accessibilityAttributeValue:NSAccessibilityFocusedUIElementAttribute];
        return [NSNumber numberWithBool:[focusedElement isEqual:self]];
    } else if ([attribute isEqualToString:NSAccessibilityParentAttribute]) {
        return NSAccessibilityUnignoredAncestor(parent);
    } else if ([attribute isEqualToString:NSAccessibilityWindowAttribute]) {
        // We're in the same window as our parent.
        return [parent accessibilityAttributeValue:NSAccessibilityWindowAttribute];
    } else if ([attribute isEqualToString:NSAccessibilityTopLevelUIElementAttribute]) {
        // We're in the same top level element as our parent.
        return [parent accessibilityAttributeValue:NSAccessibilityTopLevelUIElementAttribute];
    } else if ([attribute isEqualToString:NSAccessibilityPositionAttribute]) {
        QPoint qtPosition = acast(accessibleInterface)->rect().topLeft();
        QSize qtSize = acast(accessibleInterface)->rect().size();
        return [NSValue valueWithPoint: NSMakePoint(qtPosition.x(), qt_mac_flipYCoordinate(qtPosition.y() + qtSize.height()))];
    } else if ([attribute isEqualToString:NSAccessibilitySizeAttribute]) {
        QSize qtSize = acast(accessibleInterface)->rect().size();
        return [NSValue valueWithSize: NSMakeSize(qtSize.width(), qtSize.height())];
    } else if ([attribute isEqualToString:NSAccessibilityDescriptionAttribute]) {
        return QCFString::toNSString(acast(accessibleInterface)->text(QAccessible::Name));
    } else if ([attribute isEqualToString:NSAccessibilityEnabledAttribute]) {
        return [NSNumber numberWithBool:!acast(accessibleInterface)->state().disabled];
    } else if ([attribute isEqualToString:NSAccessibilityValueAttribute]) {
        // VoiceOver asks for the value attribute for all elements. Return nil
        // if we don't want the element to have a value attribute.
        if (!QCocoaAccessible::hasValueAttribute(acast(accessibleInterface)))
            return nil;

        return QCocoaAccessible::getValueAttribute(acast(accessibleInterface));
    }

    return nil;
}

- (BOOL)accessibilityIsAttributeSettable:(NSString *)attribute {
    if ([attribute isEqualToString:NSAccessibilityFocusedAttribute]) {
        return NO; // YES to handle keyboard input
    } else {
        return NO;
    }
}

- (void)accessibilitySetValue:(id)value forAttribute:(NSString *)attribute {
    Q_UNUSED(value);
    if ([attribute isEqualToString:NSAccessibilityFocusedAttribute]) {

    }
}

// actions

- (NSArray *)accessibilityActionNames {
    NSMutableArray * nsActions = [NSMutableArray new];

    QAccessibleActionInterface *actionInterface = acast(accessibleInterface)->actionInterface();
    if (actionInterface) {
        QStringList supportedActionNames = actionInterface->actionNames();

        foreach (const QString &qtAction, supportedActionNames) {
            NSString *nsAction = QCocoaAccessible::getTranslatedAction(qtAction);
            if (nsAction)
                [nsActions addObject : nsAction];
        }
    }

    return nsActions;
}

- (NSString *)accessibilityActionDescription:(NSString *)action {
    QAccessibleActionInterface *actionInterface = acast(accessibleInterface)->actionInterface();
    QString qtAction = QCocoaAccessible::translateAction(action);

    // Return a description from the action interface if this action is not known to the OS.
    if (qtAction.isEmpty()) {
        QString description = actionInterface->localizedActionDescription(qtAction);
        return QCFString::toNSString(description);
    }

    return NSAccessibilityActionDescription(action);
}

- (void)accessibilityPerformAction:(NSString *)action {
    QAccessibleActionInterface *actionInterface = acast(accessibleInterface)->actionInterface();
    if (actionInterface) {
        actionInterface->doAction(QCocoaAccessible::translateAction(action));
    }
}

// misc

- (BOOL)accessibilityIsIgnored {
    return QCocoaAccessible::shouldBeIgnrored(acast(accessibleInterface));
}

- (id)accessibilityHitTest:(NSPoint)point {

    if (!accessibleInterface)
        return NSAccessibilityUnignoredAncestor(self);
    QAccessibleInterface *childInterface = acast(accessibleInterface)->childAt(point.x, qt_mac_flipYCoordinate(point.y));

    // No child found, meaning we hit this element.
    if (!childInterface) {
        return NSAccessibilityUnignoredAncestor(self);
    }

    // hit a child, forward to child accessible interface.
    QCocoaAccessibleElement *accessibleElement = [QCocoaAccessibleElement elementWithInterface:childInterface parent:self];
    return [accessibleElement accessibilityHitTest:point];
}

- (id)accessibilityFocusedUIElement {
    return NSAccessibilityUnignoredAncestor(self);
}

@end

#endif // QT_NO_COCOA_ACCESSIBILITY

