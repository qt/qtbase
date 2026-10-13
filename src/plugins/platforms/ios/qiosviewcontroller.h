// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#import <UIKit/UIKit.h>
#include <QtCore/QtGlobal>

QT_BEGIN_NAMESPACE

class QIOSScreen;

QT_END_NAMESPACE

@interface QIOSViewController : UIViewController

- (instancetype)initWithWindow:(UIWindow*)window;
- (void)updateStatusBarProperties;
- (NSArray*)keyCommands;
- (void)handleShortcut:(UIKeyCommand*)keyCommand;
- (void)updatePlatformScreen;


#ifndef Q_OS_TVOS
// UIViewController
@property (nonatomic, assign) BOOL prefersStatusBarHidden;
@property (nonatomic, assign) UIStatusBarAnimation preferredStatusBarUpdateAnimation;
@property (nonatomic, assign) UIStatusBarStyle preferredStatusBarStyle;
#endif

@end

