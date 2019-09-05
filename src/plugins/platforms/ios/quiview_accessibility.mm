/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
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
