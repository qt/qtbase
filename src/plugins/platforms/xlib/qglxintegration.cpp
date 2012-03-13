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

#include "qxlibwindow.h"
#include "qxlibscreen.h"
#include "qxlibdisplay.h"

#if !defined(QT_NO_OPENGL) && !defined(QT_OPENGL_ES_2)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/glx.h>
#include "private/qglxconvenience_p.h"

#include "qglxintegration.h"

#if defined(Q_OS_LINUX) || defined(Q_OS_BSD4)
#include <dlfcn.h>
#endif

QT_BEGIN_NAMESPACE

QGLXContext::QGLXContext(QXlibScreen *screen, const QSurfaceFormat &format, QPlatformOpenGLContext *share)
    : QPlatformOpenGLContext()
    , m_screen(screen)
    , m_context(0)
    , m_windowFormat(format)
{
    GLXContext shareGlxContext = 0;
    if (share)
        shareGlxContext = static_cast<const QGLXContext*>(share)->glxContext();

    Display *xDisplay = screen->display()->nativeDisplay();

    GLXFBConfig config = qglx_findConfig(xDisplay,screen->xScreenNumber(),format);
    if (config) {
        m_context = glXCreateNewContext(xDisplay,config,GLX_RGBA_TYPE,shareGlxContext,TRUE);
        m_windowFormat = qglx_surfaceFormatFromGLXFBConfig(xDisplay,config,m_context);
    } else {
        XVisualInfo *visualInfo = qglx_findVisualInfo(xDisplay, screen->xScreenNumber(), &m_windowFormat);
        if (!visualInfo)
            qFatal("Could not initialize GLX");
        m_context = glXCreateContext(xDisplay, visualInfo, shareGlxContext, true);
        XFree(visualInfo);
    }

#ifdef MYX11_DEBUG
    qDebug() << "QGLXGLContext::create context" << m_context;
#endif
}

QGLXContext::~QGLXContext()
{
    if (m_context) {
        qDebug("Destroying GLX context 0x%p", m_context);
        glXDestroyContext(m_screen->display()->nativeDisplay(), m_context);
    }
}

QSurfaceFormat QGLXContext::format() const
{
    return m_windowFormat;
}

bool QGLXContext::makeCurrent(QPlatformSurface *surface)
{
    Q_UNUSED(surface);

    GLXDrawable glxDrawable = static_cast<QXlibWindow *>(surface)->winId();
#ifdef MYX11_DEBUG
    qDebug("QGLXGLContext::makeCurrent(window=0x%x, ctx=0x%x)", glxDrawable, m_context);
#endif
    return glXMakeCurrent(m_screen->display()->nativeDisplay(), glxDrawable, m_context);
}

void QGLXContext::doneCurrent()
{
    glXMakeCurrent(m_screen->display()->nativeDisplay(), 0, 0);
}

void QGLXContext::swapBuffers(QPlatformSurface *surface)
{
    Q_UNUSED(surface);

    GLXDrawable glxDrawable = static_cast<QXlibWindow *>(surface)->winId();
    glXSwapBuffers(m_screen->display()->nativeDisplay(), glxDrawable);
}

void (*QGLXContext::getProcAddress(const QByteArray& procName))()
{
    typedef void *(*qt_glXGetProcAddressARB)(const GLubyte *);
    static qt_glXGetProcAddressARB glXGetProcAddressARB = 0;
    static bool resolved = false;

    if (resolved && !glXGetProcAddressARB)
        return 0;
    if (!glXGetProcAddressARB) {
        QList<QByteArray> glxExt = QByteArray(glXGetClientString(m_screen->display()->nativeDisplay(), GLX_EXTENSIONS)).split(' ');
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

QSurfaceFormat QGLXContext::surfaceFormat() const
{
    return m_windowFormat;
}

QT_END_NAMESPACE

#endif //!defined(QT_NO_OPENGL) && !defined(QT_OPENGL_ES_2)
