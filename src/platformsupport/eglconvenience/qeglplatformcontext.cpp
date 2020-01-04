/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qeglplatformcontext_p.h"
#include "qeglconvenience_p.h"
#include "qeglpbuffer_p.h"
#include <qpa/qplatformwindow.h>
#include <QOpenGLContext>
#include <QtPlatformHeaders/QEGLNativeContext>
#include <QDebug>

#if defined(Q_OS_ANDROID) && !defined(Q_OS_ANDROID_EMBEDDED)
#include <QtCore/private/qjnihelpers_p.h>
#endif
#ifndef Q_OS_WIN
#include <dlfcn.h>
#endif

QT_BEGIN_NAMESPACE

/*!
    \class QEGLPlatformContext
    \brief An EGL context implementation.
    \since 5.2
    \internal
    \ingroup qpa

    Implement QPlatformOpenGLContext using EGL. To use it in platform
    plugins a subclass must be created since
    eglSurfaceForPlatformSurface() has to be reimplemented. This
    function is used for mapping platform surfaces (windows) to EGL
    surfaces and is necessary since different platform plugins may
    have different ways of handling native windows (for example, a
    plugin may choose not to back every platform window by a real EGL
    surface). Other than that, no further customization is necessary.
 */

// Constants from EGL_KHR_create_context
#ifndef EGL_CONTEXT_MINOR_VERSION_KHR
#define EGL_CONTEXT_MINOR_VERSION_KHR 0x30FB
#endif
#ifndef EGL_CONTEXT_FLAGS_KHR
#define EGL_CONTEXT_FLAGS_KHR 0x30FC
#endif
#ifndef EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR
#define EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR 0x30FD
#endif
#ifndef EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR
#define EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR 0x00000001
#endif
#ifndef EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE_BIT_KHR
#define EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE_BIT_KHR 0x00000002
#endif
#ifndef EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR
#define EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR 0x00000001
#endif
#ifndef EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT_KHR
#define EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT_KHR 0x00000002
#endif

// Constants for OpenGL which are not available in the ES headers.
#ifndef GL_CONTEXT_FLAGS
#define GL_CONTEXT_FLAGS 0x821E
#endif
#ifndef GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT
#define GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT 0x0001
#endif
#ifndef GL_CONTEXT_FLAG_DEBUG_BIT
#define GL_CONTEXT_FLAG_DEBUG_BIT 0x00000002
#endif
#ifndef GL_CONTEXT_PROFILE_MASK
#define GL_CONTEXT_PROFILE_MASK 0x9126
#endif
#ifndef GL_CONTEXT_CORE_PROFILE_BIT
#define GL_CONTEXT_CORE_PROFILE_BIT 0x00000001
#endif
#ifndef GL_CONTEXT_COMPATIBILITY_PROFILE_BIT
#define GL_CONTEXT_COMPATIBILITY_PROFILE_BIT 0x00000002
#endif

QEGLPlatformContext::QEGLPlatformContext(const QSurfaceFormat &format, QPlatformOpenGLContext *share, EGLDisplay display,
                                         EGLConfig *config, const QVariant &nativeHandle, Flags flags)
    : m_eglDisplay(display)
    , m_swapInterval(-1)
    , m_swapIntervalEnvChecked(false)
    , m_swapIntervalFromEnv(-1)
    , m_flags(flags)
{
    if (nativeHandle.isNull()) {
        m_eglConfig = config ? *config : q_configFromGLFormat(display, format);
        m_ownsContext = true;
        init(format, share);
    } else {
        m_ownsContext = false;
        adopt(nativeHandle, share);
    }
}

void QEGLPlatformContext::init(const QSurfaceFormat &format, QPlatformOpenGLContext *share)
{
    m_format = q_glFormatFromConfig(m_eglDisplay, m_eglConfig, format);
    // m_format now has the renderableType() resolved (it cannot be Default anymore)
    // but does not yet contain version, profile, options.
    m_shareContext = share ? static_cast<QEGLPlatformContext *>(share)->m_eglContext : nullptr;

    QVector<EGLint> contextAttrs;
    contextAttrs.append(EGL_CONTEXT_CLIENT_VERSION);
    contextAttrs.append(format.majorVersion());
    const bool hasKHRCreateContext = q_hasEglExtension(m_eglDisplay, "EGL_KHR_create_context");
    if (hasKHRCreateContext) {
        contextAttrs.append(EGL_CONTEXT_MINOR_VERSION_KHR);
        contextAttrs.append(format.minorVersion());
        int flags = 0;
        // The debug bit is supported both for OpenGL and OpenGL ES.
        if (format.testOption(QSurfaceFormat::DebugContext))
            flags |= EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR;
        // The fwdcompat bit is only for OpenGL 3.0+.
        if (m_format.renderableType() == QSurfaceFormat::OpenGL
            && format.majorVersion() >= 3
            && !format.testOption(QSurfaceFormat::DeprecatedFunctions))
            flags |= EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE_BIT_KHR;
        if (flags) {
            contextAttrs.append(EGL_CONTEXT_FLAGS_KHR);
            contextAttrs.append(flags);
        }
        // Profiles are OpenGL only and mandatory in 3.2+. The value is silently ignored for < 3.2.
        if (m_format.renderableType() == QSurfaceFormat::OpenGL) {
            contextAttrs.append(EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR);
            contextAttrs.append(format.profile() == QSurfaceFormat::CoreProfile
                                ? EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR
                                : EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT_KHR);
        }
    }

    // Special Options for OpenVG surfaces
    if (m_format.renderableType() == QSurfaceFormat::OpenVG) {
        contextAttrs.append(EGL_ALPHA_MASK_SIZE);
        contextAttrs.append(8);
    }

    contextAttrs.append(EGL_NONE);
    m_contextAttrs = contextAttrs;

    switch (m_format.renderableType()) {
    case QSurfaceFormat::OpenVG:
        m_api = EGL_OPENVG_API;
        break;
#ifdef EGL_VERSION_1_4
    case QSurfaceFormat::OpenGL:
        m_api = EGL_OPENGL_API;
        break;
#endif // EGL_VERSION_1_4
    default:
        m_api = EGL_OPENGL_ES_API;
        break;
    }

    eglBindAPI(m_api);
    m_eglContext = eglCreateContext(m_eglDisplay, m_eglConfig, m_shareContext, contextAttrs.constData());
    if (m_eglContext == EGL_NO_CONTEXT && m_shareContext != EGL_NO_CONTEXT) {
        m_shareContext = nullptr;
        m_eglContext = eglCreateContext(m_eglDisplay, m_eglConfig, nullptr, contextAttrs.constData());
    }

    if (m_eglContext == EGL_NO_CONTEXT) {
        qWarning("QEGLPlatformContext: Failed to create context: %x", eglGetError());
        return;
    }

    static const bool printConfig = qEnvironmentVariableIntValue("QT_QPA_EGLFS_DEBUG");
    if (printConfig) {
        qDebug() << "Created context for format" << format << "with config:";
        q_printEglConfig(m_eglDisplay, m_eglConfig);
    }

    // Cannot just call updateFormatFromGL() since it relies on virtuals. Defer it to initialize().
}

void QEGLPlatformContext::adopt(const QVariant &nativeHandle, QPlatformOpenGLContext *share)
{
    if (!nativeHandle.canConvert<QEGLNativeContext>()) {
        qWarning("QEGLPlatformContext: Requires a QEGLNativeContext");
        return;
    }
    QEGLNativeContext handle = qvariant_cast<QEGLNativeContext>(nativeHandle);
    EGLContext context = handle.context();
    if (!context) {
        qWarning("QEGLPlatformContext: No EGLContext given");
        return;
    }

    // A context belonging to a given EGLDisplay cannot be used with another one.
    if (handle.display() != m_eglDisplay) {
        qWarning("QEGLPlatformContext: Cannot adopt context from different display");
        return;
    }

    // Figure out the EGLConfig.
    EGLint value = 0;
    eglQueryContext(m_eglDisplay, context, EGL_CONFIG_ID, &value);
    EGLint n = 0;
    EGLConfig cfg;
    const EGLint attribs[] = { EGL_CONFIG_ID, value, EGL_NONE };
    if (eglChooseConfig(m_eglDisplay, attribs, &cfg, 1, &n) && n == 1) {
        m_eglConfig = cfg;
        m_format = q_glFormatFromConfig(m_eglDisplay, m_eglConfig);
    } else {
        qWarning("QEGLPlatformContext: Failed to get framebuffer configuration for context");
    }

    // Fetch client API type.
    value = 0;
    eglQueryContext(m_eglDisplay, context, EGL_CONTEXT_CLIENT_TYPE, &value);
    if (value == EGL_OPENGL_API || value == EGL_OPENGL_ES_API) {
        // if EGL config supports both OpenGL and OpenGL ES render type,
        // q_glFormatFromConfig() with the default "referenceFormat" parameter
        // will always figure it out as OpenGL render type.
        // We can override it to match user's real render type.
        if (value == EGL_OPENGL_ES_API)
            m_format.setRenderableType(QSurfaceFormat::OpenGLES);
        m_api = value;
        eglBindAPI(m_api);
    } else {
        qWarning("QEGLPlatformContext: Failed to get client API type");
        m_api = EGL_OPENGL_ES_API;
    }

    m_eglContext = context;
    m_shareContext = share ? static_cast<QEGLPlatformContext *>(share)->m_eglContext : nullptr;
    updateFormatFromGL();
}

void QEGLPlatformContext::initialize()
{
    if (m_eglContext != EGL_NO_CONTEXT)
        updateFormatFromGL();
}

// Base implementation for pbuffers. Subclasses will handle the specialized cases for
// platforms without pbuffers.
EGLSurface QEGLPlatformContext::createTemporaryOffscreenSurface()
{
    // Make the context current to ensure the GL version query works. This needs a surface too.
    const EGLint pbufferAttributes[] = {
        EGL_WIDTH, 1,
        EGL_HEIGHT, 1,
        EGL_LARGEST_PBUFFER, EGL_FALSE,
        EGL_NONE
    };

    // Cannot just pass m_eglConfig because it may not be suitable for pbuffers. Instead,
    // do what QEGLPbuffer would do: request a config with the same attributes but with
    // PBUFFER_BIT set.
    EGLConfig config = q_configFromGLFormat(m_eglDisplay, m_format, false, EGL_PBUFFER_BIT);

    return eglCreatePbufferSurface(m_eglDisplay, config, pbufferAttributes);
}

void QEGLPlatformContext::destroyTemporaryOffscreenSurface(EGLSurface surface)
{
    eglDestroySurface(m_eglDisplay, surface);
}

void QEGLPlatformContext::runGLChecks()
{
    // Nothing to do here, subclasses may override in order to perform OpenGL
    // queries needing a context.
}

void QEGLPlatformContext::updateFormatFromGL()
{
#ifndef QT_NO_OPENGL
    // Have to save & restore to prevent QOpenGLContext::currentContext() from becoming
    // inconsistent after QOpenGLContext::create().
    EGLDisplay prevDisplay = eglGetCurrentDisplay();
    if (prevDisplay == EGL_NO_DISPLAY) // when no context is current
        prevDisplay = m_eglDisplay;
    EGLContext prevContext = eglGetCurrentContext();
    EGLSurface prevSurfaceDraw = eglGetCurrentSurface(EGL_DRAW);
    EGLSurface prevSurfaceRead = eglGetCurrentSurface(EGL_READ);

    // Rely on the surfaceless extension, if available. This is beneficial since we can
    // avoid creating an extra pbuffer surface which is apparently troublesome with some
    // drivers (Mesa) when certain attributes are present (multisampling).
    EGLSurface tempSurface = EGL_NO_SURFACE;
    EGLContext tempContext = EGL_NO_CONTEXT;
    if (m_flags.testFlag(NoSurfaceless) || !q_hasEglExtension(m_eglDisplay, "EGL_KHR_surfaceless_context"))
        tempSurface = createTemporaryOffscreenSurface();

    EGLBoolean ok = eglMakeCurrent(m_eglDisplay, tempSurface, tempSurface, m_eglContext);
    if (!ok) {
        EGLConfig config = q_configFromGLFormat(m_eglDisplay, m_format, false, EGL_PBUFFER_BIT);
        tempContext = eglCreateContext(m_eglDisplay, config, nullptr, m_contextAttrs.constData());
        if (tempContext != EGL_NO_CONTEXT)
            ok = eglMakeCurrent(m_eglDisplay, tempSurface, tempSurface, tempContext);
    }
    if (ok) {
        if (m_format.renderableType() == QSurfaceFormat::OpenGL
            || m_format.renderableType() == QSurfaceFormat::OpenGLES) {
            const GLubyte *s = glGetString(GL_VERSION);
            if (s) {
                QByteArray version = QByteArray(reinterpret_cast<const char *>(s));
                int major, minor;
                if (QPlatformOpenGLContext::parseOpenGLVersion(version, major, minor)) {
                    m_format.setMajorVersion(major);
                    m_format.setMinorVersion(minor);
                }
            }
            m_format.setProfile(QSurfaceFormat::NoProfile);
            m_format.setOptions(QSurfaceFormat::FormatOptions());
            if (m_format.renderableType() == QSurfaceFormat::OpenGL) {
                // Check profile and options.
                if (m_format.majorVersion() < 3) {
                    m_format.setOption(QSurfaceFormat::DeprecatedFunctions);
                } else {
                    GLint value = 0;
                    glGetIntegerv(GL_CONTEXT_FLAGS, &value);
                    if (!(value & GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT))
                        m_format.setOption(QSurfaceFormat::DeprecatedFunctions);
                    if (value & GL_CONTEXT_FLAG_DEBUG_BIT)
                        m_format.setOption(QSurfaceFormat::DebugContext);
                    if (m_format.version() >= qMakePair(3, 2)) {
                        value = 0;
                        glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &value);
                        if (value & GL_CONTEXT_CORE_PROFILE_BIT)
                            m_format.setProfile(QSurfaceFormat::CoreProfile);
                        else if (value & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT)
                            m_format.setProfile(QSurfaceFormat::CompatibilityProfile);
                    }
                }
            }
        }
        runGLChecks();
        eglMakeCurrent(prevDisplay, prevSurfaceDraw, prevSurfaceRead, prevContext);
    } else {
        qWarning("QEGLPlatformContext: Failed to make temporary surface current, format not updated (%x)", eglGetError());
    }
    if (tempSurface != EGL_NO_SURFACE)
        destroyTemporaryOffscreenSurface(tempSurface);
    if (tempContext != EGL_NO_CONTEXT)
        eglDestroyContext(m_eglDisplay, tempContext);
#endif // QT_NO_OPENGL
}

bool QEGLPlatformContext::makeCurrent(QPlatformSurface *surface)
{
    Q_ASSERT(surface->surface()->supportsOpenGL());

    eglBindAPI(m_api);

    EGLSurface eglSurface = eglSurfaceForPlatformSurface(surface);

    // shortcut: on some GPUs, eglMakeCurrent is not a cheap operation
    if (eglGetCurrentContext() == m_eglContext &&
        eglGetCurrentDisplay() == m_eglDisplay &&
        eglGetCurrentSurface(EGL_READ) == eglSurface &&
        eglGetCurrentSurface(EGL_DRAW) == eglSurface) {
        return true;
    }

    const bool ok = eglMakeCurrent(m_eglDisplay, eglSurface, eglSurface, m_eglContext);
    if (ok) {
        if (!m_swapIntervalEnvChecked) {
            m_swapIntervalEnvChecked = true;
            if (qEnvironmentVariableIsSet("QT_QPA_EGLFS_SWAPINTERVAL")) {
                QByteArray swapIntervalString = qgetenv("QT_QPA_EGLFS_SWAPINTERVAL");
                bool intervalOk;
                const int swapInterval = swapIntervalString.toInt(&intervalOk);
                if (intervalOk)
                    m_swapIntervalFromEnv = swapInterval;
            }
        }
        const int requestedSwapInterval = m_swapIntervalFromEnv >= 0
            ? m_swapIntervalFromEnv
            : surface->format().swapInterval();
        if (requestedSwapInterval >= 0 && m_swapInterval != requestedSwapInterval) {
            m_swapInterval = requestedSwapInterval;
            if (eglSurface != EGL_NO_SURFACE) // skip if using surfaceless context
                eglSwapInterval(eglDisplay(), m_swapInterval);
        }
    } else {
        qWarning("QEGLPlatformContext: eglMakeCurrent failed: %x", eglGetError());
    }

    return ok;
}

QEGLPlatformContext::~QEGLPlatformContext()
{
    if (m_ownsContext && m_eglContext != EGL_NO_CONTEXT)
        eglDestroyContext(m_eglDisplay, m_eglContext);

    m_eglContext = EGL_NO_CONTEXT;
}

void QEGLPlatformContext::doneCurrent()
{
    eglBindAPI(m_api);
    bool ok = eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (!ok)
        qWarning("QEGLPlatformContext: eglMakeCurrent failed: %x", eglGetError());
}

void QEGLPlatformContext::swapBuffers(QPlatformSurface *surface)
{
    eglBindAPI(m_api);
    EGLSurface eglSurface = eglSurfaceForPlatformSurface(surface);
    if (eglSurface != EGL_NO_SURFACE) { // skip if using surfaceless context
        bool ok = eglSwapBuffers(m_eglDisplay, eglSurface);
        if (!ok)
            qWarning("QEGLPlatformContext: eglSwapBuffers failed: %x", eglGetError());
    }
}

QFunctionPointer QEGLPlatformContext::getProcAddress(const char *procName)
{
    eglBindAPI(m_api);
    QFunctionPointer proc = (QFunctionPointer) eglGetProcAddress(procName);
#if !defined(Q_OS_WIN) && !defined(Q_OS_INTEGRITY)
    if (!proc)
        proc = (QFunctionPointer) dlsym(RTLD_DEFAULT, procName);
#elif !defined(QT_OPENGL_DYNAMIC)
    // On systems without KHR_get_all_proc_addresses and without
    // dynamic linking there still has to be a way to access the
    // standard GLES functions. QOpenGL(Extra)Functions never makes
    // direct GL API calls since Qt 5.7, so all such workarounds are
    // expected to be handled in the platform plugin.
    if (!proc) {
        static struct StdFunc {
            const char *name;
            QFunctionPointer func;
        } standardFuncs[] = {
#ifdef QT_OPENGL_ES_2
            { "glBindTexture", (QFunctionPointer) ::glBindTexture },
            { "glBlendFunc", (QFunctionPointer) ::glBlendFunc },
            { "glClear", (QFunctionPointer) ::glClear },
            { "glClearColor", (QFunctionPointer) ::glClearColor },
            { "glClearStencil", (QFunctionPointer) ::glClearStencil },
            { "glColorMask", (QFunctionPointer) ::glColorMask },
            { "glCopyTexImage2D", (QFunctionPointer) ::glCopyTexImage2D },
            { "glCopyTexSubImage2D", (QFunctionPointer) ::glCopyTexSubImage2D },
            { "glCullFace", (QFunctionPointer) ::glCullFace },
            { "glDeleteTextures", (QFunctionPointer) ::glDeleteTextures },
            { "glDepthFunc", (QFunctionPointer) ::glDepthFunc },
            { "glDepthMask", (QFunctionPointer) ::glDepthMask },
            { "glDisable", (QFunctionPointer) ::glDisable },
            { "glDrawArrays", (QFunctionPointer) ::glDrawArrays },
            { "glDrawElements", (QFunctionPointer) ::glDrawElements },
            { "glEnable", (QFunctionPointer) ::glEnable },
            { "glFinish", (QFunctionPointer) ::glFinish },
            { "glFlush", (QFunctionPointer) ::glFlush },
            { "glFrontFace", (QFunctionPointer) ::glFrontFace },
            { "glGenTextures", (QFunctionPointer) ::glGenTextures },
            { "glGetBooleanv", (QFunctionPointer) ::glGetBooleanv },
            { "glGetError", (QFunctionPointer) ::glGetError },
            { "glGetFloatv", (QFunctionPointer) ::glGetFloatv },
            { "glGetIntegerv", (QFunctionPointer) ::glGetIntegerv },
            { "glGetString", (QFunctionPointer) ::glGetString },
            { "glGetTexParameterfv", (QFunctionPointer) ::glGetTexParameterfv },
            { "glGetTexParameteriv", (QFunctionPointer) ::glGetTexParameteriv },
            { "glHint", (QFunctionPointer) ::glHint },
            { "glIsEnabled", (QFunctionPointer) ::glIsEnabled },
            { "glIsTexture", (QFunctionPointer) ::glIsTexture },
            { "glLineWidth", (QFunctionPointer) ::glLineWidth },
            { "glPixelStorei", (QFunctionPointer) ::glPixelStorei },
            { "glPolygonOffset", (QFunctionPointer) ::glPolygonOffset },
            { "glReadPixels", (QFunctionPointer) ::glReadPixels },
            { "glScissor", (QFunctionPointer) ::glScissor },
            { "glStencilFunc", (QFunctionPointer) ::glStencilFunc },
            { "glStencilMask", (QFunctionPointer) ::glStencilMask },
            { "glStencilOp", (QFunctionPointer) ::glStencilOp },
            { "glTexImage2D", (QFunctionPointer) ::glTexImage2D },
            { "glTexParameterf", (QFunctionPointer) ::glTexParameterf },
            { "glTexParameterfv", (QFunctionPointer) ::glTexParameterfv },
            { "glTexParameteri", (QFunctionPointer) ::glTexParameteri },
            { "glTexParameteriv", (QFunctionPointer) ::glTexParameteriv },
            { "glTexSubImage2D", (QFunctionPointer) ::glTexSubImage2D },
            { "glViewport", (QFunctionPointer) ::glViewport },

            { "glActiveTexture", (QFunctionPointer) ::glActiveTexture },
            { "glAttachShader", (QFunctionPointer) ::glAttachShader },
            { "glBindAttribLocation", (QFunctionPointer) ::glBindAttribLocation },
            { "glBindBuffer", (QFunctionPointer) ::glBindBuffer },
            { "glBindFramebuffer", (QFunctionPointer) ::glBindFramebuffer },
            { "glBindRenderbuffer", (QFunctionPointer) ::glBindRenderbuffer },
            { "glBlendColor", (QFunctionPointer) ::glBlendColor },
            { "glBlendEquation", (QFunctionPointer) ::glBlendEquation },
            { "glBlendEquationSeparate", (QFunctionPointer) ::glBlendEquationSeparate },
            { "glBlendFuncSeparate", (QFunctionPointer) ::glBlendFuncSeparate },
            { "glBufferData", (QFunctionPointer) ::glBufferData },
            { "glBufferSubData", (QFunctionPointer) ::glBufferSubData },
            { "glCheckFramebufferStatus", (QFunctionPointer) ::glCheckFramebufferStatus },
            { "glCompileShader", (QFunctionPointer) ::glCompileShader },
            { "glCompressedTexImage2D", (QFunctionPointer) ::glCompressedTexImage2D },
            { "glCompressedTexSubImage2D", (QFunctionPointer) ::glCompressedTexSubImage2D },
            { "glCreateProgram", (QFunctionPointer) ::glCreateProgram },
            { "glCreateShader", (QFunctionPointer) ::glCreateShader },
            { "glDeleteBuffers", (QFunctionPointer) ::glDeleteBuffers },
            { "glDeleteFramebuffers", (QFunctionPointer) ::glDeleteFramebuffers },
            { "glDeleteProgram", (QFunctionPointer) ::glDeleteProgram },
            { "glDeleteRenderbuffers", (QFunctionPointer) ::glDeleteRenderbuffers },
            { "glDeleteShader", (QFunctionPointer) ::glDeleteShader },
            { "glDetachShader", (QFunctionPointer) ::glDetachShader },
            { "glDisableVertexAttribArray", (QFunctionPointer) ::glDisableVertexAttribArray },
            { "glEnableVertexAttribArray", (QFunctionPointer) ::glEnableVertexAttribArray },
            { "glFramebufferRenderbuffer", (QFunctionPointer) ::glFramebufferRenderbuffer },
            { "glFramebufferTexture2D", (QFunctionPointer) ::glFramebufferTexture2D },
            { "glGenBuffers", (QFunctionPointer) ::glGenBuffers },
            { "glGenerateMipmap", (QFunctionPointer) ::glGenerateMipmap },
            { "glGenFramebuffers", (QFunctionPointer) ::glGenFramebuffers },
            { "glGenRenderbuffers", (QFunctionPointer) ::glGenRenderbuffers },
            { "glGetActiveAttrib", (QFunctionPointer) ::glGetActiveAttrib },
            { "glGetActiveUniform", (QFunctionPointer) ::glGetActiveUniform },
            { "glGetAttachedShaders", (QFunctionPointer) ::glGetAttachedShaders },
            { "glGetAttribLocation", (QFunctionPointer) ::glGetAttribLocation },
            { "glGetBufferParameteriv", (QFunctionPointer) ::glGetBufferParameteriv },
            { "glGetFramebufferAttachmentParameteriv", (QFunctionPointer) ::glGetFramebufferAttachmentParameteriv },
            { "glGetProgramiv", (QFunctionPointer) ::glGetProgramiv },
            { "glGetProgramInfoLog", (QFunctionPointer) ::glGetProgramInfoLog },
            { "glGetRenderbufferParameteriv", (QFunctionPointer) ::glGetRenderbufferParameteriv },
            { "glGetShaderiv", (QFunctionPointer) ::glGetShaderiv },
            { "glGetShaderInfoLog", (QFunctionPointer) ::glGetShaderInfoLog },
            { "glGetShaderPrecisionFormat", (QFunctionPointer) ::glGetShaderPrecisionFormat },
            { "glGetShaderSource", (QFunctionPointer) ::glGetShaderSource },
            { "glGetUniformfv", (QFunctionPointer) ::glGetUniformfv },
            { "glGetUniformiv", (QFunctionPointer) ::glGetUniformiv },
            { "glGetUniformLocation", (QFunctionPointer) ::glGetUniformLocation },
            { "glGetVertexAttribfv", (QFunctionPointer) ::glGetVertexAttribfv },
            { "glGetVertexAttribiv", (QFunctionPointer) ::glGetVertexAttribiv },
            { "glGetVertexAttribPointerv", (QFunctionPointer) ::glGetVertexAttribPointerv },
            { "glIsBuffer", (QFunctionPointer) ::glIsBuffer },
            { "glIsFramebuffer", (QFunctionPointer) ::glIsFramebuffer },
            { "glIsProgram", (QFunctionPointer) ::glIsProgram },
            { "glIsRenderbuffer", (QFunctionPointer) ::glIsRenderbuffer },
            { "glIsShader", (QFunctionPointer) ::glIsShader },
            { "glLinkProgram", (QFunctionPointer) ::glLinkProgram },
            { "glReleaseShaderCompiler", (QFunctionPointer) ::glReleaseShaderCompiler },
            { "glRenderbufferStorage", (QFunctionPointer) ::glRenderbufferStorage },
            { "glSampleCoverage", (QFunctionPointer) ::glSampleCoverage },
            { "glShaderBinary", (QFunctionPointer) ::glShaderBinary },
            { "glShaderSource", (QFunctionPointer) ::glShaderSource },
            { "glStencilFuncSeparate", (QFunctionPointer) ::glStencilFuncSeparate },
            { "glStencilMaskSeparate", (QFunctionPointer) ::glStencilMaskSeparate },
            { "glStencilOpSeparate", (QFunctionPointer) ::glStencilOpSeparate },
            { "glUniform1f", (QFunctionPointer) ::glUniform1f },
            { "glUniform1fv", (QFunctionPointer) ::glUniform1fv },
            { "glUniform1i", (QFunctionPointer) ::glUniform1i },
            { "glUniform1iv", (QFunctionPointer) ::glUniform1iv },
            { "glUniform2f", (QFunctionPointer) ::glUniform2f },
            { "glUniform2fv", (QFunctionPointer) ::glUniform2fv },
            { "glUniform2i", (QFunctionPointer) ::glUniform2i },
            { "glUniform2iv", (QFunctionPointer) ::glUniform2iv },
            { "glUniform3f", (QFunctionPointer) ::glUniform3f },
            { "glUniform3fv", (QFunctionPointer) ::glUniform3fv },
            { "glUniform3i", (QFunctionPointer) ::glUniform3i },
            { "glUniform3iv", (QFunctionPointer) ::glUniform3iv },
            { "glUniform4f", (QFunctionPointer) ::glUniform4f },
            { "glUniform4fv", (QFunctionPointer) ::glUniform4fv },
            { "glUniform4i", (QFunctionPointer) ::glUniform4i },
            { "glUniform4iv", (QFunctionPointer) ::glUniform4iv },
            { "glUniformMatrix2fv", (QFunctionPointer) ::glUniformMatrix2fv },
            { "glUniformMatrix3fv", (QFunctionPointer) ::glUniformMatrix3fv },
            { "glUniformMatrix4fv", (QFunctionPointer) ::glUniformMatrix4fv },
            { "glUseProgram", (QFunctionPointer) ::glUseProgram },
            { "glValidateProgram", (QFunctionPointer) ::glValidateProgram },
            { "glVertexAttrib1f", (QFunctionPointer) ::glVertexAttrib1f },
            { "glVertexAttrib1fv", (QFunctionPointer) ::glVertexAttrib1fv },
            { "glVertexAttrib2f", (QFunctionPointer) ::glVertexAttrib2f },
            { "glVertexAttrib2fv", (QFunctionPointer) ::glVertexAttrib2fv },
            { "glVertexAttrib3f", (QFunctionPointer) ::glVertexAttrib3f },
            { "glVertexAttrib3fv", (QFunctionPointer) ::glVertexAttrib3fv },
            { "glVertexAttrib4f", (QFunctionPointer) ::glVertexAttrib4f },
            { "glVertexAttrib4fv", (QFunctionPointer) ::glVertexAttrib4fv },
            { "glVertexAttribPointer", (QFunctionPointer) ::glVertexAttribPointer },

            { "glClearDepthf", (QFunctionPointer) ::glClearDepthf },
            { "glDepthRangef", (QFunctionPointer) ::glDepthRangef },
#endif // QT_OPENGL_ES_2

#ifdef QT_OPENGL_ES_3
            { "glBeginQuery", (QFunctionPointer) ::glBeginQuery },
            { "glBeginTransformFeedback", (QFunctionPointer) ::glBeginTransformFeedback },
            { "glBindBufferBase", (QFunctionPointer) ::glBindBufferBase },
            { "glBindBufferRange", (QFunctionPointer) ::glBindBufferRange },
            { "glBindSampler", (QFunctionPointer) ::glBindSampler },
            { "glBindTransformFeedback", (QFunctionPointer) ::glBindTransformFeedback },
            { "glBindVertexArray", (QFunctionPointer) ::glBindVertexArray },
            { "glBlitFramebuffer", (QFunctionPointer) ::glBlitFramebuffer },
            { "glClearBufferfi", (QFunctionPointer) ::glClearBufferfi },
            { "glClearBufferfv", (QFunctionPointer) ::glClearBufferfv },
            { "glClearBufferiv", (QFunctionPointer) ::glClearBufferiv },
            { "glClearBufferuiv", (QFunctionPointer) ::glClearBufferuiv },
            { "glClientWaitSync", (QFunctionPointer) ::glClientWaitSync },
            { "glCompressedTexImage3D", (QFunctionPointer) ::glCompressedTexImage3D },
            { "glCompressedTexSubImage3D", (QFunctionPointer) ::glCompressedTexSubImage3D },
            { "glCopyBufferSubData", (QFunctionPointer) ::glCopyBufferSubData },
            { "glCopyTexSubImage3D", (QFunctionPointer) ::glCopyTexSubImage3D },
            { "glDeleteQueries", (QFunctionPointer) ::glDeleteQueries },
            { "glDeleteSamplers", (QFunctionPointer) ::glDeleteSamplers },
            { "glDeleteSync", (QFunctionPointer) ::glDeleteSync },
            { "glDeleteTransformFeedbacks", (QFunctionPointer) ::glDeleteTransformFeedbacks },
            { "glDeleteVertexArrays", (QFunctionPointer) ::glDeleteVertexArrays },
            { "glDrawArraysInstanced", (QFunctionPointer) ::glDrawArraysInstanced },
            { "glDrawBuffers", (QFunctionPointer) ::glDrawBuffers },
            { "glDrawElementsInstanced", (QFunctionPointer) ::glDrawElementsInstanced },
            { "glDrawRangeElements", (QFunctionPointer) ::glDrawRangeElements },
            { "glEndQuery", (QFunctionPointer) ::glEndQuery },
            { "glEndTransformFeedback", (QFunctionPointer) ::glEndTransformFeedback },
            { "glFenceSync", (QFunctionPointer) ::glFenceSync },
            { "glFlushMappedBufferRange", (QFunctionPointer) ::glFlushMappedBufferRange },
            { "glFramebufferTextureLayer", (QFunctionPointer) ::glFramebufferTextureLayer },
            { "glGenQueries", (QFunctionPointer) ::glGenQueries },
            { "glGenSamplers", (QFunctionPointer) ::glGenSamplers },
            { "glGenTransformFeedbacks", (QFunctionPointer) ::glGenTransformFeedbacks },
            { "glGenVertexArrays", (QFunctionPointer) ::glGenVertexArrays },
            { "glGetActiveUniformBlockName", (QFunctionPointer) ::glGetActiveUniformBlockName },
            { "glGetActiveUniformBlockiv", (QFunctionPointer) ::glGetActiveUniformBlockiv },
            { "glGetActiveUniformsiv", (QFunctionPointer) ::glGetActiveUniformsiv },
            { "glGetBufferParameteri64v", (QFunctionPointer) ::glGetBufferParameteri64v },
            { "glGetBufferPointerv", (QFunctionPointer) ::glGetBufferPointerv },
            { "glGetFragDataLocation", (QFunctionPointer) ::glGetFragDataLocation },
            { "glGetInteger64i_v", (QFunctionPointer) ::glGetInteger64i_v },
            { "glGetInteger64v", (QFunctionPointer) ::glGetInteger64v },
            { "glGetIntegeri_v", (QFunctionPointer) ::glGetIntegeri_v },
            { "glGetInternalformativ", (QFunctionPointer) ::glGetInternalformativ },
            { "glGetProgramBinary", (QFunctionPointer) ::glGetProgramBinary },
            { "glGetQueryObjectuiv", (QFunctionPointer) ::glGetQueryObjectuiv },
            { "glGetQueryiv", (QFunctionPointer) ::glGetQueryiv },
            { "glGetSamplerParameterfv", (QFunctionPointer) ::glGetSamplerParameterfv },
            { "glGetSamplerParameteriv", (QFunctionPointer) ::glGetSamplerParameteriv },
            { "glGetStringi", (QFunctionPointer) ::glGetStringi },
            { "glGetSynciv", (QFunctionPointer) ::glGetSynciv },
            { "glGetTransformFeedbackVarying", (QFunctionPointer) ::glGetTransformFeedbackVarying },
            { "glGetUniformBlockIndex", (QFunctionPointer) ::glGetUniformBlockIndex },
            { "glGetUniformIndices", (QFunctionPointer) ::glGetUniformIndices },
            { "glGetUniformuiv", (QFunctionPointer) ::glGetUniformuiv },
            { "glGetVertexAttribIiv", (QFunctionPointer) ::glGetVertexAttribIiv },
            { "glGetVertexAttribIuiv", (QFunctionPointer) ::glGetVertexAttribIuiv },
            { "glInvalidateFramebuffer", (QFunctionPointer) ::glInvalidateFramebuffer },
            { "glInvalidateSubFramebuffer", (QFunctionPointer) ::glInvalidateSubFramebuffer },
            { "glIsQuery", (QFunctionPointer) ::glIsQuery },
            { "glIsSampler", (QFunctionPointer) ::glIsSampler },
            { "glIsSync", (QFunctionPointer) ::glIsSync },
            { "glIsTransformFeedback", (QFunctionPointer) ::glIsTransformFeedback },
            { "glIsVertexArray", (QFunctionPointer) ::glIsVertexArray },
            { "glMapBufferRange", (QFunctionPointer) ::glMapBufferRange },
            { "glPauseTransformFeedback", (QFunctionPointer) ::glPauseTransformFeedback },
            { "glProgramBinary", (QFunctionPointer) ::glProgramBinary },
            { "glProgramParameteri", (QFunctionPointer) ::glProgramParameteri },
            { "glReadBuffer", (QFunctionPointer) ::glReadBuffer },
            { "glRenderbufferStorageMultisample", (QFunctionPointer) ::glRenderbufferStorageMultisample },
            { "glResumeTransformFeedback", (QFunctionPointer) ::glResumeTransformFeedback },
            { "glSamplerParameterf", (QFunctionPointer) ::glSamplerParameterf },
            { "glSamplerParameterfv", (QFunctionPointer) ::glSamplerParameterfv },
            { "glSamplerParameteri", (QFunctionPointer) ::glSamplerParameteri },
            { "glSamplerParameteriv", (QFunctionPointer) ::glSamplerParameteriv },
            { "glTexImage3D", (QFunctionPointer) ::glTexImage3D },
            { "glTexStorage2D", (QFunctionPointer) ::glTexStorage2D },
            { "glTexStorage3D", (QFunctionPointer) ::glTexStorage3D },
            { "glTexSubImage3D", (QFunctionPointer) ::glTexSubImage3D },
            { "glTransformFeedbackVaryings", (QFunctionPointer) ::glTransformFeedbackVaryings },
            { "glUniform1ui", (QFunctionPointer) ::glUniform1ui },
            { "glUniform1uiv", (QFunctionPointer) ::glUniform1uiv },
            { "glUniform2ui", (QFunctionPointer) ::glUniform2ui },
            { "glUniform2uiv", (QFunctionPointer) ::glUniform2uiv },
            { "glUniform3ui", (QFunctionPointer) ::glUniform3ui },
            { "glUniform3uiv", (QFunctionPointer) ::glUniform3uiv },
            { "glUniform4ui", (QFunctionPointer) ::glUniform4ui },
            { "glUniform4uiv", (QFunctionPointer) ::glUniform4uiv },
            { "glUniformBlockBinding", (QFunctionPointer) ::glUniformBlockBinding },
            { "glUniformMatrix2x3fv", (QFunctionPointer) ::glUniformMatrix2x3fv },
            { "glUniformMatrix2x4fv", (QFunctionPointer) ::glUniformMatrix2x4fv },
            { "glUniformMatrix3x2fv", (QFunctionPointer) ::glUniformMatrix3x2fv },
            { "glUniformMatrix3x4fv", (QFunctionPointer) ::glUniformMatrix3x4fv },
            { "glUniformMatrix4x2fv", (QFunctionPointer) ::glUniformMatrix4x2fv },
            { "glUniformMatrix4x3fv", (QFunctionPointer) ::glUniformMatrix4x3fv },
            { "glUnmapBuffer", (QFunctionPointer) ::glUnmapBuffer },
            { "glVertexAttribDivisor", (QFunctionPointer) ::glVertexAttribDivisor },
            { "glVertexAttribI4i", (QFunctionPointer) ::glVertexAttribI4i },
            { "glVertexAttribI4iv", (QFunctionPointer) ::glVertexAttribI4iv },
            { "glVertexAttribI4ui", (QFunctionPointer) ::glVertexAttribI4ui },
            { "glVertexAttribI4uiv", (QFunctionPointer) ::glVertexAttribI4uiv },
            { "glVertexAttribIPointer", (QFunctionPointer) ::glVertexAttribIPointer },
            { "glWaitSync", (QFunctionPointer) ::glWaitSync },
#endif // QT_OPENGL_ES_3

#ifdef QT_OPENGL_ES_3_1
            { "glActiveShaderProgram", (QFunctionPointer) ::glActiveShaderProgram },
            { "glBindImageTexture", (QFunctionPointer) ::glBindImageTexture },
            { "glBindProgramPipeline", (QFunctionPointer) ::glBindProgramPipeline },
            { "glBindVertexBuffer", (QFunctionPointer) ::glBindVertexBuffer },
            { "glCreateShaderProgramv", (QFunctionPointer) ::glCreateShaderProgramv },
            { "glDeleteProgramPipelines", (QFunctionPointer) ::glDeleteProgramPipelines },
            { "glDispatchCompute", (QFunctionPointer) ::glDispatchCompute },
            { "glDispatchComputeIndirect", (QFunctionPointer) ::glDispatchComputeIndirect },
            { "glDrawArraysIndirect", (QFunctionPointer) ::glDrawArraysIndirect },
            { "glDrawElementsIndirect", (QFunctionPointer) ::glDrawElementsIndirect },
            { "glFramebufferParameteri", (QFunctionPointer) ::glFramebufferParameteri },
            { "glGenProgramPipelines", (QFunctionPointer) ::glGenProgramPipelines },
            { "glGetBooleani_v", (QFunctionPointer) ::glGetBooleani_v },
            { "glGetFramebufferParameteriv", (QFunctionPointer) ::glGetFramebufferParameteriv },
            { "glGetMultisamplefv", (QFunctionPointer) ::glGetMultisamplefv },
            { "glGetProgramInterfaceiv", (QFunctionPointer) ::glGetProgramInterfaceiv },
            { "glGetProgramPipelineInfoLog", (QFunctionPointer) ::glGetProgramPipelineInfoLog },
            { "glGetProgramPipelineiv", (QFunctionPointer) ::glGetProgramPipelineiv },
            { "glGetProgramResourceIndex", (QFunctionPointer) ::glGetProgramResourceIndex },
            { "glGetProgramResourceLocation", (QFunctionPointer) ::glGetProgramResourceLocation },
            { "glGetProgramResourceName", (QFunctionPointer) ::glGetProgramResourceName },
            { "glGetProgramResourceiv", (QFunctionPointer) ::glGetProgramResourceiv },
            { "glGetTexLevelParameterfv", (QFunctionPointer) ::glGetTexLevelParameterfv },
            { "glGetTexLevelParameteriv", (QFunctionPointer) ::glGetTexLevelParameteriv },
            { "glIsProgramPipeline", (QFunctionPointer) ::glIsProgramPipeline },
            { "glMemoryBarrier", (QFunctionPointer) ::glMemoryBarrier },
            { "glMemoryBarrierByRegion", (QFunctionPointer) ::glMemoryBarrierByRegion },
            { "glProgramUniform1f", (QFunctionPointer) ::glProgramUniform1f },
            { "glProgramUniform1fv", (QFunctionPointer) ::glProgramUniform1fv },
            { "glProgramUniform1i", (QFunctionPointer) ::glProgramUniform1i },
            { "glProgramUniform1iv", (QFunctionPointer) ::glProgramUniform1iv },
            { "glProgramUniform1ui", (QFunctionPointer) ::glProgramUniform1ui },
            { "glProgramUniform1uiv", (QFunctionPointer) ::glProgramUniform1uiv },
            { "glProgramUniform2f", (QFunctionPointer) ::glProgramUniform2f },
            { "glProgramUniform2fv", (QFunctionPointer) ::glProgramUniform2fv },
            { "glProgramUniform2i", (QFunctionPointer) ::glProgramUniform2i },
            { "glProgramUniform2iv", (QFunctionPointer) ::glProgramUniform2iv },
            { "glProgramUniform2ui", (QFunctionPointer) ::glProgramUniform2ui },
            { "glProgramUniform2uiv", (QFunctionPointer) ::glProgramUniform2uiv },
            { "glProgramUniform3f", (QFunctionPointer) ::glProgramUniform3f },
            { "glProgramUniform3fv", (QFunctionPointer) ::glProgramUniform3fv },
            { "glProgramUniform3i", (QFunctionPointer) ::glProgramUniform3i },
            { "glProgramUniform3iv", (QFunctionPointer) ::glProgramUniform3iv },
            { "glProgramUniform3ui", (QFunctionPointer) ::glProgramUniform3ui },
            { "glProgramUniform3uiv", (QFunctionPointer) ::glProgramUniform3uiv },
            { "glProgramUniform4f", (QFunctionPointer) ::glProgramUniform4f },
            { "glProgramUniform4fv", (QFunctionPointer) ::glProgramUniform4fv },
            { "glProgramUniform4i", (QFunctionPointer) ::glProgramUniform4i },
            { "glProgramUniform4iv", (QFunctionPointer) ::glProgramUniform4iv },
            { "glProgramUniform4ui", (QFunctionPointer) ::glProgramUniform4ui },
            { "glProgramUniform4uiv", (QFunctionPointer) ::glProgramUniform4uiv },
            { "glProgramUniformMatrix2fv", (QFunctionPointer) ::glProgramUniformMatrix2fv },
            { "glProgramUniformMatrix2x3fv", (QFunctionPointer) ::glProgramUniformMatrix2x3fv },
            { "glProgramUniformMatrix2x4fv", (QFunctionPointer) ::glProgramUniformMatrix2x4fv },
            { "glProgramUniformMatrix3fv", (QFunctionPointer) ::glProgramUniformMatrix3fv },
            { "glProgramUniformMatrix3x2fv", (QFunctionPointer) ::glProgramUniformMatrix3x2fv },
            { "glProgramUniformMatrix3x4fv", (QFunctionPointer) ::glProgramUniformMatrix3x4fv },
            { "glProgramUniformMatrix4fv", (QFunctionPointer) ::glProgramUniformMatrix4fv },
            { "glProgramUniformMatrix4x2fv", (QFunctionPointer) ::glProgramUniformMatrix4x2fv },
            { "glProgramUniformMatrix4x3fv", (QFunctionPointer) ::glProgramUniformMatrix4x3fv },
            { "glSampleMaski", (QFunctionPointer) ::glSampleMaski },
            { "glTexStorage2DMultisample", (QFunctionPointer) ::glTexStorage2DMultisample },
            { "glUseProgramStages", (QFunctionPointer) ::glUseProgramStages },
            { "glValidateProgramPipeline", (QFunctionPointer) ::glValidateProgramPipeline },
            { "glVertexAttribBinding", (QFunctionPointer) ::glVertexAttribBinding },
            { "glVertexAttribFormat", (QFunctionPointer) ::glVertexAttribFormat },
            { "glVertexAttribIFormat", (QFunctionPointer) ::glVertexAttribIFormat },
            { "glVertexBindingDivisor", (QFunctionPointer) ::glVertexBindingDivisor },
#endif // QT_OPENGL_ES_3_1

#ifdef QT_OPENGL_ES_3_2
            { "glBlendBarrier", (QFunctionPointer) ::glBlendBarrier },
            { "glCopyImageSubData", (QFunctionPointer) ::glCopyImageSubData },
            { "glDebugMessageControl", (QFunctionPointer) ::glDebugMessageControl },
            { "glDebugMessageInsert", (QFunctionPointer) ::glDebugMessageInsert },
            { "glDebugMessageCallback", (QFunctionPointer) ::glDebugMessageCallback },
            { "glGetDebugMessageLog", (QFunctionPointer) ::glGetDebugMessageLog },
            { "glPushDebugGroup", (QFunctionPointer) ::glPushDebugGroup },
            { "glPopDebugGroup", (QFunctionPointer) ::glPopDebugGroup },
            { "glObjectLabel", (QFunctionPointer) ::glObjectLabel },
            { "glGetObjectLabel", (QFunctionPointer) ::glGetObjectLabel },
            { "glObjectPtrLabel", (QFunctionPointer) ::glObjectPtrLabel },
            { "glGetObjectPtrLabel", (QFunctionPointer) ::glGetObjectPtrLabel },
            { "glGetPointerv", (QFunctionPointer) ::glGetPointerv },
            { "glEnablei", (QFunctionPointer) ::glEnablei },
            { "glDisablei", (QFunctionPointer) ::glDisablei },
            { "glBlendEquationi", (QFunctionPointer) ::glBlendEquationi },
            { "glBlendEquationSeparatei", (QFunctionPointer) ::glBlendEquationSeparatei },
            { "glBlendFunci", (QFunctionPointer) ::glBlendFunci },
            { "glBlendFuncSeparatei", (QFunctionPointer) ::glBlendFuncSeparatei },
            { "glColorMaski", (QFunctionPointer) ::glColorMaski },
            { "glIsEnabledi", (QFunctionPointer) ::glIsEnabledi },
            { "glDrawElementsBaseVertex", (QFunctionPointer) ::glDrawElementsBaseVertex },
            { "glDrawRangeElementsBaseVertex", (QFunctionPointer) ::glDrawRangeElementsBaseVertex },
            { "glDrawElementsInstancedBaseVertex", (QFunctionPointer) ::glDrawElementsInstancedBaseVertex },
            { "glFramebufferTexture", (QFunctionPointer) ::glFramebufferTexture },
            { "glPrimitiveBoundingBox", (QFunctionPointer) ::glPrimitiveBoundingBox },
            { "glGetGraphicsResetStatus", (QFunctionPointer) ::glGetGraphicsResetStatus },
            { "glReadnPixels", (QFunctionPointer) ::glReadnPixels },
            { "glGetnUniformfv", (QFunctionPointer) ::glGetnUniformfv },
            { "glGetnUniformiv", (QFunctionPointer) ::glGetnUniformiv },
            { "glGetnUniformuiv", (QFunctionPointer) ::glGetnUniformuiv },
            { "glMinSampleShading", (QFunctionPointer) ::glMinSampleShading },
            { "glPatchParameteri", (QFunctionPointer) ::glPatchParameteri },
            { "glTexParameterIiv", (QFunctionPointer) ::glTexParameterIiv },
            { "glTexParameterIuiv", (QFunctionPointer) ::glTexParameterIuiv },
            { "glGetTexParameterIiv", (QFunctionPointer) ::glGetTexParameterIiv },
            { "glGetTexParameterIuiv", (QFunctionPointer) ::glGetTexParameterIuiv },
            { "glSamplerParameterIiv", (QFunctionPointer) ::glSamplerParameterIiv },
            { "glSamplerParameterIuiv", (QFunctionPointer) ::glSamplerParameterIuiv },
            { "glGetSamplerParameterIiv", (QFunctionPointer) ::glGetSamplerParameterIiv },
            { "glGetSamplerParameterIuiv", (QFunctionPointer) ::glGetSamplerParameterIuiv },
            { "glTexBuffer", (QFunctionPointer) ::glTexBuffer },
            { "glTexBufferRange", (QFunctionPointer) ::glTexBufferRange },
            { "glTexStorage3DMultisample", (QFunctionPointer) ::glTexStorage3DMultisample },
#endif // QT_OPENGL_ES_3_2
        };

        for (size_t i = 0; i < sizeof(standardFuncs) / sizeof(StdFunc); ++i) {
            if (!qstrcmp(procName, standardFuncs[i].name)) {
                proc = standardFuncs[i].func;
                break;
            }
        }
    }
#endif

    return proc;
}

QSurfaceFormat QEGLPlatformContext::format() const
{
    return m_format;
}

EGLContext QEGLPlatformContext::eglContext() const
{
    return m_eglContext;
}

EGLDisplay QEGLPlatformContext::eglDisplay() const
{
    return m_eglDisplay;
}

EGLConfig QEGLPlatformContext::eglConfig() const
{
    return m_eglConfig;
}

QT_END_NAMESPACE
