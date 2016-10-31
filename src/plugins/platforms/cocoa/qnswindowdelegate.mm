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
#include <qpa/qwindowsysteminterface.h>

@implementation QNSWindowDelegate

- (id) initWithQCocoaWindow: (QCocoaWindow *) cocoaWindow
{
    self = [super init];

    if (self) {
        m_cocoaWindow = cocoaWindow;
    }
    return self;
}

- (void)windowDidBecomeKey:(NSNotification *)notification
{
    Q_UNUSED(notification);
    if (m_cocoaWindow->m_windowUnderMouse) {
        QPointF windowPoint;
        QPointF screenPoint;
        [qnsview_cast(m_cocoaWindow->view()) convertFromScreen:[NSEvent mouseLocation] toWindowPoint:&windowPoint andScreenPoint:&screenPoint];
        QWindowSystemInterface::handleEnterEvent(m_cocoaWindow->m_enterLeaveTargetWindow, windowPoint, screenPoint);
    }
}

- (void)windowDidResize:(NSNotification *)notification
{
    Q_UNUSED(notification);
    if (m_cocoaWindow) {
        m_cocoaWindow->windowDidResize();
    }
}

- (void)windowDidEndLiveResize:(NSNotification *)notification
{
    Q_UNUSED(notification);
    if (m_cocoaWindow) {
        m_cocoaWindow->windowDidEndLiveResize();
    }
}

- (void)windowWillMove:(NSNotification *)notification
{
    Q_UNUSED(notification);
    if (m_cocoaWindow) {
        m_cocoaWindow->windowWillMove();
    }
}

- (void)windowDidMove:(NSNotification *)notification
{
    Q_UNUSED(notification);
    if (m_cocoaWindow) {
        m_cocoaWindow->windowDidMove();
    }
}

- (BOOL)windowShouldClose:(NSNotification *)notification
{
    Q_UNUSED(notification);
    if (m_cocoaWindow) {
        return m_cocoaWindow->windowShouldClose();
    }

    return YES;
}

- (BOOL)windowShouldZoom:(NSWindow *)window toFrame:(NSRect)newFrame
{
    Q_UNUSED(newFrame);
    if (m_cocoaWindow && m_cocoaWindow->window()->type() != Qt::ForeignWindow)
        [qnsview_cast(m_cocoaWindow->view()) notifyWindowWillZoom:![window isZoomed]];
    return YES;
}

- (void)windowWillClose:(NSNotification *)notification
{
    Q_UNUSED(notification);
    if (m_cocoaWindow)
        m_cocoaWindow->windowWillClose();
}

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
