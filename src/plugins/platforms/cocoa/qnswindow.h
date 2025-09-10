// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QNSWINDOW_H
#define QNSWINDOW_H

#include <qglobal.h>
#include <QPointer>
#include <QtCore/private/qcore_mac_p.h>

#include <AppKit/NSWindow.h>
#include <AppKit/NSPanel.h>

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

QCocoaNSWindow *qnswindow_cast(NSWindow *window);

#else
class QCocoaNSWindow;
#endif // __OBJC__

QT_DECLARE_NAMESPACED_OBJC_INTERFACE(QNSWindow, NSWindow <QNSWindowProtocol>)
QT_DECLARE_NAMESPACED_OBJC_INTERFACE(QNSPanel, NSPanel <QNSWindowProtocol>)

#endif // QNSWINDOW_H
