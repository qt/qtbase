/****************************************************************************
 **
 ** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
 ** All rights reserved.
 ** Contact: http://www.qt-project.org/
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
#include "qcocoaaccessibilityelement.h"
#include "qcocoaaccessibility.h"
#include "qcocoahelpers.h"

#include <QAccessible>
#include "QAccessibleActionInterface"

#import <AppKit/NSAccessibility.h>

static QAccessibleInterface *acast(void *ptr)
{
    return reinterpret_cast<QAccessibleInterface *>(ptr);
}

@implementation QCocoaAccessibleElement

- (id)initWithIndex:(int)aIndex parent:(id)aParent accessibleInterface:(void *)anQAccessibleInterface
{
    self = [super init];
    if (self) {
        index = aIndex;
        accessibleInterface = anQAccessibleInterface;
        role = QCocoaAccessible::macRole(acast(accessibleInterface)->role());
        parent = aParent;

    }

    return self;
}

+ (QCocoaAccessibleElement *)elementWithIndex:(int)aIndex parent:(id)aParent accessibleInterface:(void *)anQAccessibleInterface
{
    return [[[self alloc] initWithIndex:aIndex parent:aParent accessibleInterface:anQAccessibleInterface] autorelease];
}

- (void)dealloc {
    [super dealloc];
}

- (BOOL)isEqual:(id)object {
    if ([object isKindOfClass:[QCocoaAccessibleElement class]]) {
        QCocoaAccessibleElement *other = object;
        return (index == other->index) && [role isEqualToString:other->role] && [parent isEqual:other->parent];
    } else
        return NO;
}

- (NSUInteger)hash {
    return [parent hash] + index;
}

- (NSUInteger)index {
    return index;
}

//
// accessibility protocol
//

// attributes

- (NSArray *)accessibilityAttributeNames {
    static NSArray *attributes = nil;
    if (attributes == nil) {
        attributes = [[NSArray alloc] initWithObjects:
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
        nil];
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
            [kids addObject:[QCocoaAccessibleElement elementWithIndex:i parent:self accessibleInterface:(void*)childInterface]];
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
        return qt_mac_QStringToNSString(acast(accessibleInterface)->text(QAccessible::Name));
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
        return qt_mac_QStringToNSString(description);
    }

    return NSAccessibilityActionDescription(action);
}

- (void)accessibilityPerformAction:(NSString *)action {
    QAccessibleActionInterface *actionInterface = acast(accessibleInterface)->actionInterface();
    if (actionInterface) {
        QString qtAction = QCocoaAccessible::translateAction(action);
        actionInterface->doAction(QAccessibleActionInterface::pressAction());
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
    int childIndex = acast(accessibleInterface)->indexOfChild(childInterface);
    QCocoaAccessibleElement *accessibleElement = [QCocoaAccessibleElement elementWithIndex:childIndex parent:self accessibleInterface: childInterface];
    return [accessibleElement accessibilityHitTest:point];
}

- (id)accessibilityFocusedUIElement {
    return NSAccessibilityUnignoredAncestor(self);
}

@end
