/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "qcocoawindow.h"
#include "qnswindowdelegate.h"
#include "qcocoaautoreleasepool.h"
#include "qcocoaglcontext.h"
#include "qnsview.h"
#include <QtCore/private/qcore_mac_p.h>

#include <QWindowSystemInterface>

#include <QDebug>

QCocoaWindow::QCocoaWindow(QWindow *tlw)
    : QPlatformWindow(tlw)
    , m_glContext(0)
{
    QCocoaAutoReleasePool pool;
    const QRect geo = tlw->geometry();
    NSRect frame = NSMakeRect(geo.x(), geo.y(), geo.width(), geo.height());

    m_nsWindow  = [[NSWindow alloc] initWithContentRect:frame
                                            styleMask:NSTitledWindowMask|NSClosableWindowMask|NSMiniaturizableWindowMask|NSResizableWindowMask
                                            backing:NSBackingStoreBuffered
                                            defer:YES];

    QNSWindowDelegate *delegate = [[QNSWindowDelegate alloc] initWithQCocoaWindow:this];
    [m_nsWindow setDelegate:delegate];

    [m_nsWindow setAcceptsMouseMovedEvents:YES];

    m_contentView = [[QNSView alloc] initWithQWindow:tlw];

    if (tlw->surfaceType() == QWindow::OpenGLSurface) {
        NSRect glFrame = NSMakeRect(0, 0, geo.width(), geo.height());
        m_windowSurfaceView = [[NSOpenGLView alloc] initWithFrame : glFrame pixelFormat : QCocoaGLContext::createNSOpenGLPixelFormat() ];
        [m_contentView setAutoresizesSubviews : YES];
        [m_windowSurfaceView setAutoresizingMask : (NSViewWidthSizable | NSViewHeightSizable)];
        [m_contentView addSubview : m_windowSurfaceView];
    } else {
        m_windowSurfaceView = m_contentView;
    }

    [m_nsWindow setContentView:m_contentView];
}

QCocoaWindow::~QCocoaWindow()
{
}

void QCocoaWindow::setGeometry(const QRect &rect)
{
    QPlatformWindow::setGeometry(rect);

    NSRect bounds = NSMakeRect(rect.x(), rect.y(), rect.width(), rect.height());
    [[m_nsWindow contentView]setFrameSize:bounds.size];
}

void QCocoaWindow::setVisible(bool visible)
{
    if (visible) {
        [m_nsWindow makeKeyAndOrderFront:nil];
    } else {
        [m_nsWindow orderOut:nil];
    }
}

void QCocoaWindow::setWindowTitle(const QString &title)
{
    CFStringRef windowTitle = QCFString::toCFStringRef(title);
    [m_nsWindow setTitle: reinterpret_cast<const NSString *>(windowTitle)];
    CFRelease(windowTitle);
}

void QCocoaWindow::raise()
{
    // ### handle spaces (see Qt 4 raise_sys in qwidget_mac.mm)
    [m_nsWindow orderFront];
}

void QCocoaWindow::lower()
{
    [m_nsWindow orderBack];
}

WId QCocoaWindow::winId() const
{
    return WId(m_nsWindow);
}

NSView *QCocoaWindow::contentView() const
{
    return [m_nsWindow contentView];
}

void QCocoaWindow::windowDidResize()
{
    //jlind: XXX This isn't ideal. Eventdispatcher does not run when resizing...
    NSRect rect = [[m_nsWindow contentView]frame];
    QRect geo(rect.origin.x,rect.origin.y,rect.size.width,rect.size.height);
    QWindowSystemInterface::handleGeometryChange(window(),geo);
}

QPlatformGLContext *QCocoaWindow::glContext() const
{
    if (!m_glContext) {
        m_glContext = new QCocoaGLContext(m_windowSurfaceView);
    }
    return m_glContext;
}
