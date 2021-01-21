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
****************************************************************************/

#include <qpa/qplatformopenglcontext.h>

#include <emscripten.h>
#include <emscripten/html5.h>

QT_BEGIN_NAMESPACE

class QPlatformScreen;
class QWasmOpenGLContext : public QPlatformOpenGLContext
{
public:
    QWasmOpenGLContext(const QSurfaceFormat &format);
    ~QWasmOpenGLContext();

    QSurfaceFormat format() const override;
    void swapBuffers(QPlatformSurface *surface) override;
    GLuint defaultFramebufferObject(QPlatformSurface *surface) const override;
    bool makeCurrent(QPlatformSurface *surface) override;
    void doneCurrent() override;
    bool isSharing() const override;
    bool isValid() const override;
    QFunctionPointer getProcAddress(const char *procName) override;

private:
    static bool isOpenGLVersionSupported(QSurfaceFormat format);
    bool maybeCreateEmscriptenContext(QPlatformSurface *surface);
    static EMSCRIPTEN_WEBGL_CONTEXT_HANDLE createEmscriptenContext(const QString &canvasId, QSurfaceFormat format);

    QSurfaceFormat m_requestedFormat;
    QPlatformScreen *m_screen = nullptr;
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE m_context = 0;
};

QT_END_NAMESPACE

