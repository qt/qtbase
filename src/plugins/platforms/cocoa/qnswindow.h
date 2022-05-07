/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

#ifndef QNSWINDOW_H
#define QNSWINDOW_H

#include <qglobal.h>
#include <QPointer>
#include <QtCore/private/qcore_mac_p.h>

QT_FORWARD_DECLARE_CLASS(QCocoaWindow)

#if defined(__OBJC__)

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

#else
class QCocoaNSWindow;
#endif // __OBJC__

QT_DECLARE_NAMESPACED_OBJC_INTERFACE(QNSWindow, NSWindow <QNSWindowProtocol>)
QT_DECLARE_NAMESPACED_OBJC_INTERFACE(QNSPanel, NSPanel <QNSWindowProtocol>)

#endif // QNSWINDOW_H
