// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qpa/qplatformopenglcontext.h>

#include <emscripten.h>
#include <emscripten/html5.h>

QT_BEGIN_NAMESPACE

class QOpenGLContext;
class QPlatformScreen;
class QWasmOpenGLContext : public QPlatformOpenGLContext
{
public:
    explicit QWasmOpenGLContext(QOpenGLContext *context);
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
    static EMSCRIPTEN_WEBGL_CONTEXT_HANDLE
    createEmscriptenContext(const std::string &canvasSelector, QSurfaceFormat format);

    QSurfaceFormat m_requestedFormat;
    QPlatformSurface *m_surface = nullptr;
    QOpenGLContext *m_qGlContext;
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE m_webGLContext = 0;
};

QT_END_NAMESPACE

