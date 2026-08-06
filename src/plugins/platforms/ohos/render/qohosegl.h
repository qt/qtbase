// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSEGL_H
#define QOHOSEGL_H

#include "EGL/egl.h"
#include "EGL/eglplatform.h"
#include <GLES3/gl3.h>
#include <QtCore/qsharedpointer.h>
#include <QtCore/qglobal.h>
#include <QtCore/qsize.h>
#include <optional>

QT_BEGIN_NAMESPACE

class QOhosEGLSurface
{
public:
    enum class SurfaceFlagBits {
        PreserveBufferContentsOnSwap = 1 << 0,
    };
    Q_DECLARE_FLAGS(SurfaceFlags, SurfaceFlagBits)

    QOhosEGLSurface() = default;

    QOhosEGLSurface(const QOhosEGLSurface&) = delete;
    QOhosEGLSurface& operator=(const QOhosEGLSurface&) = delete;

    QOhosEGLSurface(QOhosEGLSurface &&) = delete;
    QOhosEGLSurface& operator=(QOhosEGLSurface &&) = delete;

    ~QOhosEGLSurface();

    EGLSurface tryGetOrCreateEGLWindowSurface(
        EGLDisplay display, EGLConfig config, bool swappingBuffers, SurfaceFlags flags = {});
    void setNativeWindowSurface(
        EGLNativeWindowType nativeWindow, const std::optional<QSize> &optSurfaceSize);
    SurfaceFlags currentSurfaceFlags() const;

private:
    void cleanup();
    void tryCreateSurface(EGLDisplay display, EGLConfig config, SurfaceFlags surfaceFlags);
    std::optional<QSize> currentSurfaceSize() const;

    EGLDisplay m_refCurrentDisplay = EGL_NO_DISPLAY;
    EGLConfig m_refCurrentConfig = nullptr;
    EGLNativeWindowType m_refCurrentNativeWindow = nullptr;
    EGLNativeWindowType m_refTargetNativeWindow = nullptr;
    EGLSurface m_ownEglSurface = EGL_NO_SURFACE;
    SurfaceFlags m_currentSurfaceFlags = {};
    std::optional<QSize> m_targetSurfaceSize;
};

QT_END_NAMESPACE

#endif // QOHOSEGL_H
