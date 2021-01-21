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
#ifndef QCOCOAACCESIBILITYELEMENT_H
#define QCOCOAACCESIBILITYELEMENT_H

#include <QtCore/qglobal.h>

#include <QtCore/private/qcore_mac_p.h>

#ifndef QT_NO_ACCESSIBILITY

#import <Cocoa/Cocoa.h>
#import <AppKit/NSAccessibility.h>

#import <qaccessible.h>

@interface QT_MANGLE_NAMESPACE(QMacAccessibilityElement) : NSObject <NSAccessibilityElement>

- (instancetype)initWithId:(QAccessible::Id)anId;
+ (instancetype)elementWithId:(QAccessible::Id)anId;

@end

QT_NAMESPACE_ALIAS_OBJC_CLASS(QMacAccessibilityElement);

#endif // QT_NO_ACCESSIBILITY

#endif // QCOCOAACCESIBILITYELEMENT_H
