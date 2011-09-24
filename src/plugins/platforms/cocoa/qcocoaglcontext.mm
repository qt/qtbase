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

#include "qcocoaglcontext.h"
#include "qcocoawindow.h"
#include "qcocoaautoreleasepool.h"
#include <qdebug.h>
#include <QtCore/private/qcore_mac_p.h>
#include <QtPlatformSupport/private/cglconvenience_p.h>

#import <Cocoa/Cocoa.h>

QCocoaGLContext::QCocoaGLContext(const QSurfaceFormat &format, QPlatformOpenGLContext *share)
    : m_format(format)
{
    QCocoaAutoReleasePool pool; // For the SG Canvas render thread.

    NSOpenGLPixelFormat *pixelFormat = static_cast <NSOpenGLPixelFormat *>(qcgl_createNSOpenGLPixelFormat(format));
    NSOpenGLContext *actualShare = share ? static_cast<QCocoaGLContext *>(share)->m_context : 0;

    m_context = [NSOpenGLContext alloc];
    [m_context initWithFormat:pixelFormat shareContext:actualShare];

    const GLint interval = 1;
    [m_context setValues:&interval forParameter:NSOpenGLCPSwapInterval];

}

// Match up with createNSOpenGLPixelFormat!
QSurfaceFormat QCocoaGLContext::format() const
{
    return m_format;
}

void QCocoaGLContext::swapBuffers(QPlatformSurface *surface)
{
    QWindow *window = static_cast<QCocoaWindow *>(surface)->window();
    setActiveWindow(window);

    [m_context flushBuffer];
}

bool QCocoaGLContext::makeCurrent(QPlatformSurface *surface)
{
    QCocoaAutoReleasePool pool;

    QWindow *window = static_cast<QCocoaWindow *>(surface)->window();
    setActiveWindow(window);

    [m_context makeCurrentContext];
    return true;
}

void QCocoaGLContext::setActiveWindow(QWindow *window)
{
    if (window == m_currentWindow.data())
        return;

    if (m_currentWindow)
        static_cast<QCocoaWindow *>(m_currentWindow.data()->handle())->setCurrentContext(0);

    Q_ASSERT(window->handle());

    m_currentWindow = window;

    QCocoaWindow *cocoaWindow = static_cast<QCocoaWindow *>(window->handle());
    cocoaWindow->setCurrentContext(this);

    NSView *view = cocoaWindow->contentView();
    [m_context setView:view];
}

void QCocoaGLContext::doneCurrent()
{
    if (m_currentWindow)
        static_cast<QCocoaWindow *>(m_currentWindow.data()->handle())->setCurrentContext(0);

    m_currentWindow.clear();

    [NSOpenGLContext clearCurrentContext];
}

void (*QCocoaGLContext::getProcAddress(const QByteArray &procName))()
{
    return qcgl_getProcAddress(procName);
}

void QCocoaGLContext::update()
{
    [m_context update];
}

NSOpenGLPixelFormat *QCocoaGLContext::createNSOpenGLPixelFormat(const QSurfaceFormat &format)
{
    return static_cast<NSOpenGLPixelFormat *>(qcgl_createNSOpenGLPixelFormat(format));
}

NSOpenGLContext *QCocoaGLContext::nsOpenGLContext() const
{
    return m_context;
}

