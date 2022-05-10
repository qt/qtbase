// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#import <UIKit/UIKit.h>
#include <QtCore/QtGlobal>

QT_BEGIN_NAMESPACE

class QIOSScreen;

QT_END_NAMESPACE

@interface QIOSViewController : UIViewController

- (instancetype)initWithQIOSScreen:(QT_PREPEND_NAMESPACE(QIOSScreen) *)screen;
- (void)updateProperties;
- (NSArray*)keyCommands;
- (void)handleShortcut:(UIKeyCommand*)keyCommand;

#ifndef Q_OS_TVOS
@property (nonatomic, assign) UIInterfaceOrientation lockedOrientation;

// UIViewController
@property (nonatomic, assign) BOOL prefersStatusBarHidden;
@property (nonatomic, assign) UIStatusBarAnimation preferredStatusBarUpdateAnimation;
@property (nonatomic, assign) UIStatusBarStyle preferredStatusBarStyle;
#endif

@end

