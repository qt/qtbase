/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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
******************************************************************************/

#ifndef QOPENGLCONTEXT_PLATFORM_H
#define QOPENGLCONTEXT_PLATFORM_H

#ifndef QT_NO_OPENGL

#include <QtGui/qtguiglobal.h>
#include <QtGui/qopenglcontext.h>
#include <QtGui/qwindowdefs.h>

#include <QtCore/qnativeinterface.h>

#if defined(Q_OS_MACOS)
Q_FORWARD_DECLARE_OBJC_CLASS(NSOpenGLContext);
#endif

#if QT_CONFIG(xcb_glx_plugin)
struct __GLXcontextRec; typedef struct __GLXcontextRec *GLXContext;
#endif
#if QT_CONFIG(egl)
typedef void *EGLContext;
typedef void *EGLDisplay;
#endif

#if !defined(Q_OS_MACOS) && defined(Q_CLANG_QDOC)
typedef void *NSOpenGLContext;
#endif

QT_BEGIN_NAMESPACE

namespace QNativeInterface {

#if defined(Q_OS_MACOS) || defined(Q_CLANG_QDOC)
struct Q_GUI_EXPORT QCocoaGLContext
{
    QT_DECLARE_NATIVE_INTERFACE(QCocoaGLContext, 1, QOpenGLContext)
    static QOpenGLContext *fromNative(QT_IGNORE_DEPRECATIONS(NSOpenGLContext) *context, QOpenGLContext *shareContext = nullptr);
    virtual QT_IGNORE_DEPRECATIONS(NSOpenGLContext) *nativeContext() const = 0;
};
#endif

#if defined(Q_OS_WIN) || defined(Q_CLANG_QDOC)
struct Q_GUI_EXPORT QWGLContext
{
    QT_DECLARE_NATIVE_INTERFACE(QWGLContext, 1, QOpenGLContext)
    static HMODULE openGLModuleHandle();
    static QOpenGLContext *fromNative(HGLRC context, HWND window, QOpenGLContext *shareContext = nullptr);
    virtual HGLRC nativeContext() const = 0;
};
#endif

#if QT_CONFIG(xcb_glx_plugin) || defined(Q_CLANG_QDOC)
struct Q_GUI_EXPORT QGLXContext
{
    QT_DECLARE_NATIVE_INTERFACE(QGLXContext, 1, QOpenGLContext)
    static QOpenGLContext *fromNative(GLXContext configBasedContext, QOpenGLContext *shareContext = nullptr);
    static QOpenGLContext *fromNative(GLXContext visualBasedContext, void *visualInfo, QOpenGLContext *shareContext = nullptr);
    virtual GLXContext nativeContext() const = 0;
};
#endif

#if QT_CONFIG(egl) || defined(Q_CLANG_QDOC)
struct Q_GUI_EXPORT QEGLContext
{
    QT_DECLARE_NATIVE_INTERFACE(QEGLContext, 1, QOpenGLContext)
    static QOpenGLContext *fromNative(EGLContext context, EGLDisplay display, QOpenGLContext *shareContext = nullptr);
    virtual EGLContext nativeContext() const = 0;
};
#endif

} // QNativeInterface

QT_END_NAMESPACE

#endif // QT_NO_OPENGL

#endif // QOPENGLCONTEXT_PLATFORM_H
