// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosegl.h"
#include <QtCore/private/qohoscommon_p.h>
#include <qohosjsenv_p.h>

#include <QtCore/qmap.h>

QT_BEGIN_NAMESPACE

void QOhosEGLSurface::setNativeWindowSurface(
    EGLNativeWindowType nativeWindow, const QOhosOptional<QSize> &optSurfaceSize)
{
    m_targetSurfaceSize = optSurfaceSize;
    m_refTargetNativeWindow = nativeWindow;
}

void QOhosEGLSurface::tryCreateSurface(EGLDisplay display, EGLConfig config, SurfaceFlags surfaceFlags)
{
    bool canBuild = m_refTargetNativeWindow != nullptr;
    if (!canBuild)
        return;

    EGLint surfaceAttribs[] = {
        EGL_RENDER_BUFFER, EGL_BACK_BUFFER,
        EGL_NONE
    };

    EGLNativeWindowType eglNativeWindow = m_refTargetNativeWindow;

    EGLSurface surface = eglCreateWindowSurface(
        display,
        config,
        eglNativeWindow,
        surfaceAttribs);

    std::string errMessage;
    if (surface == EGL_NO_SURFACE) {
        auto eglErrorCode = eglGetError();
        switch (eglErrorCode) {
        case EGL_BAD_MATCH: errMessage = "EGL_BAD_MATCH"; break;
        case EGL_BAD_CONFIG: errMessage = "EGL_BAD_CONFIG"; break;
        case EGL_BAD_NATIVE_WINDOW: errMessage = "EGL_BAD_NATIVE_WINDOW"; break;
        case EGL_BAD_ALLOC: errMessage = "EGL_BAD_ALLOC"; break;
        }
        qOhosReportFatalErrorAndAbort(
            "Failed to create surface with error: %d - %s",
            eglErrorCode,
            errMessage.c_str());
    }

    bool preserveBufOnSwapRequested = surfaceFlags.testFlag(
            SurfaceFlagBits::PreserveBufferContentsOnSwap);
    if (preserveBufOnSwapRequested
            && eglSurfaceAttrib(display, surface, EGL_SWAP_BEHAVIOR, EGL_BUFFER_PRESERVED) == EGL_FALSE) {
        qOhosReportFatalErrorAndAbort("Failed to set EGL_BUFFER_PRESERVED surface attrib");
    }

    m_ownEglSurface = surface;
    m_refCurrentConfig = config;
    m_refCurrentDisplay = display;
    m_refCurrentNativeWindow = m_refTargetNativeWindow;
    m_currentSurfaceFlags = surfaceFlags;
}

QOhosOptional<QSize> QOhosEGLSurface::currentSurfaceSize() const
{
    if (m_refCurrentDisplay == EGL_NO_DISPLAY || m_ownEglSurface == EGL_NO_SURFACE)
        return makeEmptyQOhosOptional();

    EGLint width;
    if (eglQuerySurface(m_refCurrentDisplay, m_ownEglSurface, EGL_WIDTH, &width) != EGL_TRUE) {
        qOhosPrintfWarning("%s - cannot get surface width, error: %d", Q_FUNC_INFO, eglGetError());
        return makeEmptyQOhosOptional();
    }

    EGLint height;
    if (eglQuerySurface(m_refCurrentDisplay, m_ownEglSurface, EGL_HEIGHT, &height) != EGL_TRUE) {
        qOhosPrintfWarning("%s - cannot get surface height, error: %d", Q_FUNC_INFO, eglGetError());
        return makeEmptyQOhosOptional();
    }

    return makeQOhosOptional(QSize(width, height));
}

EGLSurface QOhosEGLSurface::tryGetOrCreateEGLWindowSurface(
    EGLDisplay display, EGLConfig config, bool swappingBuffers, SurfaceFlags surfaceFlags)
{
    if (swappingBuffers)
       return m_ownEglSurface;

    bool needsRebuild = m_ownEglSurface == EGL_NO_SURFACE
        || m_refCurrentNativeWindow != m_refTargetNativeWindow
        || m_refCurrentDisplay != display
        || m_refCurrentConfig != config
        || m_currentSurfaceFlags != surfaceFlags
        || (m_targetSurfaceSize.hasValue() && m_targetSurfaceSize != currentSurfaceSize());

    if (!needsRebuild)
        return m_ownEglSurface;

    cleanup();
    tryCreateSurface(display, config, surfaceFlags);

    return m_ownEglSurface;
}

void QOhosEGLSurface::cleanup()
{
    if (m_ownEglSurface != EGL_NO_SURFACE && eglDestroySurface(m_refCurrentDisplay, m_ownEglSurface) == EGL_FALSE)
        qOhosPrintfDebug("Failed to destroy surface");

    m_refCurrentDisplay = EGL_NO_DISPLAY;
    m_refCurrentConfig = nullptr;
    m_refCurrentNativeWindow = nullptr;
    m_ownEglSurface = EGL_NO_SURFACE;
    m_currentSurfaceFlags = {};
    m_targetSurfaceSize = makeEmptyQOhosOptional();
}

QOhosEGLSurface::~QOhosEGLSurface()
{
    cleanup();
}

QOhosEGLSurface::SurfaceFlags QOhosEGLSurface::currentSurfaceFlags() const
{
    return m_currentSurfaceFlags;
}

QT_END_NAMESPACE
