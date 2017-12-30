/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
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
{
  @public
    QT_PREPEND_NAMESPACE(QIOSWindow) *m_qioswindow;
  @private
    QHash<UITouch *, QWindowSystemInterface::TouchPoint> m_activeTouches;
    UITouch *m_activePencilTouch;
    int m_nextTouchId;

  @private
    NSMutableArray *m_accessibleElements;
};

- (id)initWithQIOSWindow:(QT_PREPEND_NAMESPACE(QIOSWindow) *)window;
- (void)sendUpdatedExposeEvent;
- (BOOL)isActiveWindow;
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

