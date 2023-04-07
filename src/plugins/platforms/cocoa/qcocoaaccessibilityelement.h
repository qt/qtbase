// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCOCOAACCESIBILITYELEMENT_H
#define QCOCOAACCESIBILITYELEMENT_H

#include <QtGui/qtguiglobal.h>

#if QT_CONFIG(accessibility)

#include <QtCore/private/qcore_mac_p.h>
#include <QtGui/qaccessible.h>

QT_DECLARE_NAMESPACED_OBJC_INTERFACE(QMacAccessibilityElement, NSObject <NSAccessibilityElement>
- (instancetype)initWithId:(QAccessible::Id)anId;
- (instancetype)initWithId:(QAccessible::Id)anId role:(NSAccessibilityRole)role;
+ (instancetype)elementWithId:(QAccessible::Id)anId;
+ (instancetype)elementWithInterface:(QAccessibleInterface *)iface;
- (void)updateTableModel;
- (QAccessibleInterface *)qtInterface;
)

#endif // QT_CONFIG(accessibility)

#endif // QCOCOAACCESIBILITYELEMENT_H
