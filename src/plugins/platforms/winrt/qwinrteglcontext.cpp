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

#include "qwinrteglcontext.h"
#include "qwinrtwindow.h"
#include <private/qeventdispatcher_winrt_p.h>

#include <functional>

#include <d3d11.h>

#include <EGL/egl.h>
#define EGL_EGLEXT_PROTOTYPES
#include <EGL/eglext.h>

#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QtEglSupport/private/qeglconvenience_p.h>
#include <QtEglSupport/private/qeglpbuffer_p.h>

QT_BEGIN_NAMESPACE

struct WinRTEGLDisplay
{
    WinRTEGLDisplay() {
    }
    ~WinRTEGLDisplay() {
        eglTerminate(eglDisplay);
    }

    EGLDisplay eglDisplay;
};

Q_GLOBAL_STATIC(WinRTEGLDisplay, g)

class QWinRTEGLContextPrivate
{
public:
    QWinRTEGLContextPrivate() : eglContext(EGL_NO_CONTEXT), eglShareContext(EGL_NO_CONTEXT) { }
    QSurfaceFormat format;
    EGLConfig eglConfig;
    EGLContext eglContext;
    EGLContext eglShareContext;
};

QWinRTEGLContext::QWinRTEGLContext(QOpenGLContext *context)
    : d_ptr(new QWinRTEGLContextPrivate)
{
    Q_D(QWinRTEGLContext);
    d->format = context->format();
    d->format.setRenderableType(QSurfaceFormat::OpenGLES);
    if (QPlatformOpenGLContext *shareHandle = context->shareHandle())
        d->eglShareContext = static_cast<QWinRTEGLContext *>(shareHandle)->d_ptr->eglContext;
}

QWinRTEGLContext::~QWinRTEGLContext()
{
    Q_D(QWinRTEGLContext);
    if (d->eglContext != EGL_NO_CONTEXT)
        eglDestroyContext(g->eglDisplay, d->eglContext);
}

void QWinRTEGLContext::initialize()
{
    Q_D(QWinRTEGLContext);

    // Test if the hardware supports at least level 9_3
    D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_9_3 }; // minimum feature level
    HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, featureLevels, 1,
                                   D3D11_SDK_VERSION, nullptr, nullptr, nullptr);
    EGLint deviceType = SUCCEEDED(hr) ? EGL_PLATFORM_ANGLE_DEVICE_TYPE_HARDWARE_ANGLE
                                      : EGL_PLATFORM_ANGLE_DEVICE_TYPE_WARP_ANGLE;

    eglBindAPI(EGL_OPENGL_ES_API);

    const EGLint displayAttributes[] = {
        EGL_PLATFORM_ANGLE_TYPE_ANGLE, EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
        EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE, deviceType,
        EGL_PLATFORM_ANGLE_ENABLE_AUTOMATIC_TRIM_ANGLE, true,
        EGL_NONE,
    };
    g->eglDisplay = eglGetPlatformDisplayEXT(EGL_PLATFORM_ANGLE_ANGLE, EGL_DEFAULT_DISPLAY, displayAttributes);
    if (Q_UNLIKELY(g->eglDisplay == EGL_NO_DISPLAY))
        qCritical("Failed to initialize EGL display: 0x%x", eglGetError());

    // eglInitialize checks for EGL_PLATFORM_ANGLE_ENABLE_AUTOMATIC_TRIM_ANGLE
    // which adds a suspending handler. This needs to be added from the Xaml
    // thread itself, otherwise it will not be invoked. add_Suspending does
    // not return an error unfortunately, so it silently fails and causes
    // applications to not quit when the system wants to terminate the app
    // after suspend.
    hr = QEventDispatcherWinRT::runOnXamlThread([]() {
        if (!eglInitialize(g->eglDisplay, nullptr, nullptr))
            qCritical("Failed to initialize EGL: 0x%x", eglGetError());
        return S_OK;
    });
    d->eglConfig = q_configFromGLFormat(g->eglDisplay, d->format);

    const EGLint flags = d->format.testOption(QSurfaceFormat::DebugContext)
            ? EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR : 0;
    const EGLint attributes[] = {
        EGL_CONTEXT_CLIENT_VERSION, d->format.majorVersion(),
        EGL_CONTEXT_MINOR_VERSION_KHR, d->format.minorVersion(),
        EGL_CONTEXT_FLAGS_KHR, flags,
        EGL_CONTEXT_OPENGL_NO_ERROR_KHR, true,
        EGL_NONE
    };
    d->eglContext = eglCreateContext(g->eglDisplay, d->eglConfig, d->eglShareContext, attributes);
    if (d->eglContext == EGL_NO_CONTEXT) {
        qWarning("QEGLPlatformContext: Failed to create context: %x", eglGetError());
        return;
    }
}

bool QWinRTEGLContext::makeCurrent(QPlatformSurface *windowSurface)
{
    Q_D(QWinRTEGLContext);
    Q_ASSERT(windowSurface->surface()->supportsOpenGL());

    EGLSurface surface;
    if (windowSurface->surface()->surfaceClass() == QSurface::Window) {
        QWinRTWindow *window = static_cast<QWinRTWindow *>(windowSurface);
        if (window->eglSurface() == EGL_NO_SURFACE)
            window->createEglSurface(g->eglDisplay, d->eglConfig);

        surface = window->eglSurface();
    } else { // Offscreen
        surface = static_cast<QEGLPbuffer *>(windowSurface)->pbuffer();
    }

    if (surface == EGL_NO_SURFACE)
        return false;

    const bool ok = eglMakeCurrent(g->eglDisplay, surface, surface, d->eglContext);
    if (!ok) {
        qWarning("QEGLPlatformContext: eglMakeCurrent failed: %x", eglGetError());
        return false;
    }

    eglSwapInterval(g->eglDisplay, d->format.swapInterval());
    return true;
}

void QWinRTEGLContext::doneCurrent()
{
    const bool ok = eglMakeCurrent(g->eglDisplay, EGL_NO_SURFACE,
                                   EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (!ok)
        qWarning("QEGLPlatformContext: eglMakeCurrent failed: %x", eglGetError());
}

void QWinRTEGLContext::swapBuffers(QPlatformSurface *windowSurface)
{
    Q_ASSERT(windowSurface->surface()->supportsOpenGL());

    const QWinRTWindow *window = static_cast<QWinRTWindow *>(windowSurface);
    eglSwapBuffers(g->eglDisplay, window->eglSurface());
}

QSurfaceFormat QWinRTEGLContext::format() const
{
    Q_D(const QWinRTEGLContext);
    return d->format;
}

QFunctionPointer QWinRTEGLContext::getProcAddress(const char *procName)
{
    static QHash<QByteArray, QFunctionPointer> standardFuncs;
    if (standardFuncs.isEmpty()) {
        standardFuncs.insert(QByteArrayLiteral("glBindTexture"), (QFunctionPointer)&glBindTexture);
        standardFuncs.insert(QByteArrayLiteral("glBlendFunc"), (QFunctionPointer)&glBlendFunc);
        standardFuncs.insert(QByteArrayLiteral("glClear"), (QFunctionPointer)&glClear);
        standardFuncs.insert(QByteArrayLiteral("glClearColor"), (QFunctionPointer)&glClearColor);
        standardFuncs.insert(QByteArrayLiteral("glClearStencil"), (QFunctionPointer)&glClearStencil);
        standardFuncs.insert(QByteArrayLiteral("glColorMask"), (QFunctionPointer)&glColorMask);
        standardFuncs.insert(QByteArrayLiteral("glCopyTexImage2D"), (QFunctionPointer)&glCopyTexImage2D);
        standardFuncs.insert(QByteArrayLiteral("glCopyTexSubImage2D"), (QFunctionPointer)&glCopyTexSubImage2D);
        standardFuncs.insert(QByteArrayLiteral("glCullFace"), (QFunctionPointer)&glCullFace);
        standardFuncs.insert(QByteArrayLiteral("glDeleteTextures"), (QFunctionPointer)&glDeleteTextures);
        standardFuncs.insert(QByteArrayLiteral("glDepthFunc"), (QFunctionPointer)&glDepthFunc);
        standardFuncs.insert(QByteArrayLiteral("glDepthMask"), (QFunctionPointer)&glDepthMask);
        standardFuncs.insert(QByteArrayLiteral("glDisable"), (QFunctionPointer)&glDisable);
        standardFuncs.insert(QByteArrayLiteral("glDrawArrays"), (QFunctionPointer)&glDrawArrays);
        standardFuncs.insert(QByteArrayLiteral("glDrawElements"), (QFunctionPointer)&glDrawElements);
        standardFuncs.insert(QByteArrayLiteral("glEnable"), (QFunctionPointer)&glEnable);
        standardFuncs.insert(QByteArrayLiteral("glFinish"), (QFunctionPointer)&glFinish);
        standardFuncs.insert(QByteArrayLiteral("glFlush"), (QFunctionPointer)&glFlush);
        standardFuncs.insert(QByteArrayLiteral("glFrontFace"), (QFunctionPointer)&glFrontFace);
        standardFuncs.insert(QByteArrayLiteral("glGenTextures"), (QFunctionPointer)&glGenTextures);
        standardFuncs.insert(QByteArrayLiteral("glGetBooleanv"), (QFunctionPointer)&glGetBooleanv);
        standardFuncs.insert(QByteArrayLiteral("glGetError"), (QFunctionPointer)&glGetError);
        standardFuncs.insert(QByteArrayLiteral("glGetFloatv"), (QFunctionPointer)&glGetFloatv);
        standardFuncs.insert(QByteArrayLiteral("glGetIntegerv"), (QFunctionPointer)&glGetIntegerv);
        standardFuncs.insert(QByteArrayLiteral("glGetString"), (QFunctionPointer)&glGetString);
        standardFuncs.insert(QByteArrayLiteral("glGetTexParameterfv"), (QFunctionPointer)&glGetTexParameterfv);
        standardFuncs.insert(QByteArrayLiteral("glGetTexParameteriv"), (QFunctionPointer)&glGetTexParameteriv);
        standardFuncs.insert(QByteArrayLiteral("glHint"), (QFunctionPointer)&glHint);
        standardFuncs.insert(QByteArrayLiteral("glIsEnabled"), (QFunctionPointer)&glIsEnabled);
        standardFuncs.insert(QByteArrayLiteral("glIsTexture"), (QFunctionPointer)&glIsTexture);
        standardFuncs.insert(QByteArrayLiteral("glLineWidth"), (QFunctionPointer)&glLineWidth);
        standardFuncs.insert(QByteArrayLiteral("glPixelStorei"), (QFunctionPointer)&glPixelStorei);
        standardFuncs.insert(QByteArrayLiteral("glPolygonOffset"), (QFunctionPointer)&glPolygonOffset);
        standardFuncs.insert(QByteArrayLiteral("glReadPixels"), (QFunctionPointer)&glReadPixels);
        standardFuncs.insert(QByteArrayLiteral("glScissor"), (QFunctionPointer)&glScissor);
        standardFuncs.insert(QByteArrayLiteral("glStencilFunc"), (QFunctionPointer)&glStencilFunc);
        standardFuncs.insert(QByteArrayLiteral("glStencilMask"), (QFunctionPointer)&glStencilMask);
        standardFuncs.insert(QByteArrayLiteral("glStencilOp"), (QFunctionPointer)&glStencilOp);
        standardFuncs.insert(QByteArrayLiteral("glTexImage2D"), (QFunctionPointer)&glTexImage2D);
        standardFuncs.insert(QByteArrayLiteral("glTexParameterf"), (QFunctionPointer)&glTexParameterf);
        standardFuncs.insert(QByteArrayLiteral("glTexParameterfv"), (QFunctionPointer)&glTexParameterfv);
        standardFuncs.insert(QByteArrayLiteral("glTexParameteri"), (QFunctionPointer)&glTexParameteri);
        standardFuncs.insert(QByteArrayLiteral("glTexParameteriv"), (QFunctionPointer)&glTexParameteriv);
        standardFuncs.insert(QByteArrayLiteral("glTexSubImage2D"), (QFunctionPointer)&glTexSubImage2D);
        standardFuncs.insert(QByteArrayLiteral("glViewport"), (QFunctionPointer)&glViewport);
        standardFuncs.insert(QByteArrayLiteral("glActiveTexture"), (QFunctionPointer)&glActiveTexture);
        standardFuncs.insert(QByteArrayLiteral("glAttachShader"), (QFunctionPointer)&glAttachShader);
        standardFuncs.insert(QByteArrayLiteral("glBindAttribLocation"), (QFunctionPointer)&glBindAttribLocation);
        standardFuncs.insert(QByteArrayLiteral("glBindBuffer"), (QFunctionPointer)&glBindBuffer);
        standardFuncs.insert(QByteArrayLiteral("glBindFramebuffer"), (QFunctionPointer)&glBindFramebuffer);
        standardFuncs.insert(QByteArrayLiteral("glBindRenderbuffer"), (QFunctionPointer)&glBindRenderbuffer);
        standardFuncs.insert(QByteArrayLiteral("glBlendColor"), (QFunctionPointer)&glBlendColor);
        standardFuncs.insert(QByteArrayLiteral("glBlendEquation"), (QFunctionPointer)&glBlendEquation);
        standardFuncs.insert(QByteArrayLiteral("glBlendEquationSeparate"), (QFunctionPointer)&glBlendEquationSeparate);
        standardFuncs.insert(QByteArrayLiteral("glBlendFuncSeparate"), (QFunctionPointer)&glBlendFuncSeparate);
        standardFuncs.insert(QByteArrayLiteral("glBufferData"), (QFunctionPointer)&glBufferData);
        standardFuncs.insert(QByteArrayLiteral("glBufferSubData"), (QFunctionPointer)&glBufferSubData);
        standardFuncs.insert(QByteArrayLiteral("glCheckFramebufferStatus"), (QFunctionPointer)&glCheckFramebufferStatus);
        standardFuncs.insert(QByteArrayLiteral("glCompileShader"), (QFunctionPointer)&glCompileShader);
        standardFuncs.insert(QByteArrayLiteral("glCompressedTexImage2D"), (QFunctionPointer)&glCompressedTexImage2D);
        standardFuncs.insert(QByteArrayLiteral("glCompressedTexSubImage2D"), (QFunctionPointer)&glCompressedTexSubImage2D);
        standardFuncs.insert(QByteArrayLiteral("glCreateProgram"), (QFunctionPointer)&glCreateProgram);
        standardFuncs.insert(QByteArrayLiteral("glCreateShader"), (QFunctionPointer)&glCreateShader);
        standardFuncs.insert(QByteArrayLiteral("glDeleteBuffers"), (QFunctionPointer)&glDeleteBuffers);
        standardFuncs.insert(QByteArrayLiteral("glDeleteFramebuffers"), (QFunctionPointer)&glDeleteFramebuffers);
        standardFuncs.insert(QByteArrayLiteral("glDeleteProgram"), (QFunctionPointer)&glDeleteProgram);
        standardFuncs.insert(QByteArrayLiteral("glDeleteRenderbuffers"), (QFunctionPointer)&glDeleteRenderbuffers);
        standardFuncs.insert(QByteArrayLiteral("glDeleteShader"), (QFunctionPointer)&glDeleteShader);
        standardFuncs.insert(QByteArrayLiteral("glDetachShader"), (QFunctionPointer)&glDetachShader);
        standardFuncs.insert(QByteArrayLiteral("glDisableVertexAttribArray"), (QFunctionPointer)&glDisableVertexAttribArray);
        standardFuncs.insert(QByteArrayLiteral("glEnableVertexAttribArray"), (QFunctionPointer)&glEnableVertexAttribArray);
        standardFuncs.insert(QByteArrayLiteral("glFramebufferRenderbuffer"), (QFunctionPointer)&glFramebufferRenderbuffer);
        standardFuncs.insert(QByteArrayLiteral("glFramebufferTexture2D"), (QFunctionPointer)&glFramebufferTexture2D);
        standardFuncs.insert(QByteArrayLiteral("glGenBuffers"), (QFunctionPointer)&glGenBuffers);
        standardFuncs.insert(QByteArrayLiteral("glGenerateMipmap"), (QFunctionPointer)&glGenerateMipmap);
        standardFuncs.insert(QByteArrayLiteral("glGenFramebuffers"), (QFunctionPointer)&glGenFramebuffers);
        standardFuncs.insert(QByteArrayLiteral("glGenRenderbuffers"), (QFunctionPointer)&glGenRenderbuffers);
        standardFuncs.insert(QByteArrayLiteral("glGetActiveAttrib"), (QFunctionPointer)&glGetActiveAttrib);
        standardFuncs.insert(QByteArrayLiteral("glGetActiveUniform"), (QFunctionPointer)&glGetActiveUniform);
        standardFuncs.insert(QByteArrayLiteral("glGetAttachedShaders"), (QFunctionPointer)&glGetAttachedShaders);
        standardFuncs.insert(QByteArrayLiteral("glGetAttribLocation"), (QFunctionPointer)&glGetAttribLocation);
        standardFuncs.insert(QByteArrayLiteral("glGetBufferParameteriv"), (QFunctionPointer)&glGetBufferParameteriv);
        standardFuncs.insert(QByteArrayLiteral("glGetFramebufferAttachmentParameteriv"), (QFunctionPointer)&glGetFramebufferAttachmentParameteriv);
        standardFuncs.insert(QByteArrayLiteral("glGetProgramiv"), (QFunctionPointer)&glGetProgramiv);
        standardFuncs.insert(QByteArrayLiteral("glGetProgramInfoLog"), (QFunctionPointer)&glGetProgramInfoLog);
        standardFuncs.insert(QByteArrayLiteral("glGetRenderbufferParameteriv"), (QFunctionPointer)&glGetRenderbufferParameteriv);
        standardFuncs.insert(QByteArrayLiteral("glGetShaderiv"), (QFunctionPointer)&glGetShaderiv);
        standardFuncs.insert(QByteArrayLiteral("glGetShaderInfoLog"), (QFunctionPointer)&glGetShaderInfoLog);
        standardFuncs.insert(QByteArrayLiteral("glGetShaderPrecisionFormat"), (QFunctionPointer)&glGetShaderPrecisionFormat);
        standardFuncs.insert(QByteArrayLiteral("glGetShaderSource"), (QFunctionPointer)&glGetShaderSource);
        standardFuncs.insert(QByteArrayLiteral("glGetUniformfv"), (QFunctionPointer)&glGetUniformfv);
        standardFuncs.insert(QByteArrayLiteral("glGetUniformiv"), (QFunctionPointer)&glGetUniformiv);
        standardFuncs.insert(QByteArrayLiteral("glGetUniformLocation"), (QFunctionPointer)&glGetUniformLocation);
        standardFuncs.insert(QByteArrayLiteral("glGetVertexAttribfv"), (QFunctionPointer)&glGetVertexAttribfv);
        standardFuncs.insert(QByteArrayLiteral("glGetVertexAttribiv"), (QFunctionPointer)&glGetVertexAttribiv);
        standardFuncs.insert(QByteArrayLiteral("glGetVertexAttribPointerv"), (QFunctionPointer)&glGetVertexAttribPointerv);
        standardFuncs.insert(QByteArrayLiteral("glIsBuffer"), (QFunctionPointer)&glIsBuffer);
        standardFuncs.insert(QByteArrayLiteral("glIsFramebuffer"), (QFunctionPointer)&glIsFramebuffer);
        standardFuncs.insert(QByteArrayLiteral("glIsProgram"), (QFunctionPointer)&glIsProgram);
        standardFuncs.insert(QByteArrayLiteral("glIsRenderbuffer"), (QFunctionPointer)&glIsRenderbuffer);
        standardFuncs.insert(QByteArrayLiteral("glIsShader"), (QFunctionPointer)&glIsShader);
        standardFuncs.insert(QByteArrayLiteral("glLinkProgram"), (QFunctionPointer)&glLinkProgram);
        standardFuncs.insert(QByteArrayLiteral("glReleaseShaderCompiler"), (QFunctionPointer)&glReleaseShaderCompiler);
        standardFuncs.insert(QByteArrayLiteral("glRenderbufferStorage"), (QFunctionPointer)&glRenderbufferStorage);
        standardFuncs.insert(QByteArrayLiteral("glSampleCoverage"), (QFunctionPointer)&glSampleCoverage);
        standardFuncs.insert(QByteArrayLiteral("glShaderBinary"), (QFunctionPointer)&glShaderBinary);
        standardFuncs.insert(QByteArrayLiteral("glShaderSource"), (QFunctionPointer)&glShaderSource);
        standardFuncs.insert(QByteArrayLiteral("glStencilFuncSeparate"), (QFunctionPointer)&glStencilFuncSeparate);
        standardFuncs.insert(QByteArrayLiteral("glStencilMaskSeparate"), (QFunctionPointer)&glStencilMaskSeparate);
        standardFuncs.insert(QByteArrayLiteral("glStencilOpSeparate"), (QFunctionPointer)&glStencilOpSeparate);
        standardFuncs.insert(QByteArrayLiteral("glUniform1f"), (QFunctionPointer)&glUniform1f);
        standardFuncs.insert(QByteArrayLiteral("glUniform1fv"), (QFunctionPointer)&glUniform1fv);
        standardFuncs.insert(QByteArrayLiteral("glUniform1i"), (QFunctionPointer)&glUniform1i);
        standardFuncs.insert(QByteArrayLiteral("glUniform1iv"), (QFunctionPointer)&glUniform1iv);
        standardFuncs.insert(QByteArrayLiteral("glUniform2f"), (QFunctionPointer)&glUniform2f);
        standardFuncs.insert(QByteArrayLiteral("glUniform2fv"), (QFunctionPointer)&glUniform2fv);
        standardFuncs.insert(QByteArrayLiteral("glUniform2i"), (QFunctionPointer)&glUniform2i);
        standardFuncs.insert(QByteArrayLiteral("glUniform2iv"), (QFunctionPointer)&glUniform2iv);
        standardFuncs.insert(QByteArrayLiteral("glUniform3f"), (QFunctionPointer)&glUniform3f);
        standardFuncs.insert(QByteArrayLiteral("glUniform3fv"), (QFunctionPointer)&glUniform3fv);
        standardFuncs.insert(QByteArrayLiteral("glUniform3i"), (QFunctionPointer)&glUniform3i);
        standardFuncs.insert(QByteArrayLiteral("glUniform3iv"), (QFunctionPointer)&glUniform3iv);
        standardFuncs.insert(QByteArrayLiteral("glUniform4f"), (QFunctionPointer)&glUniform4f);
        standardFuncs.insert(QByteArrayLiteral("glUniform4fv"), (QFunctionPointer)&glUniform4fv);
        standardFuncs.insert(QByteArrayLiteral("glUniform4i"), (QFunctionPointer)&glUniform4i);
        standardFuncs.insert(QByteArrayLiteral("glUniform4iv"), (QFunctionPointer)&glUniform4iv);
        standardFuncs.insert(QByteArrayLiteral("glUniformMatrix2fv"), (QFunctionPointer)&glUniformMatrix2fv);
        standardFuncs.insert(QByteArrayLiteral("glUniformMatrix3fv"), (QFunctionPointer)&glUniformMatrix3fv);
        standardFuncs.insert(QByteArrayLiteral("glUniformMatrix4fv"), (QFunctionPointer)&glUniformMatrix4fv);
        standardFuncs.insert(QByteArrayLiteral("glUseProgram"), (QFunctionPointer)&glUseProgram);
        standardFuncs.insert(QByteArrayLiteral("glValidateProgram"), (QFunctionPointer)&glValidateProgram);
        standardFuncs.insert(QByteArrayLiteral("glVertexAttrib1f"), (QFunctionPointer)&glVertexAttrib1f);
        standardFuncs.insert(QByteArrayLiteral("glVertexAttrib1fv"), (QFunctionPointer)&glVertexAttrib1fv);
        standardFuncs.insert(QByteArrayLiteral("glVertexAttrib2f"), (QFunctionPointer)&glVertexAttrib2f);
        standardFuncs.insert(QByteArrayLiteral("glVertexAttrib2fv"), (QFunctionPointer)&glVertexAttrib2fv);
        standardFuncs.insert(QByteArrayLiteral("glVertexAttrib3f"), (QFunctionPointer)&glVertexAttrib3f);
        standardFuncs.insert(QByteArrayLiteral("glVertexAttrib3fv"), (QFunctionPointer)&glVertexAttrib3fv);
        standardFuncs.insert(QByteArrayLiteral("glVertexAttrib4f"), (QFunctionPointer)&glVertexAttrib4f);
        standardFuncs.insert(QByteArrayLiteral("glVertexAttrib4fv"), (QFunctionPointer)&glVertexAttrib4fv);
        standardFuncs.insert(QByteArrayLiteral("glVertexAttribPointer"), (QFunctionPointer)&glVertexAttribPointer);
        standardFuncs.insert(QByteArrayLiteral("glClearDepthf"), (QFunctionPointer)&glClearDepthf);
        standardFuncs.insert(QByteArrayLiteral("glDepthRangef"), (QFunctionPointer)&glDepthRangef);
    };

    QHash<QByteArray, QFunctionPointer>::const_iterator i = standardFuncs.find(procName);
    if (i != standardFuncs.end())
        return i.value();

    return eglGetProcAddress(procName);
}

EGLDisplay QWinRTEGLContext::display()
{
    return g->eglDisplay;
}

QT_END_NAMESPACE
