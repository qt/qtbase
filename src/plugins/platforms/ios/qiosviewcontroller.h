// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#import <UIKit/UIKit.h>
#include <QtCore/QtGlobal>

QT_BEGIN_NAMESPACE

class QIOSScreen;

QT_END_NAMESPACE

@interface QIOSViewController : UIViewController

- (instancetype)initWithWindow:(UIWindow*)window andScreen:(QT_PREPEND_NAMESPACE(QIOSScreen) *)screen;
- (void)updateProperties;
- (NSArray*)keyCommands;
- (void)handleShortcut:(UIKeyCommand*)keyCommand;

#ifndef Q_OS_TVOS
// UIViewController
@property (nonatomic, assign) BOOL prefersStatusBarHidden;
@property (nonatomic, assign) UIStatusBarAnimation preferredStatusBarUpdateAnimation;
@property (nonatomic, assign) UIStatusBarStyle preferredStatusBarStyle;
#endif

@end

