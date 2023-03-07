// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmopenglcontext.h"

#include "qwasmoffscreensurface.h"
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

QWasmOpenGLContext::QWasmOpenGLContext(QOpenGLContext *context)
    : m_requestedFormat(context->format()), m_qGlContext(context)
{
    m_requestedFormat.setRenderableType(QSurfaceFormat::OpenGLES);

    // if we set one, we need to set the other as well since in webgl, these are tied together
    if (m_requestedFormat.depthBufferSize() < 0 && m_requestedFormat.stencilBufferSize() > 0)
        m_requestedFormat.setDepthBufferSize(16);

    if (m_requestedFormat.stencilBufferSize() < 0 && m_requestedFormat.depthBufferSize() > 0)
        m_requestedFormat.setStencilBufferSize(8);
}

QWasmOpenGLContext::~QWasmOpenGLContext()
{
    // Destroy GL context. Work around bug in emscripten_webgl_destroy_context
    // which removes all event handlers on the canvas by temporarily replacing the function
    // that does the removal with a function that does nothing.
    destroyWebGLContext(m_ownedWebGLContext.handle);
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

EMSCRIPTEN_WEBGL_CONTEXT_HANDLE
QWasmOpenGLContext::obtainEmscriptenContext(QPlatformSurface *surface)
{
    if (m_ownedWebGLContext.surface == surface)
        return m_ownedWebGLContext.handle;

    if (surface->surface()->surfaceClass() == QSurface::Offscreen) {
        if (const auto *shareContext = m_qGlContext->shareContext()) {
            // Since there are no resource sharing capabilities with WebGL whatsoever, we use the
            // same actual underlaying WebGL context. This is not perfect, but it works in most
            // cases.
            return static_cast<QWasmOpenGLContext *>(shareContext->handle())
                    ->m_ownedWebGLContext.handle;
        } else {
            // Reuse the existing context for offscreen drawing, even if it happens to be a canvas
            // context. This is because it is impossible to re-home an existing context to the
            // new surface and works as an emulation measure.
            if (m_ownedWebGLContext.handle)
                return m_ownedWebGLContext.handle;

            //  The non-shared offscreen context is heavily limited on WASM, but we provide it
            //  anyway for potential pixel readbacks.
            m_ownedWebGLContext =
                    QOpenGLContextData{ .surface = surface,
                                        .handle = createEmscriptenContext(
                                                static_cast<QWasmOffscreenSurface *>(surface)->id(),
                                                m_requestedFormat) };
            return m_ownedWebGLContext.handle;
        }
    } else {
        destroyWebGLContext(m_ownedWebGLContext.handle);

        // Create a full on-screen context for the window canvas.
        m_ownedWebGLContext = QOpenGLContextData{
            .surface = surface,
            .handle = createEmscriptenContext(static_cast<QWasmWindow *>(surface)->canvasSelector(),
                                              m_requestedFormat)
        };

        return m_ownedWebGLContext.handle;
    }
}

void QWasmOpenGLContext::destroyWebGLContext(EMSCRIPTEN_WEBGL_CONTEXT_HANDLE contextHandle)
{
    if (!contextHandle)
        return;
    emscripten::val jsEvents = emscripten::val::module_property("JSEvents");
    emscripten::val savedRemoveAllHandlersOnTargetFunction = jsEvents["removeAllHandlersOnTarget"];
    jsEvents.set("removeAllHandlersOnTarget", emscripten::val::module_property("qtDoNothing"));
    emscripten_webgl_destroy_context(contextHandle);
    jsEvents.set("removeAllHandlersOnTarget", savedRemoveAllHandlersOnTargetFunction);
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
    const auto context = obtainEmscriptenContext(surface);
    if (!context)
        return false;

    m_usedWebGLContextHandle = context;

    return emscripten_webgl_make_context_current(context) == EMSCRIPTEN_RESULT_SUCCESS;
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
    return m_qGlContext->shareContext();
}

bool QWasmOpenGLContext::isValid() const
{
    if (!isOpenGLVersionSupported(m_requestedFormat))
        return false;

    // Note: we get isValid() calls before we see the surface and can
    // create a native context, so no context is also a valid state.
    return !m_usedWebGLContextHandle || !emscripten_is_webgl_context_lost(m_usedWebGLContextHandle);
}

QFunctionPointer QWasmOpenGLContext::getProcAddress(const char *procName)
{
    return reinterpret_cast<QFunctionPointer>(eglGetProcAddress(procName));
}

QT_END_NAMESPACE
