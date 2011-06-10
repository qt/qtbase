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

#include <QDebug>
#include <QLibrary>
#include <QGLFormat>

#include "qxcbwindow.h"
#include "qxcbscreen.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/glx.h>

#include <QtGui/QGuiGLContext>

#include "qglxintegration.h"
#include <QtPlatformSupport/private/qglxconvenience_p.h>

#if defined(Q_OS_LINUX) || defined(Q_OS_BSD4)
#include <dlfcn.h>
#endif

QGLXSurface::QGLXSurface(GLXDrawable drawable, const QGuiGLFormat &format)
    : QPlatformGLSurface(format)
    , glxDrawable(drawable)
{
}

QGLXContext::QGLXContext(QXcbScreen *screen, const QGuiGLFormat &format, QPlatformGLContext *share)
    : QPlatformGLContext()
    , m_screen(screen)
    , m_context(0)
{
    Q_XCB_NOOP(m_screen->connection());
    GLXContext shareGlxContext = 0;
    if (share)
        shareGlxContext = static_cast<const QGLXContext*>(share)->glxContext();

    GLXFBConfig config = qglx_findConfig(DISPLAY_FROM_XCB(screen),screen->screenNumber(),format);
    m_context = glXCreateNewContext(DISPLAY_FROM_XCB(screen), config, GLX_RGBA_TYPE, shareGlxContext, TRUE);
    m_format = qglx_guiGLFormatFromGLXFBConfig(DISPLAY_FROM_XCB(screen), config, m_context);
    Q_XCB_NOOP(m_screen->connection());
}

QGLXContext::~QGLXContext()
{
    Q_XCB_NOOP(m_screen->connection());
    glXDestroyContext(DISPLAY_FROM_XCB(m_screen), m_context);
    Q_XCB_NOOP(m_screen->connection());
}

bool QGLXContext::makeCurrent(const QPlatformGLSurface &surface)
{
    Q_XCB_NOOP(m_screen->connection());

    GLXDrawable glxSurface = static_cast<const QGLXSurface &>(surface).glxDrawable;

    bool result = glXMakeCurrent(DISPLAY_FROM_XCB(m_screen), glxSurface, m_context);

    Q_XCB_NOOP(m_screen->connection());
    return result;
}

void QGLXContext::doneCurrent()
{
    glXMakeCurrent(DISPLAY_FROM_XCB(m_screen), 0, 0);
}

void QGLXContext::swapBuffers(const QPlatformGLSurface &drawable)
{
    Q_XCB_NOOP(m_screen->connection());
    GLXDrawable glxDrawable = static_cast<const QGLXSurface &>(drawable).glxDrawable;
    glXSwapBuffers(DISPLAY_FROM_XCB(m_screen), glxDrawable);
    Q_XCB_NOOP(m_screen->connection());
}

void (*QGLXContext::getProcAddress(const QByteArray &procName)) ()
{
    Q_XCB_NOOP(m_screen->connection());
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

QGuiGLFormat QGLXContext::format() const
{
    return m_format;
}
