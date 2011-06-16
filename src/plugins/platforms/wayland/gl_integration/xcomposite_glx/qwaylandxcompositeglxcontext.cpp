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

#include <QtCore/QDebug>

#include "qwaylandxcompositeglxcontext.h"

#include "qwaylandxcompositeglxwindow.h"

#include <QRegion>

QWaylandXCompositeGLXSurface::QWaylandXCompositeGLXSurface(QWaylandXCompositeGLXWindow *window)
    : m_window(window)
{
}

Window QWaylandXCompositeGLXSurface::xWindow() const
{
    return m_window->xWindow();
}

QWaylandXCompositeGLXContext::QWaylandXCompositeGLXContext(const QGuiGLFormat &format, QPlatformGLContext *share, Display *display, int screen)
    : m_display(display)
{
    qDebug("creating XComposite-GLX context");
    GLXContext shareContext = share ? static_cast<QWaylandXCompositeGLXContext *>(share)->m_context : 0;
    GLXFBConfig config = qglx_findConfig(display, screen, format);
    XVisualInfo *visualInfo = glXGetVisualFromFBConfig(display, config);
    m_context = glXCreateContext(display, visualInfo, shareContext, true);
    m_format = qglx_guiGLFormatFromGLXFBConfig(display, config, m_context);
}

bool QWaylandXCompositeGLXContext::makeCurrent(const QPlatformGLSurface &surface)
{
    Window xWindow = static_cast<const QWaylandXCompositeGLXSurface &>(surface).xWindow();

    return glXMakeCurrent(m_display, xWindow, m_context);
}

void QWaylandXCompositeGLXContext::doneCurrent()
{
    glXMakeCurrent(m_display, 0, 0);
}

void QWaylandXCompositeGLXContext::swapBuffers(const QPlatformGLSurface &surface)
{
    const QWaylandXCompositeGLXSurface &s =
        static_cast<const QWaylandXCompositeGLXSurface &>(surface);

    QSize size = s.window()->geometry().size();

    glXSwapBuffers(m_display, s.xWindow());

    s.window()->damage(QRect(QPoint(), size));
    s.window()->waitForFrameSync();
}

void (*QWaylandXCompositeGLXContext::getProcAddress(const QByteArray &procName)) ()
{
    return glXGetProcAddress(reinterpret_cast<const GLubyte *>(procName.constData()));
}

QGuiGLFormat QWaylandXCompositeGLXContext::format() const
{
    return m_format;
}

