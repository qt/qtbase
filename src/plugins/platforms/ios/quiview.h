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
