/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

#ifndef QCOCOAACCESIBILITYELEMENT_H
#define QCOCOAACCESIBILITYELEMENT_H

#ifndef QT_NO_ACCESSIBILITY

#include <QtCore/qglobal.h>

#include <QtCore/private/qcore_mac_p.h>
#include <QtGui/qaccessible.h>

QT_DECLARE_NAMESPACED_OBJC_INTERFACE(QMacAccessibilityElement, NSObject <NSAccessibilityElement>
- (instancetype)initWithId:(QAccessible::Id)anId;
+ (instancetype)elementWithId:(QAccessible::Id)anId;
)

#endif // QT_NO_ACCESSIBILITY

#endif // QCOCOAACCESIBILITYELEMENT_H
