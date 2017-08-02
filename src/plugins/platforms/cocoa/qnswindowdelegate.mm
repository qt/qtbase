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

#include "qnswindowdelegate.h"
#include "qcocoahelpers.h"

#include <QDebug>
#include <qpa/qplatformscreen.h>
#include <qpa/qwindowsysteminterface.h>

@implementation QNSWindowDelegate

- (id)initWithQCocoaWindow:(QCocoaWindow *)cocoaWindow
{
    if (self = [super init])
        m_cocoaWindow = cocoaWindow;

    return self;
}

- (BOOL)windowShouldClose:(NSNotification *)notification
{
    Q_UNUSED(notification);
    if (m_cocoaWindow) {
        return m_cocoaWindow->windowShouldClose();
    }

    return YES;
}
/*!
    Overridden to ensure that the zoomed state always results in a maximized
    window, which would otherwise not be the case for borderless windows.
*/
- (NSRect)windowWillUseStandardFrame:(NSWindow *)window defaultFrame:(NSRect)newFrame
{
    Q_UNUSED(newFrame);

    // We explicitly go through the QScreen API here instead of just using
    // window.screen.visibleFrame directly, as that ensures we have the same
    // behavior for both use-cases/APIs.
    Q_ASSERT(window == m_cocoaWindow->nativeWindow());
    return NSRectFromCGRect(m_cocoaWindow->screen()->availableGeometry().toCGRect());
}

#if QT_MACOS_DEPLOYMENT_TARGET_BELOW(__MAC_10_11)
/*
    AppKit on OS X 10.10 wrongly calls windowWillUseStandardFrame:defaultFrame
    from -[NSWindow _frameForFullScreenMode] when going into fullscreen, resulting
    in black bars on top and bottom of the window. By implementing the following
    method, AppKit will choose that instead, and resolve the right fullscreen
    geometry.
*/
- (NSSize)window:(NSWindow *)window willUseFullScreenContentSize:(NSSize)proposedSize
{
    Q_UNUSED(proposedSize);
    Q_ASSERT(window == m_cocoaWindow->nativeWindow());
    return NSSizeFromCGSize(m_cocoaWindow->screen()->geometry().size().toCGSize());
}
#endif

- (BOOL)window:(NSWindow *)window shouldPopUpDocumentPathMenu:(NSMenu *)menu
{
    Q_UNUSED(window);
    Q_UNUSED(menu);
    return m_cocoaWindow && m_cocoaWindow->m_hasWindowFilePath;
}

- (BOOL)window:(NSWindow *)window shouldDragDocumentWithEvent:(NSEvent *)event from:(NSPoint)dragImageLocation withPasteboard:(NSPasteboard *)pasteboard
{
    Q_UNUSED(window);
    Q_UNUSED(event);
    Q_UNUSED(dragImageLocation);
    Q_UNUSED(pasteboard);
    return m_cocoaWindow && m_cocoaWindow->m_hasWindowFilePath;
}
@end
