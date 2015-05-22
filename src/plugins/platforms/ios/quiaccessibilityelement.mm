/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "quiaccessibilityelement.h"

#include "private/qaccessiblecache_p.h"

@implementation QMacAccessibilityElement

- (id)initWithId:(QAccessible::Id)anId withAccessibilityContainer:(id)view
{
    Q_ASSERT((int)anId < 0);
    self = [super initWithAccessibilityContainer: view];
    if (self)
        _axid = anId;

    return self;
}

+ (id)elementWithId:(QAccessible::Id)anId withAccessibilityContainer:(id)view
{
    Q_ASSERT(anId);
    if (!anId)
        return nil;

    QAccessibleCache *cache = QAccessibleCache::instance();

    QMacAccessibilityElement *element = cache->elementForId(anId);
    if (!element) {
        Q_ASSERT(QAccessible::accessibleInterface(anId));
        element = [[self alloc] initWithId:anId withAccessibilityContainer: view];
        cache->insertElement(anId, element);
    }
    return element;
}

- (void)invalidate
{
    [self release];
}

- (BOOL)isAccessibilityElement
{
    return YES;
}

- (NSString*)accessibilityLabel
{
    QAccessibleInterface *iface = QAccessible::accessibleInterface(self.axid);
    if (!iface) {
        qWarning() << "invalid accessible interface for: " << self.axid;
        return @"";
    }

    return iface->text(QAccessible::Name).toNSString();
}

- (NSString*)accessibilityHint
{
    QAccessibleInterface *iface = QAccessible::accessibleInterface(self.axid);
    if (!iface) {
        qWarning() << "invalid accessible interface for: " << self.axid;
        return @"";
    }
    return iface->text(QAccessible::Description).toNSString();
}

- (NSString*)accessibilityValue
{
    QAccessibleInterface *iface = QAccessible::accessibleInterface(self.axid);
    if (!iface) {
        qWarning() << "invalid accessible interface for: " << self.axid;
        return @"";
    }

    QAccessible::State state = iface->state();

    if (state.checkable)
        return state.checked ? @"checked" : @"unchecked"; // FIXME: translation

    QAccessibleValueInterface *val = iface->valueInterface();
    if (val) {
        return val->currentValue().toString().toNSString();
    } else if (QAccessibleTextInterface *text = iface->textInterface()) {
        // FIXME doesn't work?
        return text->text(0, text->characterCount() - 1).toNSString();
    }

    return [super accessibilityHint];
}

- (CGRect)accessibilityFrame
{
    QAccessibleInterface *iface = QAccessible::accessibleInterface(self.axid);
    if (!iface) {
        qWarning() << "invalid accessible interface for: " << self.axid;
        return CGRect();
    }

    QRect rect = iface->rect();
    return CGRectMake(rect.x(), rect.y(), rect.width(), rect.height());
}

- (UIAccessibilityTraits)accessibilityTraits
{
    UIAccessibilityTraits traits = UIAccessibilityTraitNone;

    QAccessibleInterface *iface = QAccessible::accessibleInterface(self.axid);
    if (!iface) {
        qWarning() << "invalid accessible interface for: " << self.axid;
        return traits;
    }
    QAccessible::State state = iface->state();
    if (state.disabled)
        traits |= UIAccessibilityTraitNotEnabled;

    if (state.searchEdit)
        traits |= UIAccessibilityTraitSearchField;

    if (iface->role() == QAccessible::Button)
        traits |= UIAccessibilityTraitButton;

    if (iface->valueInterface())
        traits |= UIAccessibilityTraitAdjustable;

    return traits;
}

- (BOOL)accessibilityActivate
{
    QAccessibleInterface *iface = QAccessible::accessibleInterface(self.axid);
    if (QAccessibleActionInterface *action = iface->actionInterface()) {
        if (action->actionNames().contains(QAccessibleActionInterface::pressAction())) {
            action->doAction(QAccessibleActionInterface::pressAction());
            return YES;
        } else if (action->actionNames().contains(QAccessibleActionInterface::showMenuAction())) {
            action->doAction(QAccessibleActionInterface::showMenuAction());
            return YES;
        }
    }
    return NO; // fall back to sending mouse clicks
}

- (void)accessibilityIncrement
{
    QAccessibleInterface *iface = QAccessible::accessibleInterface(self.axid);
    if (QAccessibleActionInterface *action = iface->actionInterface())
        action->doAction(QAccessibleActionInterface::increaseAction());
}

- (void)accessibilityDecrement
{
    QAccessibleInterface *iface = QAccessible::accessibleInterface(self.axid);
    if (QAccessibleActionInterface *action = iface->actionInterface())
        action->doAction(QAccessibleActionInterface::decreaseAction());
}

- (BOOL)accessibilityScroll:(UIAccessibilityScrollDirection)direction
{
    QAccessibleInterface *iface = QAccessible::accessibleInterface(self.axid);
    QAccessibleActionInterface *action = iface->actionInterface();
    if (!action)
        return NO;
    switch (direction) {
    case UIAccessibilityScrollDirectionRight:
        action->doAction(QAccessibleActionInterface::scrollRightAction());
        return YES;
    case UIAccessibilityScrollDirectionLeft:
        action->doAction(QAccessibleActionInterface::scrollLeftAction());
        return YES;
    case UIAccessibilityScrollDirectionUp:
        action->doAction(QAccessibleActionInterface::scrollUpAction());
        return YES;
    case UIAccessibilityScrollDirectionDown:
        action->doAction(QAccessibleActionInterface::scrollDownAction());
        return YES;
    case UIAccessibilityScrollDirectionNext:
        action->doAction(QAccessibleActionInterface::nextPageAction());
        return YES;
    case UIAccessibilityScrollDirectionPrevious:
        action->doAction(QAccessibleActionInterface::previousPageAction());
        return YES;
    }
    return NO;
}

@end
