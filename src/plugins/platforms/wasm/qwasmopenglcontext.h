// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QWASMOPENGLCONTEXT_H
#define QWASMOPENGLCONTEXT_H

#include <qpa/qplatformopenglcontext.h>

#include <emscripten.h>
#include <emscripten/html5.h>

QT_BEGIN_NAMESPACE

class QOpenGLContext;
class QPlatformScreen;
class QPlatformSurface;
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
    struct QOpenGLContextData
    {
        QPlatformSurface *surface = nullptr;
        EMSCRIPTEN_WEBGL_CONTEXT_HANDLE handle = 0;
    };

    static bool isOpenGLVersionSupported(QSurfaceFormat format);
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE obtainEmscriptenContext(QPlatformSurface *surface);
    static EMSCRIPTEN_WEBGL_CONTEXT_HANDLE
    createEmscriptenContext(const std::string &canvasSelector, QSurfaceFormat format);

    static void destroyWebGLContext(EMSCRIPTEN_WEBGL_CONTEXT_HANDLE contextHandle);

    QSurfaceFormat m_requestedFormat;
    QOpenGLContext *m_qGlContext;
    QOpenGLContextData m_ownedWebGLContext;
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE m_usedWebGLContextHandle = 0;
};

QT_END_NAMESPACE

#endif // QWASMOPENGLCONTEXT_H
