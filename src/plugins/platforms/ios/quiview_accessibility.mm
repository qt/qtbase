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

#include "qiosplatformaccessibility.h"
#include "quiaccessibilityelement.h"

#include <QtGui/private/qguiapplication_p.h>

@implementation QUIView (Accessibility)

- (void)createAccessibleElement:(QAccessibleInterface *)iface
{
    if (!iface || iface->state().invisible || (iface->text(QAccessible::Name).isEmpty() && iface->text(QAccessible::Value).isEmpty() && iface->text(QAccessible::Description).isEmpty()))
        return;
    QAccessible::Id accessibleId = QAccessible::uniqueId(iface);
    UIAccessibilityElement *elem = [[QMacAccessibilityElement alloc] initWithId: accessibleId withAccessibilityContainer: self];
    [m_accessibleElements addObject: elem];
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

    QWindow *win = m_qioswindow->window();
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
    return m_accessibleElements[index];
}

- (NSInteger)indexOfAccessibilityElement:(id)element
{
    [self initAccessibility];
    return [m_accessibleElements indexOfObject:element];
}

@end
