/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "QHTML5OpenGLContext.h"

#include <EGL/egl.h>

QT_BEGIN_NAMESPACE

QHTML5OpenGLContext::QHTML5OpenGLContext(const QSurfaceFormat &format)
    :m_requestedFormat(format)
{
    m_requestedFormat.setRenderableType(QSurfaceFormat::OpenGLES);
}

QHTML5OpenGLContext::~QHTML5OpenGLContext()
{
    if (m_context)
        emscripten_webgl_destroy_context(m_context);
}

void QHTML5OpenGLContext::maybeRecreateEmscriptenContext(QPlatformSurface *surface)
{
    // Native emscripten contexts are tied to a single surface. Recreate
    // the context if the surface is changed.
    if (surface != m_surface) {
        m_surface = surface;

        // Destroy existing context
        if (m_context)
            emscripten_webgl_destroy_context(m_context);

        // Create new context
        const char *canvasId = 0; // (use default canvas) FIXME: get the actual canvas from the surface.
        m_context = createEmscriptenContext(canvasId, m_requestedFormat);

        // Register context-lost callback.
        auto callback = [](int eventType, const void *reserved, void *userData) -> EM_BOOL
        {
            Q_UNUSED(eventType);
            Q_UNUSED(reserved);
            // The application may get contex-lost if e.g. moved to the background. Set
            // m_contextLost which will make isValid() return false. Application code will
            // then detect this and recrate the the context, resulting in a new QHTML5OpenGLContext
            // instance.
            reinterpret_cast<QHTML5OpenGLContext *>(userData)->m_contextLost = true;
            return true;
        };
        bool capture = true;
        emscripten_set_webglcontextlost_callback(canvasId, this, capture, callback);
    }
}

EMSCRIPTEN_WEBGL_CONTEXT_HANDLE QHTML5OpenGLContext::createEmscriptenContext(const char *canvasId, QSurfaceFormat format)
{
    EmscriptenWebGLContextAttributes attributes;
    emscripten_webgl_init_context_attributes(&attributes); // Populate with default attributes

    attributes.preferLowPowerToHighPerformance = false;
    attributes.failIfMajorPerformanceCaveat = false;
    attributes.antialias = true;
    attributes.enableExtensionsByDefault = true;

    // WebGL offers enable/disable control but not size control for these
    attributes.alpha = format.alphaBufferSize() > 0;
    attributes.depth = format.depthBufferSize() > 0;
    attributes.stencil = format.stencilBufferSize() > 0;

    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context = emscripten_webgl_create_context(canvasId, &attributes);

    return context;
}

QSurfaceFormat QHTML5OpenGLContext::format() const
{
    return m_requestedFormat;
}

GLuint QHTML5OpenGLContext::defaultFramebufferObject(QPlatformSurface *surface) const
{
    return QPlatformOpenGLContext::defaultFramebufferObject(surface);
}

bool QHTML5OpenGLContext::makeCurrent(QPlatformSurface *surface)
{
    maybeRecreateEmscriptenContext(surface);

    return emscripten_webgl_make_context_current(m_context) == EMSCRIPTEN_RESULT_SUCCESS;
}

void QHTML5OpenGLContext::swapBuffers(QPlatformSurface *surface)
{
    Q_UNUSED(surface);
    // No swapbuffers on WebGl
}

void QHTML5OpenGLContext::doneCurrent()
{
    // No doneCurrent on WebGl
}

bool QHTML5OpenGLContext::isSharing() const
{
    return false;
}

bool QHTML5OpenGLContext::isValid() const
{
    return (m_contextLost == false);
}

QFunctionPointer QHTML5OpenGLContext::getProcAddress(const char *procName)
{
    return reinterpret_cast<QFunctionPointer>(eglGetProcAddress(procName));
}

QT_END_NAMESPACE
