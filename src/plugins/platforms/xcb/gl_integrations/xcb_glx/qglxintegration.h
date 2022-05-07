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

#ifndef QGLXINTEGRATION_H
#define QGLXINTEGRATION_H

#include "qxcbwindow.h"
#include "qxcbscreen.h"

#include <qpa/qplatformopenglcontext.h>
#include <qpa/qplatformoffscreensurface.h>
#include <QtGui/QSurfaceFormat>

#include <QtCore/QMutex>

#include <GL/glx.h>

QT_BEGIN_NAMESPACE

class QGLXContext : public QPlatformOpenGLContext,
                    public QNativeInterface::QGLXContext
{
public:
    QGLXContext(Display *display, QXcbScreen *screen, const QSurfaceFormat &format, QPlatformOpenGLContext *share);
    QGLXContext(Display *display, GLXContext context, void *visualInfo, QPlatformOpenGLContext *share);
    ~QGLXContext();

    bool makeCurrent(QPlatformSurface *surface) override;
    void doneCurrent() override;
    void swapBuffers(QPlatformSurface *surface) override;
    QFunctionPointer getProcAddress(const char *procName) override;

    QSurfaceFormat format() const override;
    bool isSharing() const override;
    bool isValid() const override;

    GLXContext nativeContext() const override { return glxContext(); }

    GLXContext glxContext() const { return m_context; }
    GLXFBConfig glxConfig() const { return m_config; }

    static bool supportsThreading();
    static void queryDummyContext();

private:
    Display *m_display = nullptr;
    GLXFBConfig m_config = nullptr;
    GLXContext m_context = nullptr;
    GLXContext m_shareContext = nullptr;
    QSurfaceFormat m_format;
    bool m_isPBufferCurrent = false;
    bool m_ownsContext = false;
    GLenum (APIENTRY * m_getGraphicsResetStatus)() = nullptr;
    bool m_lost = false;
    static bool m_queriedDummyContext;
    static bool m_supportsThreading;
};


class QGLXPbuffer : public QPlatformOffscreenSurface
{
public:
    explicit QGLXPbuffer(QOffscreenSurface *offscreenSurface);
    ~QGLXPbuffer();

    QSurfaceFormat format() const override { return m_format; }
    bool isValid() const override { return m_pbuffer != 0; }

    GLXPbuffer pbuffer() const { return m_pbuffer; }

private:
    QXcbScreen *m_screen;
    QSurfaceFormat m_format;
    Display *m_display;
    GLXPbuffer m_pbuffer;
};

QT_END_NAMESPACE

#endif
