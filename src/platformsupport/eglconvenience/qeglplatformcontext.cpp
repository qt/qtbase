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

#include "qeglplatformcontext_p.h"

#include "qeglconvenience_p.h"

#include <qpa/qplatformwindow.h>

#include <EGL/egl.h>

static inline void bindApi(const QSurfaceFormat &format)
{
    if (format.renderableType() == QSurfaceFormat::OpenVG)
        eglBindAPI(EGL_OPENVG_API);
#ifdef EGL_VERSION_1_4
    else if (format.renderableType() == QSurfaceFormat::OpenGL)
        eglBindAPI(EGL_OPENGL_API);
#endif
    else
        eglBindAPI(EGL_OPENGL_ES_API);
}

QEGLPlatformContext::QEGLPlatformContext(const QSurfaceFormat &format, QPlatformOpenGLContext *share, EGLDisplay display,
                                         EGLenum eglApi)
    : m_eglDisplay(display)
    , m_eglApi(eglApi)
    , m_eglConfig(q_configFromGLFormat(display, format))
{
    init(format, share);
}

QEGLPlatformContext::QEGLPlatformContext(const QSurfaceFormat &format, QPlatformOpenGLContext *share, EGLDisplay display,
                                         EGLConfig config, EGLenum eglApi)
    : m_eglDisplay(display)
    , m_eglApi(eglApi)
    , m_eglConfig(config)
{
    init(format, share);
}

void QEGLPlatformContext::init(const QSurfaceFormat &format, QPlatformOpenGLContext *share)
{
    m_format = q_glFormatFromConfig(m_eglDisplay, m_eglConfig);
    m_shareContext = share ? static_cast<QEGLPlatformContext *>(share)->m_eglContext : 0;

    QVector<EGLint> contextAttrs;
    contextAttrs.append(EGL_CONTEXT_CLIENT_VERSION);
    contextAttrs.append(format.majorVersion());
    contextAttrs.append(EGL_NONE);

    bindApi(m_format);
    m_eglContext = eglCreateContext(m_eglDisplay, m_eglConfig, m_shareContext, contextAttrs.constData());
    if (m_eglContext == EGL_NO_CONTEXT && m_shareContext != EGL_NO_CONTEXT) {
        m_shareContext = 0;
        m_eglContext = eglCreateContext(m_eglDisplay, m_eglConfig, 0, contextAttrs.constData());
    }
}

bool QEGLPlatformContext::makeCurrent(QPlatformSurface *surface)
{
    Q_ASSERT(surface->surface()->surfaceType() == QSurface::OpenGLSurface);

#ifdef QEGL_EXTRA_DEBUG
    qWarning("QEglContext::makeCurrent: %p\n",this);
#endif
    bindApi(m_format);

    EGLSurface eglSurface = eglSurfaceForPlatformSurface(surface);

    bool ok = eglMakeCurrent(m_eglDisplay, eglSurface, eglSurface, m_eglContext);
    if (!ok)
        qWarning("QEGLPlatformContext::makeCurrent: eglError: %x, this: %p \n", eglGetError(), this);
#ifdef QEGL_EXTRA_DEBUG
    static bool showDebug = true;
    if (showDebug) {
        showDebug = false;
        const char *str = (const char*)glGetString(GL_VENDOR);
        qWarning("Vendor %s\n", str);
        str = (const char*)glGetString(GL_RENDERER);
        qWarning("Renderer %s\n", str);
        str = (const char*)glGetString(GL_VERSION);
        qWarning("Version %s\n", str);

        str = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
        qWarning("Extensions %s\n",str);

        str = (const char*)glGetString(GL_EXTENSIONS);
        qWarning("Extensions %s\n", str);

    }
#endif
    return ok;
}

QEGLPlatformContext::~QEGLPlatformContext()
{
#ifdef QEGL_EXTRA_DEBUG
    qWarning("QEglContext::~QEglContext(): %p\n",this);
#endif
    if (m_eglContext != EGL_NO_CONTEXT) {
        eglDestroyContext(m_eglDisplay, m_eglContext);
        m_eglContext = EGL_NO_CONTEXT;
    }
}

void QEGLPlatformContext::doneCurrent()
{
#ifdef QEGL_EXTRA_DEBUG
    qWarning("QEglContext::doneCurrent:%p\n",this);
#endif
    bindApi(m_format);
    bool ok = eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (!ok)
        qWarning("QEGLPlatformContext::doneCurrent(): eglError: %d, this: %p \n", eglGetError(), this);
}

void QEGLPlatformContext::swapBuffers(QPlatformSurface *surface)
{
#ifdef QEGL_EXTRA_DEBUG
    qWarning("QEglContext::swapBuffers:%p\n",this);
#endif
    bindApi(m_format);
    EGLSurface eglSurface = eglSurfaceForPlatformSurface(surface);
    bool ok = eglSwapBuffers(m_eglDisplay, eglSurface);
    if (!ok)
        qWarning("QEGLPlatformContext::swapBuffers(): eglError: %d, this: %p \n", eglGetError(), this);
}

void (*QEGLPlatformContext::getProcAddress(const QByteArray &procName)) ()
{
#ifdef QEGL_EXTRA_DEBUG
    qWarning("QEglContext::getProcAddress%p\n",this);
#endif
    bindApi(m_format);
    return eglGetProcAddress(procName.constData());
}

QSurfaceFormat QEGLPlatformContext::format() const
{
    return m_format;
}

EGLContext QEGLPlatformContext::eglContext() const
{
    return m_eglContext;
}

EGLDisplay QEGLPlatformContext::eglDisplay() const
{
    return m_eglDisplay;
}

EGLConfig QEGLPlatformContext::eglConfig() const
{
    return m_eglConfig;
}
