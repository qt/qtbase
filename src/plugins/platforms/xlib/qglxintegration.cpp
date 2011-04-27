/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
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
#include <QGLFormat>

#include "qxlibwindow.h"
#include "qxlibscreen.h"
#include "qxlibdisplay.h"

#if !defined(QT_NO_OPENGL) && !defined(QT_OPENGL_ES_2)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/glx.h>
#include "qglxconvenience.h"

#include "qglxintegration.h"

#if defined(Q_OS_LINUX) || defined(Q_OS_BSD4)
#include <dlfcn.h>
#endif

QT_BEGIN_NAMESPACE

QGLXContext::QGLXContext(Window window, QXlibScreen *screen, const QPlatformWindowFormat &format)
    : QPlatformGLContext()
    , m_screen(screen)
    , m_drawable((Drawable)window)
    , m_context(0)
{

    const QPlatformGLContext *sharePlatformContext;
    sharePlatformContext = format.sharedGLContext();
    GLXContext shareGlxContext = 0;
    if (sharePlatformContext)
        shareGlxContext = static_cast<const QGLXContext*>(sharePlatformContext)->glxContext();

    GLXFBConfig config = qglx_findConfig(screen->display()->nativeDisplay(),screen->xScreenNumber(),format);
    m_context = glXCreateNewContext(screen->display()->nativeDisplay(),config,GLX_RGBA_TYPE,shareGlxContext,TRUE);
    m_windowFormat = qglx_platformWindowFromGLXFBConfig(screen->display()->nativeDisplay(),config,m_context);

#ifdef MYX11_DEBUG
    qDebug() << "QGLXGLContext::create context" << m_context;
#endif
}

QGLXContext::QGLXContext(QXlibScreen *screen, Drawable drawable, GLXContext context)
    : QPlatformGLContext(), m_screen(screen), m_drawable(drawable), m_context(context)
{

}

QGLXContext::~QGLXContext()
{
    if (m_context) {
        qDebug("Destroying GLX context 0x%p", m_context);
        glXDestroyContext(m_screen->display()->nativeDisplay(), m_context);
    }
}

void QGLXContext::makeCurrent()
{
    QPlatformGLContext::makeCurrent();
#ifdef MYX11_DEBUG
    qDebug("QGLXGLContext::makeCurrent(window=0x%x, ctx=0x%x)", m_drawable, m_context);
#endif
    glXMakeCurrent(m_screen->display()->nativeDisplay(), m_drawable, m_context);
}

void QGLXContext::doneCurrent()
{
    QPlatformGLContext::doneCurrent();
    glXMakeCurrent(m_screen->display()->nativeDisplay(), 0, 0);
}

void QGLXContext::swapBuffers()
{
    glXSwapBuffers(m_screen->display()->nativeDisplay(), m_drawable);
}

void* QGLXContext::getProcAddress(const QString& procName)
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
    return glXGetProcAddressARB(reinterpret_cast<const GLubyte *>(procName.toLatin1().data()));
}

QPlatformWindowFormat QGLXContext::platformWindowFormat() const
{
    return m_windowFormat;
}

QT_END_NAMESPACE

#endif //!defined(QT_NO_OPENGL) && !defined(QT_OPENGL_ES_2)
