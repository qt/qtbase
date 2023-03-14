// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <AppKit/AppKit.h>

#include "qnswindowdelegate.h"
#include "qcocoahelpers.h"
#include "qcocoawindow.h"
#include "qcocoascreen.h"

#include <QDebug>
#include <QtCore/private/qcore_mac_p.h>
#include <qpa/qplatformscreen.h>
#include <qpa/qwindowsysteminterface.h>

static inline bool isWhiteSpace(const QString &s)
{
    for (int i = 0; i < s.size(); ++i)
        if (!s.at(i).isSpace())
            return false;
    return true;
}

static QCocoaWindow *toPlatformWindow(NSWindow *window)
{
    return qnswindow_cast(window).platformWindow;
}

@implementation QNSWindowDelegate

- (BOOL)windowShouldClose:(NSWindow *)window
{
    if (QCocoaWindow *platformWindow = toPlatformWindow(window))
        return platformWindow->windowShouldClose();

    return YES;
}
/*!
    Overridden to ensure that the zoomed state always results in a maximized
    window, which would otherwise not be the case for borderless windows.

    We also keep the window on the same screen as before; something AppKit
    sometimes fails to do using its built in logic.
*/
- (NSRect)windowWillUseStandardFrame:(NSWindow *)window defaultFrame:(NSRect)proposedFrame
{
    Q_UNUSED(proposedFrame);

    QCocoaWindow *platformWindow = toPlatformWindow(window);
    Q_ASSERT(platformWindow);
    const QWindow *w = platformWindow->window();

    // maximumSize() refers to the client size, but AppKit expects the full frame size
    QSizeF maximumSize = w->maximumSize() + QSize(0, w->frameMargins().top());

    // The window should never be larger than the current screen geometry
    const QRectF screenGeometry = platformWindow->screen()->geometry();
    maximumSize = maximumSize.boundedTo(screenGeometry.size());

    // Use the current frame position for the initial maximized frame,
    // so that the window stays put and just expand, in case its maximum
    // size is within the screen bounds.
    QRectF maximizedFrame = QRectF(w->framePosition(), maximumSize);

    // But constrain the frame to the screen bounds in case the frame
    // extends beyond the screen bounds as a result of starting out
    // with the current frame position.
    maximizedFrame.translate(QPoint(
        qMax(screenGeometry.left() - maximizedFrame.left(), 0.0) +
        qMin(screenGeometry.right() - maximizedFrame.right(), 0.0),
        qMax(screenGeometry.top() - maximizedFrame.top(), 0.0) +
        qMin(screenGeometry.bottom() - maximizedFrame.bottom(), 0.0)));

    return QCocoaScreen::mapToNative(maximizedFrame);
}

- (BOOL)windowShouldZoom:(NSWindow*)window toFrame:(NSRect)newFrame
{
    QCocoaWindow *platformWindow = toPlatformWindow(window);
    Q_ASSERT(platformWindow);
    platformWindow->windowWillZoom();
    return YES;
}

- (BOOL)window:(NSWindow *)window shouldPopUpDocumentPathMenu:(NSMenu *)menu
{
    Q_UNUSED(menu);

    QCocoaWindow *platformWindow = toPlatformWindow(window);
    Q_ASSERT(platformWindow);

    // Only pop up document path if the filename is non-empty. We allow whitespace, to
    // allow faking a window icon by setting the file path to a single space character.
    return !isWhiteSpace(platformWindow->window()->filePath());
}

- (BOOL)window:(NSWindow *)window shouldDragDocumentWithEvent:(NSEvent *)event from:(NSPoint)dragImageLocation withPasteboard:(NSPasteboard *)pasteboard
{
    Q_UNUSED(event);
    Q_UNUSED(dragImageLocation);
    Q_UNUSED(pasteboard);

    QCocoaWindow *platformWindow = toPlatformWindow(window);
    Q_ASSERT(platformWindow);

    // Only allow drag if the filename is non-empty. We allow whitespace, to
    // allow faking a window icon by setting the file path to a single space.
    return !isWhiteSpace(platformWindow->window()->filePath());
}
@end
