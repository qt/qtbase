// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "nativewindow.h"

#include <QtCore/qscopeguard.h>

#if defined(Q_OS_MACOS) || defined(QT_PLATFORM_UIKIT)

#if defined(Q_OS_MACOS)
#  include <AppKit/AppKit.h>
#  define VIEW_BASE NSView
#elif defined(Q_OS_IOS)
#  include <UIKit/UIKit.h>
#  define VIEW_BASE UIView
#endif

@interface View : VIEW_BASE
@end

@implementation View
- (instancetype)init
{
    if ((self = [super init])) {
#if defined(Q_OS_MACOS)
        self.wantsLayer = YES;
#endif
        self.layer.backgroundColor = CGColorCreateGenericRGB(1.0, 0.5, 1.0, 1.0);
    }
    return self;
}

- (void)dealloc
{
    [super dealloc];
}
@end

NativeWindow::NativeWindow()
    : m_handle([View new])
{
    m_handle.hidden = YES;
}

NativeWindow::~NativeWindow()
{
    if (m_handle.window.contentView == m_handle)
        [m_handle.window close];

    [m_handle release];
}

void NativeWindow::setGeometry(const QRect &rect)
{
    m_handle.frame = QRectF(rect).toCGRect();
}

QRect NativeWindow::geometry() const
{
    return QRectF::fromCGRect(m_handle.frame).toRect();
}

NativeWindow::operator WId() const
{
    return reinterpret_cast<WId>(m_handle);
}

WId NativeWindow::parentWinId() const
{
    return WId(m_handle.superview);
}

bool NativeWindow::isParentOf(WId childWinId)
{
    auto *subview = reinterpret_cast<Handle>(childWinId);
    return subview.superview == m_handle;
}

void NativeWindow::setParent(WId parent)
{
    if (auto *superview = reinterpret_cast<Handle>(parent))
        [superview addSubview:m_handle];
    else
        [m_handle removeFromSuperview];
}

#endif
