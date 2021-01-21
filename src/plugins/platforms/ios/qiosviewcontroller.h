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

#import <UIKit/UIKit.h>
#include <QtCore/QtGlobal>

QT_BEGIN_NAMESPACE

class QIOSScreen;

QT_END_NAMESPACE

@interface QIOSViewController : UIViewController

- (instancetype)initWithQIOSScreen:(QT_PREPEND_NAMESPACE(QIOSScreen) *)screen;
- (void)updateProperties;

#ifndef Q_OS_TVOS
@property (nonatomic, assign) UIInterfaceOrientation lockedOrientation;

// UIViewController
@property (nonatomic, assign) BOOL prefersStatusBarHidden;
@property (nonatomic, assign) UIStatusBarAnimation preferredStatusBarUpdateAnimation;
@property (nonatomic, assign) UIStatusBarStyle preferredStatusBarStyle;
#endif

@end

