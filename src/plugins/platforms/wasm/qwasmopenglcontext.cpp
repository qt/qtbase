// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmopenglcontext.h"
#include "qwasmintegration.h"
#include <EGL/egl.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>

namespace {
void qtDoNothing(emscripten::val) { }
} // namespace

EMSCRIPTEN_BINDINGS(qwasmopenglcontext)
{
    function("qtDoNothing", &qtDoNothing);
}

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
    if (!m_context)
        return;

    // Destroy GL context. Work around bug in emscripten_webgl_destroy_context
    // which removes all event handlers on the canvas by temporarily replacing the function
    // that does the removal with a function that does nothing.
    emscripten::val jsEvents = emscripten::val::module_property("JSEvents");
    emscripten::val savedRemoveAllHandlersOnTargetFunction =
            jsEvents["removeAllHandlersOnTarget"];
    jsEvents.set("removeAllHandlersOnTarget", emscripten::val::module_property("qtDoNothing"));
    emscripten_webgl_destroy_context(m_context);
    jsEvents.set("removeAllHandlersOnTarget", savedRemoveAllHandlersOnTargetFunction);
    m_context = 0;
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
    if (m_context && m_surface == surface)
        return true;

    // TODO(mikolajboc): Use OffscreenCanvas if available.
    if (surface->surface()->surfaceClass() == QSurface::Offscreen)
        return false;

    m_surface = surface;

    auto *window = static_cast<QWasmWindow *>(surface);
    m_context = createEmscriptenContext(window->canvasSelector(), m_requestedFormat);
    return true;
}

EMSCRIPTEN_WEBGL_CONTEXT_HANDLE
QWasmOpenGLContext::createEmscriptenContext(const std::string &canvasSelector,
                                            QSurfaceFormat format)
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
    const bool useDepthStencil = (format.depthBufferSize() > 0 || format.stencilBufferSize() > 0);

    // WebGL offers enable/disable control but not size control for these
    attributes.alpha = format.alphaBufferSize() > 0;
    attributes.depth = useDepthStencil;
    attributes.stencil = useDepthStencil;

    return emscripten_webgl_create_context(canvasSelector.c_str(), &attributes);
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
    if (!maybeCreateEmscriptenContext(surface))
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
    if (!isOpenGLVersionSupported(m_requestedFormat))
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
