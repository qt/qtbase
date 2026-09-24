// Copyright (C) 2014 BogDan Vatra <bogdan@kde.org>
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qandroidplatformopenglwindow.h"

#include "androidjnimain.h"
#include "qandroidplatformscreen.h"

#include <QSurfaceFormat>
#include <QtGui/private/qwindow_p.h>
#include <QtGui/qguiapplication.h>

#include <QtGui/private/qeglconvenience_p.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <qpa/qplatformscreen.h>
#include <qpa/qwindowsysteminterface.h>

QT_BEGIN_NAMESPACE

QAndroidPlatformOpenGLWindow::QAndroidPlatformOpenGLWindow(QWindow *window, EGLDisplay display)
    :QAndroidPlatformWindow(window), m_eglDisplay(display)
{ }

QAndroidPlatformOpenGLWindow::~QAndroidPlatformOpenGLWindow()
{
    // The base class destructor can't reach destroyWindowSurface().
    destroySurface();
}

void QAndroidPlatformOpenGLWindow::setGeometry(const QRect &rect)
{
    QAndroidPlatformWindow::setGeometry(rect);

    QRect availableGeometry = screen()->availableGeometry();
    if (rect.width() > 0
            && rect.height() > 0
            && availableGeometry.width() > 0
            && availableGeometry.height() > 0) {
        QWindowSystemInterface::handleExposeEvent(window(), QRect(QPoint(0, 0), rect.size()));
    }
}

EGLSurface QAndroidPlatformOpenGLWindow::eglSurface(EGLConfig config)
{
    if (Q_LIKELY(m_eglSurface != EGL_NO_SURFACE))
        return m_eglSurface;
    ensureNativeWindow();
    if (!m_nativeWindow)
        return EGL_NO_SURFACE;

    m_eglSurface = eglCreateWindowSurface(m_eglDisplay, config, m_nativeWindow, nullptr);
    if (Q_UNLIKELY(m_eglSurface == EGL_NO_SURFACE)) {
        qWarning("EGL Error: could not create egl surface: 0x%x", eglGetError());
        return EGL_NO_SURFACE;
    }
    m_format = q_glFormatFromConfig(m_eglDisplay, config, window()->requestedFormat());
    return m_eglSurface;
}

QSurfaceFormat QAndroidPlatformOpenGLWindow::format() const
{
    if (m_nativeWindow == 0)
        return window()->requestedFormat();

    return m_format;
}

void QAndroidPlatformOpenGLWindow::destroyWindowSurface()
{
    if (m_eglSurface != EGL_NO_SURFACE) {
        eglDestroySurface(m_eglDisplay, m_eglSurface);
        m_eglSurface = EGL_NO_SURFACE;
    }
}

QT_END_NAMESPACE
