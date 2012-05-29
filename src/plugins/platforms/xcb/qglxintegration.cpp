/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QDebug>
#include <QLibrary>

#include "qxcbwindow.h"
#include "qxcbscreen.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/glx.h>

#include <QtGui/QOpenGLContext>

#include "qglxintegration.h"
#include <QtPlatformSupport/private/qglxconvenience_p.h>

#if defined(Q_OS_LINUX) || defined(Q_OS_BSD4)
#include <dlfcn.h>
#endif

QT_BEGIN_NAMESPACE

QGLXContext::QGLXContext(QXcbScreen *screen, const QSurfaceFormat &format, QPlatformOpenGLContext *share)
    : QPlatformOpenGLContext()
    , m_screen(screen)
    , m_context(0)
    , m_format(format)
{
    m_shareContext = 0;
    if (share)
        m_shareContext = static_cast<const QGLXContext*>(share)->glxContext();

    GLXFBConfig config = qglx_findConfig(DISPLAY_FROM_XCB(screen),screen->screenNumber(),format);

    if (config) {
        m_context = glXCreateNewContext(DISPLAY_FROM_XCB(screen), config, GLX_RGBA_TYPE, m_shareContext, TRUE);
        if (!m_context && m_shareContext) {
            // re-try without a shared glx context
            m_shareContext = 0;
            m_context = glXCreateNewContext(DISPLAY_FROM_XCB(screen), config, GLX_RGBA_TYPE, 0, TRUE);
        }

        if (m_context)
            m_format = qglx_surfaceFormatFromGLXFBConfig(DISPLAY_FROM_XCB(screen), config, m_context);
    } else {
        XVisualInfo *visualInfo = qglx_findVisualInfo(DISPLAY_FROM_XCB(m_screen), screen->screenNumber(), &m_format);
        if (!visualInfo)
            qFatal("Could not initialize GLX");
        m_context = glXCreateContext(DISPLAY_FROM_XCB(screen), visualInfo, m_shareContext, true);
        if (!m_context && m_shareContext) {
            // re-try without a shared glx context
            m_shareContext = 0;
            m_context = glXCreateContext(DISPLAY_FROM_XCB(screen), visualInfo, 0, true);
        }
        XFree(visualInfo);
    }
}

QGLXContext::~QGLXContext()
{
    glXDestroyContext(DISPLAY_FROM_XCB(m_screen), m_context);
}

bool QGLXContext::makeCurrent(QPlatformSurface *surface)
{
    Q_ASSERT(surface->surface()->surfaceType() == QSurface::OpenGLSurface);

    GLXDrawable glxDrawable = static_cast<QXcbWindow *>(surface)->xcb_window();

    return glXMakeCurrent(DISPLAY_FROM_XCB(m_screen), glxDrawable, m_context);
}

void QGLXContext::doneCurrent()
{
    glXMakeCurrent(DISPLAY_FROM_XCB(m_screen), 0, 0);
}

void QGLXContext::swapBuffers(QPlatformSurface *surface)
{
    GLXDrawable glxDrawable = static_cast<QXcbWindow *>(surface)->xcb_window();
    glXSwapBuffers(DISPLAY_FROM_XCB(m_screen), glxDrawable);
}

void (*QGLXContext::getProcAddress(const QByteArray &procName)) ()
{
    typedef void *(*qt_glXGetProcAddressARB)(const GLubyte *);
    static qt_glXGetProcAddressARB glXGetProcAddressARB = 0;
    static bool resolved = false;

    if (resolved && !glXGetProcAddressARB)
        return 0;
    if (!glXGetProcAddressARB) {
        QList<QByteArray> glxExt = QByteArray(glXGetClientString(DISPLAY_FROM_XCB(m_screen), GLX_EXTENSIONS)).split(' ');
        if (glxExt.contains("GLX_ARB_get_proc_address")) {
#if defined(Q_OS_LINUX) || defined(Q_OS_BSD4)
            void *handle = dlopen(NULL, RTLD_LAZY);
            if (handle) {
                glXGetProcAddressARB = (qt_glXGetProcAddressARB) dlsym(handle, "glXGetProcAddressARB");
                dlclose(handle);
            }
            if (!glXGetProcAddressARB)
#endif
            {
                extern const QString qt_gl_library_name();
//                QLibrary lib(qt_gl_library_name());
                QLibrary lib(QLatin1String("GL"));
                glXGetProcAddressARB = (qt_glXGetProcAddressARB) lib.resolve("glXGetProcAddressARB");
            }
        }
        resolved = true;
    }
    if (!glXGetProcAddressARB)
        return 0;
    return (void (*)())glXGetProcAddressARB(reinterpret_cast<const GLubyte *>(procName.constData()));
}

QSurfaceFormat QGLXContext::format() const
{
    return m_format;
}

bool QGLXContext::isSharing() const
{
    return m_shareContext != 0;
}

bool QGLXContext::isValid() const
{
    return m_context != 0;
}

QT_END_NAMESPACE
