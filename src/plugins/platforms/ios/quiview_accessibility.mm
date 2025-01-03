// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qiosplatformaccessibility.h"
#include "quiaccessibilityelement.h"

#include <QtGui/private/qguiapplication_p.h>

@implementation QUIView (Accessibility)

- (void)createAccessibleElement:(QAccessibleInterface *)iface
{
    if (!iface || iface->state().invisible || (iface->text(QAccessible::Name).isEmpty() && iface->text(QAccessible::Value).isEmpty() && iface->text(QAccessible::Description).isEmpty()))
        return;
    QAccessible::Id accessibleId = QAccessible::uniqueId(iface);
    UIAccessibilityElement *elem = [[QT_MANGLE_NAMESPACE(QMacAccessibilityElement) alloc] initWithId:accessibleId withAccessibilityContainer:self];
    [m_accessibleElements addObject:elem];
    [elem release];
}

- (void)createAccessibleContainer:(QAccessibleInterface *)iface
{
    if (!iface || iface->state().invisible)
        return;

    for (int i = 0; i < iface->childCount(); ++i)
        [self createAccessibleContainer: iface->child(i)];

    // The container element must go last, so that it underlays all its children
    [self createAccessibleElement:iface];
}

- (void)initAccessibility
{
    // The window may have gone away, but with the view
    // temporarily caught in the a11y subsystem.
    if (!self.platformWindow)
        return;

    static bool init = false;
    if (!init)
        QGuiApplicationPrivate::platformIntegration()->accessibility()->setActive(true);
    init = true;

    if ([m_accessibleElements count])
        return;

    QWindow *win = self.platformWindow->window();
    QAccessibleInterface *iface = win->accessibleRoot();
    if (iface)
        [self createAccessibleContainer: iface];
}

- (void)clearAccessibleCache
{
    [m_accessibleElements removeAllObjects];
}

// this is a container, returning yes here means the functions below will never be called
- (BOOL)isAccessibilityElement
{
    return NO;
}

- (NSInteger)accessibilityElementCount
{
    [self initAccessibility];
    return [m_accessibleElements count];
}

- (id)accessibilityElementAtIndex:(NSInteger)index
{
    [self initAccessibility];
    if (NSUInteger(index) >= [m_accessibleElements count])
        return nil;
    return m_accessibleElements[index];
}

- (NSInteger)indexOfAccessibilityElement:(id)element
{
    [self initAccessibility];
    return [m_accessibleElements indexOfObject:element];
}

- (NSArray *)accessibilityElements
{
    [self initAccessibility];
    return m_accessibleElements;
}

@end
