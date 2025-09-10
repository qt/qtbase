// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QUIACCESSIBILITYELEMENT_H
#define QUIACCESSIBILITYELEMENT_H

#import <UIKit/UIKit.h>
#import <QtGui/QtGui>

#if QT_CONFIG(accessibility)

@interface QT_MANGLE_NAMESPACE(QMacAccessibilityElement) : UIAccessibilityElement

@property (readonly) QAccessible::Id axid;

+ (instancetype)elementWithId:(QAccessible::Id)anId;

@end

#endif
#endif
