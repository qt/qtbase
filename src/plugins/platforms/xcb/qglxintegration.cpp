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

typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);

#ifndef GLX_CONTEXT_CORE_PROFILE_BIT_ARB
#define GLX_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001
#endif

#ifndef GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB
#define GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002
#endif

#ifndef GLX_CONTEXT_PROFILE_MASK_ARB
#define GLX_CONTEXT_PROFILE_MASK_ARB 0x9126
#endif

#ifndef GL_CONTEXT_FLAG_DEBUG_BIT
#define GL_CONTEXT_FLAG_DEBUG_BIT 0x00000002
#endif

static Window createDummyWindow(QXcbScreen *screen, XVisualInfo *visualInfo)
{
    Colormap cmap = XCreateColormap(DISPLAY_FROM_XCB(screen), screen->root(), visualInfo->visual, AllocNone);
    XSetWindowAttributes a;
    a.background_pixel = WhitePixel(DISPLAY_FROM_XCB(screen), screen->screenNumber());
    a.border_pixel = BlackPixel(DISPLAY_FROM_XCB(screen), screen->screenNumber());
    a.colormap = cmap;

    Window window = XCreateWindow(DISPLAY_FROM_XCB(screen), screen->root(),
                                  0, 0, 100, 100,
                                  0, visualInfo->depth, InputOutput, visualInfo->visual,
                                  CWBackPixel|CWBorderPixel|CWColormap, &a);
    XFreeColormap(DISPLAY_FROM_XCB(screen), cmap);
    return window;
}

static Window createDummyWindow(QXcbScreen *screen, GLXFBConfig config)
{
    XVisualInfo *visualInfo = glXGetVisualFromFBConfig(DISPLAY_FROM_XCB(screen), config);
    if (!visualInfo)
        qFatal("Could not initialize GLX");
    Window window = createDummyWindow(screen, visualInfo);
    XFree(visualInfo);
    return window;
}

static inline QByteArray getGlString(GLenum param)
{
    if (const GLubyte *s = glGetString(param))
        return QByteArray(reinterpret_cast<const char*>(s));
    return QByteArray();
}

static void updateFormatFromContext(QSurfaceFormat &format)
{
    // Update the version, profile, and context bit of the format
    int major = 0, minor = 0;
    QByteArray versionString(getGlString(GL_VERSION));
    if (QPlatformOpenGLContext::parseOpenGLVersion(versionString, major, minor)) {
        format.setMajorVersion(major);
        format.setMinorVersion(minor);
    }

    format.setProfile(QSurfaceFormat::NoProfile);

    const int version = (major << 8) + minor;
    if (version < 0x0300) {
        format.setProfile(QSurfaceFormat::NoProfile);
        format.setOption(QSurfaceFormat::DeprecatedFunctions);
        return;
    }

    // Version 3.0 onwards - check if it includes deprecated functionality or is
    // a debug context
    GLint value = 0;
    glGetIntegerv(GL_CONTEXT_FLAGS, &value);
    if (!(value & GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT))
        format.setOption(QSurfaceFormat::DeprecatedFunctions);
    if (value & GL_CONTEXT_FLAG_DEBUG_BIT)
        format.setOption(QSurfaceFormat::DebugContext);
    if (version < 0x0302)
        return;

    // Version 3.2 and newer have a profile
    value = 0;
    glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &value);

    if (value & GL_CONTEXT_CORE_PROFILE_BIT)
        format.setProfile(QSurfaceFormat::CoreProfile);
    else if (value & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT)
        format.setProfile(QSurfaceFormat::CompatibilityProfile);
}

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
    XVisualInfo *visualInfo = 0;
    Window window = 0; // Temporary window used to query OpenGL context

    if (config) {
        // Resolve entry point for glXCreateContextAttribsARB
        glXCreateContextAttribsARBProc glXCreateContextAttribsARB = 0;
        glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc) glXGetProcAddress((const GLubyte*)"glXCreateContextAttribsARB");

        QList<QByteArray> glxExt = QByteArray(glXQueryExtensionsString(DISPLAY_FROM_XCB(m_screen), m_screen->screenNumber())).split(' ');
        bool supportsProfiles = glxExt.contains("GLX_ARB_create_context_profile");

        // Use glXCreateContextAttribsARB if available
        if (glxExt.contains("GLX_ARB_create_context") && glXCreateContextAttribsARB != 0) {
            // Try to create an OpenGL context for each known OpenGL version in descending
            // order from the requested version.
            const int requestedVersion = format.majorVersion() * 10 + qMin(format.minorVersion(), 9);

            QVector<int> glVersions;
            if (requestedVersion > 43)
                glVersions << requestedVersion;

            // Don't bother with versions below 2.0
            glVersions << 43 << 42 << 41 << 40 << 33 << 32 << 31 << 30 << 21 << 20;

            for (int i = 0; !m_context && i < glVersions.count(); i++) {
                const int version = glVersions[i];
                if (version > requestedVersion)
                    continue;

                const int majorVersion = version / 10;
                const int minorVersion = version % 10;

                QVector<int> contextAttributes;
                contextAttributes << GLX_CONTEXT_MAJOR_VERSION_ARB << majorVersion
                                  << GLX_CONTEXT_MINOR_VERSION_ARB << minorVersion;

                // If asking for OpenGL 3.2 or newer we should also specify a profile
                if (version >= 32 && supportsProfiles) {
                    if (m_format.profile() == QSurfaceFormat::CoreProfile)
                        contextAttributes << GLX_CONTEXT_PROFILE_MASK_ARB << GLX_CONTEXT_CORE_PROFILE_BIT_ARB;
                    else
                        contextAttributes << GLX_CONTEXT_PROFILE_MASK_ARB << GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB;
                }

                int flags = 0;

                if (m_format.testOption(QSurfaceFormat::DebugContext))
                    flags |= GLX_CONTEXT_DEBUG_BIT_ARB;

                // A forward-compatible context may be requested for 3.0 and later
                if (version >= 30 && !m_format.testOption(QSurfaceFormat::DeprecatedFunctions))
                    flags |= GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;

                if (flags != 0)
                    contextAttributes << GLX_CONTEXT_FLAGS_ARB << flags;

                contextAttributes << None;

                m_context = glXCreateContextAttribsARB(DISPLAY_FROM_XCB(screen), config, m_shareContext, true, contextAttributes.data());
                if (!m_context && m_shareContext) {
                    // re-try without a shared glx context
                    m_context = glXCreateContextAttribsARB(DISPLAY_FROM_XCB(screen), config, 0, true, contextAttributes.data());
                    if (m_context)
                        m_shareContext = 0;
                }
            }
        }

        // Could not create a context using glXCreateContextAttribsARB, falling back to glXCreateNewContext.
        if (!m_context) {
            m_context = glXCreateNewContext(DISPLAY_FROM_XCB(screen), config, GLX_RGBA_TYPE, m_shareContext, true);
            if (!m_context && m_shareContext) {
                // re-try without a shared glx context
                m_context = glXCreateNewContext(DISPLAY_FROM_XCB(screen), config, GLX_RGBA_TYPE, 0, true);
                if (m_context)
                    m_shareContext = 0;
            }
        }

        // Get the basic surface format details
        if (m_context)
            m_format = qglx_surfaceFormatFromGLXFBConfig(DISPLAY_FROM_XCB(screen), config, m_context);

        // Create a temporary window so that we can make the new context current
        window = createDummyWindow(screen, config);
    } else {
        // Note that m_format gets updated with the used surface format
        visualInfo = qglx_findVisualInfo(DISPLAY_FROM_XCB(screen), screen->screenNumber(), &m_format);
        if (!visualInfo)
            qFatal("Could not initialize GLX");
        m_context = glXCreateContext(DISPLAY_FROM_XCB(screen), visualInfo, m_shareContext, true);
        if (!m_context && m_shareContext) {
            // re-try without a shared glx context
            m_shareContext = 0;
            m_context = glXCreateContext(DISPLAY_FROM_XCB(screen), visualInfo, 0, true);
        }

        // Create a temporary window so that we can make the new context current
        window = createDummyWindow(screen, visualInfo);
        XFree(visualInfo);
    }

    // Query the OpenGL version and profile
    if (m_context && window) {
        glXMakeCurrent(DISPLAY_FROM_XCB(screen), window, m_context);
        updateFormatFromContext(m_format);

        // Make our context non-current
        glXMakeCurrent(DISPLAY_FROM_XCB(screen), 0, 0);
    }

    // Destroy our temporary window
    XDestroyWindow(DISPLAY_FROM_XCB(screen), window);
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
