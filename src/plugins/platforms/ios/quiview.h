// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#import <UIKit/UIKit.h>

#include <qhash.h>
#include <qstring.h>

#include <qpa/qwindowsysteminterface.h>

QT_BEGIN_NAMESPACE

class QIOSWindow;

QT_END_NAMESPACE

@class QIOSViewController;

@interface QUIView : UIView
- (instancetype)initWithQIOSWindow:(QT_PREPEND_NAMESPACE(QIOSWindow) *)window;
- (void)sendUpdatedExposeEvent;
- (BOOL)isActiveWindow;
- (bool)handlePresses:(NSSet<UIPress *> *)presses eventType:(QEvent::Type)type;
@property (nonatomic, assign) QT_PREPEND_NAMESPACE(QIOSWindow) *platformWindow;
@end

@interface QUIView (Accessibility)
- (void)clearAccessibleCache;
@end

@interface UIView (QtHelpers)
- (QWindow *)qwindow;
- (UIViewController *)viewController;
- (QIOSViewController*)qtViewController;
@property (nonatomic, readonly) UIEdgeInsets qt_safeAreaInsets;
@end

#ifdef Q_OS_IOS
@interface QUIMetalView : QUIView
@end
#endif
