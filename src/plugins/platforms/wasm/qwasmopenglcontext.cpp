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

QHash<QPlatformSurface *, EMSCRIPTEN_WEBGL_CONTEXT_HANDLE> QWasmOpenGLContext::s_contexts;

QWasmOpenGLContext::QWasmOpenGLContext(QOpenGLContext *context)
    : m_actualFormat(context->format())
{
    m_actualFormat.setRenderableType(QSurfaceFormat::OpenGLES);

    // if we set one, we need to set the other as well since in webgl, these are tied together
    if (m_actualFormat.depthBufferSize() < 0 && m_actualFormat.stencilBufferSize() > 0)
        m_actualFormat.setDepthBufferSize(16);

    if (m_actualFormat.stencilBufferSize() < 0 && m_actualFormat.depthBufferSize() > 0)
        m_actualFormat.setStencilBufferSize(8);
}

QWasmOpenGLContext::~QWasmOpenGLContext()
{
    destroyWebGLContext(m_contextOwningSurface);
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

void QWasmOpenGLContext::destroyWebGLContext(QPlatformSurface *surface)
{
    if (surface == nullptr)
        return;
    int context = s_contexts.take(surface);
    if (context)
        destroyWebGLContext(context);
}

void QWasmOpenGLContext::destroyWebGLContext(EMSCRIPTEN_WEBGL_CONTEXT_HANDLE contextHandle)
{
    if (!contextHandle)
        return;

    // Destroy GL context. Work around bug in emscripten_webgl_destroy_context
    // which removes all event handlers on the canvas by temporarily replacing the function
    // that does the removal with a function that does nothing.
    emscripten::val jsEvents = emscripten::val::module_property("JSEvents");
    emscripten::val savedRemoveAllHandlersOnTargetFunction = jsEvents["removeAllHandlersOnTarget"];
    jsEvents.set("removeAllHandlersOnTarget", emscripten::val::module_property("qtDoNothing"));
    emscripten_webgl_destroy_context(contextHandle);
    jsEvents.set("removeAllHandlersOnTarget", savedRemoveAllHandlersOnTargetFunction);
}

EMSCRIPTEN_WEBGL_CONTEXT_HANDLE QWasmOpenGLContext::createEmscriptenContext(const std::string &canvasSelector,
                                            QSurfaceFormat format)
{
    EmscriptenWebGLContextAttributes attributes;
    emscripten_webgl_init_context_attributes(&attributes); // Populate with default attributes

    attributes.powerPreference = EM_WEBGL_POWER_PREFERENCE_HIGH_PERFORMANCE;
    attributes.failIfMajorPerformanceCaveat = false;
    attributes.antialias = true;
    attributes.enableExtensionsByDefault = true;
    attributes.majorVersion = 2; // try highest supported version ES3.0 / WebGL 2.0
    attributes.minorVersion = 0; // emscripten only supports minor version 0
    // WebGL doesn't allow separate attach buffers to STENCIL_ATTACHMENT and DEPTH_ATTACHMENT
    // we need both or none
    const bool useDepthStencil = (format.depthBufferSize() > 0 || format.stencilBufferSize() > 0);

    // WebGL offers enable/disable control but not size control for these
    attributes.alpha = format.alphaBufferSize() > 0;
    attributes.depth = useDepthStencil;
    attributes.stencil = useDepthStencil;
    EMSCRIPTEN_RESULT contextResult = emscripten_webgl_create_context(canvasSelector.c_str(), &attributes);

    if (contextResult <= 0) {
        // fallback to opengles2/webgl1
        // for devices that do not support opengles3/webgl2
        attributes.majorVersion = 1;
        contextResult = emscripten_webgl_create_context(canvasSelector.c_str(), &attributes);
    }

    if (contextResult <= 0) {
        qWarning() << "WebGL context creation failed";
        return contextResult;
    }

    // Sync up actual format
    EmscriptenWebGLContextAttributes actualAttributes;
    EMSCRIPTEN_RESULT attributesResult = emscripten_webgl_get_context_attributes(contextResult, &actualAttributes);
    if (attributesResult == EMSCRIPTEN_RESULT_SUCCESS) {
        if (actualAttributes.majorVersion == 1) {
            m_actualFormat.setMajorVersion(2);
        } else if (actualAttributes.majorVersion == 2) {
            m_actualFormat.setMajorVersion(3);
        }
        m_actualFormat.setMinorVersion(0);
    }

    return contextResult;
}

QSurfaceFormat QWasmOpenGLContext::format() const
{
    return m_actualFormat;
}

GLuint QWasmOpenGLContext::defaultFramebufferObject(QPlatformSurface *surface) const
{
    return QPlatformOpenGLContext::defaultFramebufferObject(surface);
}

bool QWasmOpenGLContext::makeCurrent(QPlatformSurface *surface)
{
    // Record this makeCurrent() attempt, since isValid() must repeat the answer
    // from this function in order to signal context loss to calling code.
    m_madeCurrentSurface = surface;

    // The native webgl context is tied to a single surface, and can't
    // be made current for a different surface.
    if (m_contextOwningSurface && m_contextOwningSurface != surface)
        return false;

    // Return existing context or crate a new one.
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context;
    if (m_contextOwningSurface && s_contexts.contains(m_contextOwningSurface)) {
        context = s_contexts.value(surface);
    } else {
        m_contextOwningSurface = surface;
        bool isOffscreen = surface->surface()->surfaceClass() == QSurface::Offscreen;
        auto canvasId = isOffscreen ? static_cast<QWasmOffscreenSurface *>(surface)->id() :
                                      static_cast<QWasmWindow *>(surface)->canvasSelector();

        context = createEmscriptenContext(canvasId, m_actualFormat);
        s_contexts.insert(surface, context);
    }

    if (context == 0)
        return false;

    return emscripten_webgl_make_context_current(context) == EMSCRIPTEN_RESULT_SUCCESS;
}

void QWasmOpenGLContext::swapBuffers(QPlatformSurface *surface)
{
    Q_UNUSED(surface);
    // No swapbuffers on WebGl
}

void QWasmOpenGLContext::doneCurrent()
{
    m_madeCurrentSurface = nullptr;
}

bool QWasmOpenGLContext::isSharing() const
{
    // Return false to signal that context sharing is not supported.
    // This will in turn make QOpenGLContext::shareContext() return
    // a null context to the application.
    return false;
}

bool QWasmOpenGLContext::isValid() const
{
    if (!isOpenGLVersionSupported(m_actualFormat))
        return false;

    // We get isValid() calls before we see the surface and are able to
    // create a native context, which means that "no context" is a valid state.
    if (!m_madeCurrentSurface && !m_contextOwningSurface)
        return true;

    // Can't use this context for a different surface, since the native
    // webgl context is tied to a single canvas.
    if (m_madeCurrentSurface != m_contextOwningSurface)
        return false;

    // If the owning surfce/canvas has been deleted then this context is invalid
    if (!s_contexts.contains(m_contextOwningSurface))
        return false;

    return !emscripten_is_webgl_context_lost(s_contexts.value(m_contextOwningSurface));
}

QFunctionPointer QWasmOpenGLContext::getProcAddress(const char *procName)
{
    return reinterpret_cast<QFunctionPointer>(eglGetProcAddress(procName));
}

QT_END_NAMESPACE
