// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmopenglcontext.h"
#include "qwasmintegration.h"
#include <EGL/egl.h>
#include <emscripten/val.h>

QT_BEGIN_NAMESPACE

QWasmOpenGLContext::QWasmOpenGLContext(const QSurfaceFormat &format)
    : m_requestedFormat(format)
{
    m_requestedFormat.setRenderableType(QSurfaceFormat::OpenGLES);

    // if we set one, we need to set the other as well since in webgl, these are tied together
    if (format.depthBufferSize() < 0 && format.stencilBufferSize() > 0)
       m_requestedFormat.setDepthBufferSize(16);

   if (format.stencilBufferSize() < 0 && format.depthBufferSize() > 0)
       m_requestedFormat.setStencilBufferSize(8);

}

QWasmOpenGLContext::~QWasmOpenGLContext()
{
    if (m_context) {
        // Destroy GL context. Work around bug in emscripten_webgl_destroy_context
        // which removes all event handlers on the canvas by temporarily removing
        // emscripten's JSEvents global object.
        emscripten::val jsEvents = emscripten::val::global("window")["JSEvents"];
        emscripten::val::global("window").set("JSEvents", emscripten::val::undefined());
        emscripten_webgl_destroy_context(m_context);
        emscripten::val::global("window").set("JSEvents", jsEvents);
        m_context = 0;
    }
}

bool QWasmOpenGLContext::isOpenGLVersionSupported(QSurfaceFormat format)
{
    // Version check: support WebGL 1 and 2:
    // (ES) 2.0 -> WebGL 1.0
    // (ES) 3.0 -> WebGL 2.0
    // [we don't expect that new WebGL versions will be created]
    return ((format.majorVersion() == 2 && format.minorVersion() == 0) ||
           (format.majorVersion() == 3 && format.minorVersion() == 0));
}

bool QWasmOpenGLContext::maybeCreateEmscriptenContext(QPlatformSurface *surface)
{
    // Native emscripten/WebGL contexts are tied to a single screen/canvas. The first
    // call to this function creates a native canvas for the given screen, subsequent
    // calls verify that the surface is on/off the same screen.
    QPlatformScreen *screen = surface->screen();
    if (m_context && !screen)
        return false; // Alternative: return true to support makeCurrent on QOffScreenSurface with
                      // no screen. However, Qt likes to substitute QGuiApplication::primaryScreen()
                      // for null screens, which foils this plan.
    if (!screen)
        return false;
    if (m_context)
        return m_screen == screen;

    m_context = createEmscriptenContext(QWasmScreen::get(screen)->canvasTargetId(), m_requestedFormat);
    m_screen = screen;
    return true;
}

EMSCRIPTEN_WEBGL_CONTEXT_HANDLE QWasmOpenGLContext::createEmscriptenContext(const QString &canvasTargetId, QSurfaceFormat format)
{
    EmscriptenWebGLContextAttributes attributes;
    emscripten_webgl_init_context_attributes(&attributes); // Populate with default attributes

    attributes.powerPreference = EM_WEBGL_POWER_PREFERENCE_HIGH_PERFORMANCE;
    attributes.failIfMajorPerformanceCaveat = false;
    attributes.antialias = true;
    attributes.enableExtensionsByDefault = true;
    attributes.majorVersion = format.majorVersion() - 1;
    attributes.minorVersion = format.minorVersion();

    // WebGL doesn't allow separate attach buffers to STENCIL_ATTACHMENT and DEPTH_ATTACHMENT
    // we need both or none
    bool useDepthStencil = (format.depthBufferSize() > 0 || format.stencilBufferSize() > 0);

    // WebGL offers enable/disable control but not size control for these
    attributes.alpha = format.alphaBufferSize() > 0;
    attributes.depth = useDepthStencil;
    attributes.stencil = useDepthStencil;

    QByteArray convasSelector = canvasTargetId.toUtf8();
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context = emscripten_webgl_create_context(convasSelector.constData(), &attributes);

    return context;
}

QSurfaceFormat QWasmOpenGLContext::format() const
{
    return m_requestedFormat;
}

GLuint QWasmOpenGLContext::defaultFramebufferObject(QPlatformSurface *surface) const
{
    return QPlatformOpenGLContext::defaultFramebufferObject(surface);
}

bool QWasmOpenGLContext::makeCurrent(QPlatformSurface *surface)
{
    bool ok = maybeCreateEmscriptenContext(surface);
    if (!ok)
        return false;

    return emscripten_webgl_make_context_current(m_context) == EMSCRIPTEN_RESULT_SUCCESS;
}

void QWasmOpenGLContext::swapBuffers(QPlatformSurface *surface)
{
    Q_UNUSED(surface);
    // No swapbuffers on WebGl
}

void QWasmOpenGLContext::doneCurrent()
{
    // No doneCurrent on WebGl
}

bool QWasmOpenGLContext::isSharing() const
{
    return false;
}

bool QWasmOpenGLContext::isValid() const
{
    if (!(isOpenGLVersionSupported(m_requestedFormat)))
        return false;

    // Note: we get isValid() calls before we see the surface and can
    // create a native context, so no context is also a valid state.
    return !m_context || !emscripten_is_webgl_context_lost(m_context);
}

QFunctionPointer QWasmOpenGLContext::getProcAddress(const char *procName)
{
    return reinterpret_cast<QFunctionPointer>(eglGetProcAddress(procName));
}

QT_END_NAMESPACE
