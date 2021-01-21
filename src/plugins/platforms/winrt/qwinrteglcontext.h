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

#include <qpa/qplatformopenglcontext.h>
#include <EGL/egl.h>

QT_BEGIN_NAMESPACE

class QWinRTEGLContextPrivate;
class QWinRTEGLContext : public QPlatformOpenGLContext
{
public:
    explicit QWinRTEGLContext(QOpenGLContext *context);
    ~QWinRTEGLContext() override;

    void initialize() override;

    bool makeCurrent(QPlatformSurface *windowSurface) override;
    void doneCurrent() override;
    void swapBuffers(QPlatformSurface *windowSurface) override;

    QSurfaceFormat format() const override;
    QFunctionPointer getProcAddress(const char *procName) override;
    bool isValid() const override;

    static EGLDisplay display();
private:
    QScopedPointer<QWinRTEGLContextPrivate> d_ptr;
    Q_DECLARE_PRIVATE(QWinRTEGLContext)
};

QT_END_NAMESPACE

#endif // QWINDOWSEGLCONTEXT_H
