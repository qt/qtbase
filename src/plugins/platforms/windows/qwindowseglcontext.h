/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QWINDOWSEGLCONTEXT_H
#define QWINDOWSEGLCONTEXT_H

#include "qwindowsopenglcontext.h"
#include "qwindowsopengltester.h"
#include <EGL/egl.h>

QT_BEGIN_NAMESPACE

struct QWindowsLibEGL
{
    bool init();

    EGLint (EGLAPIENTRY * eglGetError)(void);
    EGLDisplay (EGLAPIENTRY * eglGetDisplay)(EGLNativeDisplayType display_id);
    EGLBoolean (EGLAPIENTRY * eglInitialize)(EGLDisplay dpy, EGLint *major, EGLint *minor);
    EGLBoolean (EGLAPIENTRY * eglTerminate)(EGLDisplay dpy);
    EGLBoolean (EGLAPIENTRY * eglChooseConfig)(EGLDisplay dpy, const EGLint *attrib_list,
                                               EGLConfig *configs, EGLint config_size,
                                               EGLint *num_config);
    EGLBoolean (EGLAPIENTRY * eglGetConfigAttrib)(EGLDisplay dpy, EGLConfig config,
                                                  EGLint attribute, EGLint *value);
    EGLSurface (EGLAPIENTRY * eglCreateWindowSurface)(EGLDisplay dpy, EGLConfig config,
                                                      EGLNativeWindowType win,
                                                      const EGLint *attrib_list);
    EGLSurface (EGLAPIENTRY * eglCreatePbufferSurface)(EGLDisplay dpy, EGLConfig config,
                                                       const EGLint *attrib_list);
    EGLBoolean (EGLAPIENTRY * eglDestroySurface)(EGLDisplay dpy, EGLSurface surface);
    EGLBoolean (EGLAPIENTRY * eglBindAPI)(EGLenum api);
    EGLBoolean (EGLAPIENTRY * eglSwapInterval)(EGLDisplay dpy, EGLint interval);
    EGLContext (EGLAPIENTRY * eglCreateContext)(EGLDisplay dpy, EGLConfig config,
                                                EGLContext share_context,
                                                const EGLint *attrib_list);
    EGLBoolean (EGLAPIENTRY * eglDestroyContext)(EGLDisplay dpy, EGLContext ctx);
    EGLBoolean (EGLAPIENTRY * eglMakeCurrent)(EGLDisplay dpy, EGLSurface draw,
                                              EGLSurface read, EGLContext ctx);
    EGLContext (EGLAPIENTRY * eglGetCurrentContext)(void);
    EGLSurface (EGLAPIENTRY * eglGetCurrentSurface)(EGLint readdraw);
    EGLDisplay (EGLAPIENTRY * eglGetCurrentDisplay)(void);
    EGLBoolean (EGLAPIENTRY * eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
    QFunctionPointer (EGLAPIENTRY *eglGetProcAddress)(const char *procname);

    EGLDisplay (EGLAPIENTRY * eglGetPlatformDisplayEXT)(EGLenum platform, void *native_display, const EGLint *attrib_list);

private:
#if !defined(QT_STATIC) || defined(QT_OPENGL_DYNAMIC)
    void *resolve(const char *name);
    HMODULE m_lib;
#endif
};

struct QWindowsLibGLESv2
{
    bool init();

#if !defined(QT_STATIC) || defined(QT_OPENGL_DYNAMIC)
    void *moduleHandle() const { return m_lib; }
#else
    void *moduleHandle() const { return Q_NULLPTR; }
#endif

    const GLubyte * (APIENTRY * glGetString)(GLenum name);

#if !defined(QT_STATIC) || defined(QT_OPENGL_DYNAMIC)
    void *resolve(const char *name);
private:
    HMODULE m_lib;
#endif
};

class QWindowsEGLStaticContext : public QWindowsStaticOpenGLContext
{
    Q_DISABLE_COPY(QWindowsEGLStaticContext)

public:
    static QWindowsEGLStaticContext *create(QWindowsOpenGLTester::Renderers preferredType);
    ~QWindowsEGLStaticContext();

    EGLDisplay display() const { return m_display; }

    QWindowsOpenGLContext *createContext(QOpenGLContext *context) Q_DECL_OVERRIDE;
    void *moduleHandle() const Q_DECL_OVERRIDE { return libGLESv2.moduleHandle(); }
    QOpenGLContext::OpenGLModuleType moduleType() const Q_DECL_OVERRIDE { return QOpenGLContext::LibGLES; }

    void *createWindowSurface(void *nativeWindow, void *nativeConfig, int *err) Q_DECL_OVERRIDE;
    void destroyWindowSurface(void *nativeSurface) Q_DECL_OVERRIDE;

    QSurfaceFormat formatFromConfig(EGLDisplay display, EGLConfig config, const QSurfaceFormat &referenceFormat);

    static QWindowsLibEGL libEGL;
    static QWindowsLibGLESv2 libGLESv2;

private:
    explicit QWindowsEGLStaticContext(EGLDisplay display);
    static bool initializeAngle(QWindowsOpenGLTester::Renderers preferredType, HDC dc,
                                EGLDisplay *display, EGLint *major, EGLint *minor);

    const EGLDisplay m_display;
};

class QWindowsEGLContext : public QWindowsOpenGLContext
{
public:
    QWindowsEGLContext(QWindowsEGLStaticContext *staticContext,
                       const QSurfaceFormat &format,
                       QPlatformOpenGLContext *share);
    ~QWindowsEGLContext();

    bool makeCurrent(QPlatformSurface *surface) Q_DECL_OVERRIDE;
    void doneCurrent() Q_DECL_OVERRIDE;
    void swapBuffers(QPlatformSurface *surface) Q_DECL_OVERRIDE;
    QFunctionPointer getProcAddress(const char *procName) Q_DECL_OVERRIDE;

    QSurfaceFormat format() const Q_DECL_OVERRIDE { return m_format; }
    bool isSharing() const Q_DECL_OVERRIDE { return m_shareContext != EGL_NO_CONTEXT; }
    bool isValid() const Q_DECL_OVERRIDE { return m_eglContext != EGL_NO_CONTEXT; }

    void *nativeContext() const Q_DECL_OVERRIDE { return m_eglContext; }
    void *nativeDisplay() const Q_DECL_OVERRIDE { return m_eglDisplay; }
    void *nativeConfig() const Q_DECL_OVERRIDE { return m_eglConfig; }

private:
    EGLConfig chooseConfig(const QSurfaceFormat &format);

    QWindowsEGLStaticContext *m_staticContext;
    EGLContext m_eglContext;
    EGLContext m_shareContext;
    EGLDisplay m_eglDisplay;
    EGLConfig m_eglConfig;
    QSurfaceFormat m_format;
    EGLenum m_api;
    int m_swapInterval;
};

QT_END_NAMESPACE

#endif // QWINDOWSEGLCONTEXT_H
