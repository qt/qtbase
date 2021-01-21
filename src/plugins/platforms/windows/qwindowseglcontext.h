/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
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
    void *moduleHandle() const { return nullptr; }
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
    Q_DISABLE_COPY_MOVE(QWindowsEGLStaticContext)

public:
    static QWindowsEGLStaticContext *create(QWindowsOpenGLTester::Renderers preferredType);
    ~QWindowsEGLStaticContext() override;

    EGLDisplay display() const { return m_display; }

    QWindowsOpenGLContext *createContext(QOpenGLContext *context) override;
    void *moduleHandle() const override { return libGLESv2.moduleHandle(); }
    QOpenGLContext::OpenGLModuleType moduleType() const override { return QOpenGLContext::LibGLES; }

    void *createWindowSurface(void *nativeWindow, void *nativeConfig, int *err) override;
    void destroyWindowSurface(void *nativeSurface) override;

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
    ~QWindowsEGLContext() override;

    bool makeCurrent(QPlatformSurface *surface) override;
    void doneCurrent() override;
    void swapBuffers(QPlatformSurface *surface) override;
    QFunctionPointer getProcAddress(const char *procName) override;

    QSurfaceFormat format() const override { return m_format; }
    bool isSharing() const override { return m_shareContext != EGL_NO_CONTEXT; }
    bool isValid() const override { return m_eglContext != EGL_NO_CONTEXT; }

    void *nativeContext() const override { return m_eglContext; }
    void *nativeDisplay() const override { return m_eglDisplay; }
    void *nativeConfig() const override { return m_eglConfig; }

private:
    EGLConfig chooseConfig(const QSurfaceFormat &format);

    QWindowsEGLStaticContext *m_staticContext;
    EGLContext m_eglContext;
    EGLContext m_shareContext;
    EGLDisplay m_eglDisplay;
    EGLConfig m_eglConfig;
    QSurfaceFormat m_format;
    EGLenum m_api = EGL_OPENGL_ES_API;
    int m_swapInterval = -1;
};

QT_END_NAMESPACE

#endif // QWINDOWSEGLCONTEXT_H
