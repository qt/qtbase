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
    if (!iface)
        return;

    [self createAccessibleElement: iface];
    for (int i = 0; i < iface->childCount(); ++i)
        [self createAccessibleContainer: iface->child(i)];
}

- (void)initAccessibility
{
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
    UIAccessibilityPostNotification(UIAccessibilityScreenChangedNotification, @"");
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
