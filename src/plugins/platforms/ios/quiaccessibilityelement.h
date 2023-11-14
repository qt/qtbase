// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#ifndef QUIACCESSIBILITYELEMENT_H
#define QUIACCESSIBILITYELEMENT_H

#import <UIKit/UIKit.h>
#import <QtGui/QtGui>

#if QT_CONFIG(accessibility)

@interface QT_MANGLE_NAMESPACE(QMacAccessibilityElement) : UIAccessibilityElement

@property (readonly) QAccessible::Id axid;

- (instancetype)initWithId:(QAccessible::Id)anId withAccessibilityContainer:(id)view;
+ (instancetype)elementWithId:(QAccessible::Id)anId withAccessibilityContainer:(id)view;

@end

#endif
#endif
