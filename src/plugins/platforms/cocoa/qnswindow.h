/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#ifndef QNSWINDOW_H
#define QNSWINDOW_H

#include <qglobal.h>
#include <QPointer>
#include <QtCore/private/qcore_mac_p.h>

#include <AppKit/AppKit.h>

QT_FORWARD_DECLARE_CLASS(QCocoaWindow)

// @compatibility_alias doesn't work with categories or their methods
#define FullScreenProperty QT_MANGLE_NAMESPACE(FullScreenProperty)
#define qt_fullScreen QT_MANGLE_NAMESPACE(qt_fullScreen)

@interface NSWindow (FullScreenProperty)
@property(readonly) BOOL qt_fullScreen;
@end

// @compatibility_alias doesn't work with protocols
#define QNSWindowProtocol QT_MANGLE_NAMESPACE(QNSWindowProtocol)

@protocol QNSWindowProtocol
- (instancetype)initWithContentRect:(NSRect)contentRect styleMask:(NSWindowStyleMask)style
    backing:(NSBackingStoreType)backingStoreType defer:(BOOL)flag screen:(NSScreen *)screen
    platformWindow:(QCocoaWindow*)window;
- (void)closeAndRelease;
@property (nonatomic, readonly) QCocoaWindow *platformWindow;
@end

typedef NSWindow<QNSWindowProtocol> QCocoaNSWindow;

@interface QT_MANGLE_NAMESPACE(QNSWindow) : NSWindow<QNSWindowProtocol> @end
QT_NAMESPACE_ALIAS_OBJC_CLASS(QNSWindow);

@interface QT_MANGLE_NAMESPACE(QNSPanel) : NSPanel<QNSWindowProtocol> @end
QT_NAMESPACE_ALIAS_OBJC_CLASS(QNSPanel);

#endif // QNSWINDOW_H
