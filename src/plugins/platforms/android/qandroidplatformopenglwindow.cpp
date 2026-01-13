// Copyright (C) 2014 BogDan Vatra <bogdan@kde.org>
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qandroidplatformopenglwindow.h"

#include "androidjnimain.h"
#include "qandroideventdispatcher.h"
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
    m_surfaceWaitCondition.wakeOne();
    lockSurface();
    destroySurface();
    clearSurface();
    unlockSurface();
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

// Called by QAndroidPlatformOpenGLContext::eglSurfaceForPlatformSurface(),
// surface is already locked when calling this
EGLSurface QAndroidPlatformOpenGLWindow::eglSurface(EGLConfig config)
{
    if (QAndroidEventDispatcherStopper::stopped() ||
        QGuiApplication::applicationState() == Qt::ApplicationSuspended) {
        qCDebug(lcQpaWindow) << "Application not active, return existing surface.";
        return m_eglSurface;
    }
    // If we haven't called createSurface() yet, call it and wait until Android has created
    // the Surface
    if (!m_androidSurfaceCreated) {
        static constexpr char funcName[] = "QAndroidPlatformOpenGLWindow::eglSurface()";
        QtAndroidPrivate::AndroidDeadlockProtector protector(funcName);
        if (!protector.acquire()) {
            qFatal("Failed to acquire deadlock protector for %s.", funcName);
            return m_eglSurface;
        }

        createSurface();
        qCDebug(lcQpaWindow) << "called createSurface(), waiting for Surface to be ready...";
        m_surfaceWaitCondition.wait(&m_surfaceMutex);
    }

    if (m_eglSurface == EGL_NO_SURFACE)
        ensureEglSurfaceCreated(config);

    return m_eglSurface;
}

void QAndroidPlatformOpenGLWindow::applicationStateChanged(Qt::ApplicationState state)
{
    QAndroidPlatformWindow::applicationStateChanged(state);
    if (state <=  Qt::ApplicationHidden) {
        lockSurface();
        destroySurface();
        clearSurface();
        unlockSurface();
    }
}

// m_surfaceMutex already locked, called only by eglSurface()
// and QAndroidPlatformOpenGLContext::swapBuffers().
bool QAndroidPlatformOpenGLWindow::ensureEglSurfaceCreated(EGLConfig config)
{
    // Either no surface created, or the m_eglSurface already wraps the active Surface,
    // so makeCurrent is NOT needed, and we should not create a new EGL surface.
    if (!m_androidSurfaceCreated || !m_androidSurfaceObject.isValid()) {
        qCDebug(lcQpaWindow) << "Skipping create egl on invalid or not yet created surface";
        return false;
    }

    clearSurface();
    m_nativeWindow = ANativeWindow_fromSurface(
        QJniEnvironment::getJniEnv(), m_androidSurfaceObject.object());
    m_androidSurfaceObject = QJniObject();
    m_eglSurface = eglCreateWindowSurface(m_eglDisplay, config, m_nativeWindow, NULL);
    m_format = q_glFormatFromConfig(m_eglDisplay, config, window()->requestedFormat());
    if (Q_UNLIKELY(m_eglSurface == EGL_NO_SURFACE)) {
        EGLint error = eglGetError();
        eglTerminate(m_eglDisplay);
        qFatal("EGL Error : Could not create the egl surface: error = 0x%x\n", error);
    }

    // we've created another Surface, the window should be repainted
    sendExpose();

    return true;
}

QSurfaceFormat QAndroidPlatformOpenGLWindow::format() const
{
    if (m_nativeWindow == 0)
        return window()->requestedFormat();

    return m_format;
}

void QAndroidPlatformOpenGLWindow::clearSurface()
{
    if (m_eglSurface != EGL_NO_SURFACE) {
        eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglDestroySurface(m_eglDisplay, m_eglSurface);
        m_eglSurface = EGL_NO_SURFACE;
    }

    if (m_nativeWindow) {
        ANativeWindow_release(m_nativeWindow);
        m_nativeWindow = nullptr;
    }
}

QT_END_NAMESPACE
