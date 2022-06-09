// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "quiaccessibilityelement.h"

#if QT_CONFIG(accessibility)

#include "private/qaccessiblecache_p.h"
#include "private/qcore_mac_p.h"
#include "uistrings_p.h"

QT_NAMESPACE_ALIAS_OBJC_CLASS(QMacAccessibilityElement);

@implementation QMacAccessibilityElement

- (instancetype)initWithId:(QAccessible::Id)anId withAccessibilityContainer:(id)view
{
    Q_ASSERT((int)anId < 0);
    self = [super initWithAccessibilityContainer:view];
    if (self)
        _axid = anId;

    return self;
}

+ (instancetype)elementWithId:(QAccessible::Id)anId withAccessibilityContainer:(id)view
{
    Q_ASSERT(anId);
    if (!anId)
        return nil;

    QAccessibleCache *cache = QAccessibleCache::instance();

    QMacAccessibilityElement *element = cache->elementForId(anId);
    if (!element) {
        Q_ASSERT(QAccessible::accessibleInterface(anId));
        element = [[self alloc] initWithId:anId withAccessibilityContainer:view];
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
        return state.checked
                ? QCoreApplication::translate(ACCESSIBILITY_ELEMENT, AE_CHECKED).toNSString()
                : QCoreApplication::translate(ACCESSIBILITY_ELEMENT, AE_UNCHECKED).toNSString();

    QAccessibleValueInterface *val = iface->valueInterface();
    if (val) {
        return val->currentValue().toString().toNSString();
    } else if (QAccessibleTextInterface *text = iface->textInterface()) {
        return text->text(0, text->characterCount()).toNSString();
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

    if (state.selected)
        traits |= UIAccessibilityTraitSelected;

    const auto accessibleRole = iface->role();
    if (accessibleRole == QAccessible::Button) {
        traits |= UIAccessibilityTraitButton;
    } else if (accessibleRole == QAccessible::EditableText) {
        static auto defaultTextFieldTraits = []{
            auto *textField = [[[UITextField alloc] initWithFrame:CGRectZero] autorelease];
            return textField.accessibilityTraits;
        }();
        traits |= defaultTextFieldTraits;
    } else if (accessibleRole == QAccessible::Graphic) {
        traits |= UIAccessibilityTraitImage;
    } else if (accessibleRole == QAccessible::Heading) {
        traits |= UIAccessibilityTraitHeader;
    } else if (accessibleRole == QAccessible::Link) {
        traits |= UIAccessibilityTraitLink;
    } else if (accessibleRole == QAccessible::StaticText) {
        traits |= UIAccessibilityTraitStaticText;
    }

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

#endif
