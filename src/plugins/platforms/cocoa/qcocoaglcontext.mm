/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
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
    : m_context(nil),
      m_shareContext(nil),
      m_format(format)
{
    // we only support OpenGL contexts under Cocoa
    if (m_format.renderableType() == QSurfaceFormat::DefaultRenderableType)
        m_format.setRenderableType(QSurfaceFormat::OpenGL);
    if (m_format.renderableType() != QSurfaceFormat::OpenGL)
        return;

    QCocoaAutoReleasePool pool; // For the SG Canvas render thread

    NSOpenGLPixelFormat *pixelFormat = static_cast <NSOpenGLPixelFormat *>(qcgl_createNSOpenGLPixelFormat(m_format));
    m_shareContext = share ? static_cast<QCocoaGLContext *>(share)->nsOpenGLContext() : nil;

    m_context = [NSOpenGLContext alloc];
    [m_context initWithFormat:pixelFormat shareContext:m_shareContext];

    if (!m_context && m_shareContext) {
        // try without shared context
        m_shareContext = nil;
        [m_context initWithFormat:pixelFormat shareContext:nil];
    }

    [pixelFormat release];

    const GLint interval = 1;
    [m_context setValues:&interval forParameter:NSOpenGLCPSwapInterval];

    if (format.alphaBufferSize() > 0) {
        int zeroOpacity = 0;
        [m_context setValues:&zeroOpacity forParameter:NSOpenGLCPSurfaceOpacity];
    }
}

QCocoaGLContext::~QCocoaGLContext()
{
    [m_context release];
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
    Q_ASSERT(surface->surface()->surfaceType() == QSurface::OpenGLSurface);

    QCocoaAutoReleasePool pool;

    QWindow *window = static_cast<QCocoaWindow *>(surface)->window();
    setActiveWindow(window);

    [m_context makeCurrentContext];
    update();
    return true;
}

void QCocoaGLContext::setActiveWindow(QWindow *window)
{
    if (window == m_currentWindow.data())
        return;

    if (m_currentWindow && m_currentWindow.data()->handle())
        static_cast<QCocoaWindow *>(m_currentWindow.data()->handle())->setCurrentContext(0);

    Q_ASSERT(window->handle());

    m_currentWindow = window;

    QCocoaWindow *cocoaWindow = static_cast<QCocoaWindow *>(window->handle());
    cocoaWindow->setCurrentContext(this);

    [(QNSView *) cocoaWindow->contentView() setQCocoaGLContext:this];
}

void QCocoaGLContext::doneCurrent()
{
    if (m_currentWindow && m_currentWindow.data()->handle())
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

bool QCocoaGLContext::isValid() const
{
    return m_context != nil;
}

bool QCocoaGLContext::isSharing() const
{
    return m_shareContext != nil;
}
