/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

#ifndef QOFFSCREENINTEGRATION_X11_H
#define QOFFSCREENINTEGRATION_X11_H

#include "qoffscreenintegration.h"
#include "qoffscreencommon.h"

#include <qglobal.h>
#include <qscopedpointer.h>

#include <qpa/qplatformopenglcontext.h>
#include <QtGui/qguiapplication.h>

QT_BEGIN_NAMESPACE

class QOffscreenX11Connection;
class QOffscreenX11Info;

class QOffscreenX11PlatformNativeInterface : public QOffscreenPlatformNativeInterface
#if QT_CONFIG(xcb)
                                           , public QNativeInterface::QX11Application
#endif
{
public:
    ~QOffscreenX11PlatformNativeInterface();

    void *nativeResourceForScreen(const QByteArray &resource, QScreen *screen) override;
#if !defined(QT_NO_OPENGL) && QT_CONFIG(xcb_glx_plugin)
    void *nativeResourceForContext(const QByteArray &resource, QOpenGLContext *context) override;
#endif
#if QT_CONFIG(xcb)
    Display *display() const override;
    xcb_connection_t *connection() const override { return nullptr; };
#endif
    QScopedPointer<QOffscreenX11Connection> m_connection;
};

class QOffscreenX11Integration : public QOffscreenIntegration
{
public:
    ~QOffscreenX11Integration();
    bool hasCapability(QPlatformIntegration::Capability cap) const override;

#if !defined(QT_NO_OPENGL) && QT_CONFIG(xcb_glx_plugin)
    QPlatformOpenGLContext *createPlatformOpenGLContext(QOpenGLContext *context) const override;
#endif
    QOffscreenX11PlatformNativeInterface *nativeInterface() const override;
};

class QOffscreenX11Connection {
public:
    QOffscreenX11Connection();
    ~QOffscreenX11Connection();

    QOffscreenX11Info *x11Info();

    void *display() const { return m_display; }
    int screenNumber() const { return m_screenNumber; }

private:
    void *m_display;
    int m_screenNumber;

    QScopedPointer<QOffscreenX11Info> m_x11Info;
};

#if QT_CONFIG(xcb_glx_plugin)
class QOffscreenX11GLXContextData;

class QOffscreenX11GLXContext : public QPlatformOpenGLContext
                              , public QNativeInterface::QGLXContext
{
public:
    QOffscreenX11GLXContext(QOffscreenX11Info *x11, QOpenGLContext *context);
    ~QOffscreenX11GLXContext();

    bool makeCurrent(QPlatformSurface *surface) override;
    void doneCurrent() override;
    void swapBuffers(QPlatformSurface *surface) override;
    QFunctionPointer getProcAddress(const char *procName) override;

    QSurfaceFormat format() const override;
    bool isSharing() const override;
    bool isValid() const override;

    GLXContext nativeContext() const override { return glxContext(); }

    void *glxConfig() const;
    GLXContext glxContext() const;

private:
    QScopedPointer<QOffscreenX11GLXContextData> d;
};
#endif // QT_CONFIG(xcb_glx_plugin)

QT_END_NAMESPACE

#endif
