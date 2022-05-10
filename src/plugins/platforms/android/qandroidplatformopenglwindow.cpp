// Copyright (C) 2014 BogDan Vatra <bogdan@kde.org>
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qandroidplatformopenglwindow.h"

#include "androiddeadlockprotector.h"
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
{
}

QAndroidPlatformOpenGLWindow::~QAndroidPlatformOpenGLWindow()
{
    m_surfaceWaitCondition.wakeOne();
    lockSurface();
    if (m_nativeSurfaceId != -1)
        QtAndroid::destroySurface(m_nativeSurfaceId);
    clearEgl();
    unlockSurface();
}

void QAndroidPlatformOpenGLWindow::repaint(const QRegion &region)
{
    // This is only for real raster top-level windows. Stop in all other cases.
    if ((window()->surfaceType() == QSurface::RasterGLSurface && qt_window_private(window())->compositing)
        || window()->surfaceType() == QSurface::OpenGLSurface
        || QAndroidPlatformWindow::parent())
        return;

    QRect currentGeometry = geometry();

    QRect dirtyClient = region.boundingRect();
    QRect dirtyRegion(currentGeometry.left() + dirtyClient.left(),
                      currentGeometry.top() + dirtyClient.top(),
                      dirtyClient.width(),
                      dirtyClient.height());
    QRect mOldGeometryLocal = m_oldGeometry;
    m_oldGeometry = currentGeometry;
    // If this is a move, redraw the previous location
    if (mOldGeometryLocal != currentGeometry)
        platformScreen()->setDirty(mOldGeometryLocal);
    platformScreen()->setDirty(dirtyRegion);
}

void QAndroidPlatformOpenGLWindow::setGeometry(const QRect &rect)
{
    if (rect == geometry())
        return;

    m_oldGeometry = geometry();

    QAndroidPlatformWindow::setGeometry(rect);
    if (m_nativeSurfaceId != -1)
        QtAndroid::setSurfaceGeometry(m_nativeSurfaceId, rect);

    QRect availableGeometry = screen()->availableGeometry();
    if (rect.width() > 0
            && rect.height() > 0
            && availableGeometry.width() > 0
            && availableGeometry.height() > 0) {
        QWindowSystemInterface::handleExposeEvent(window(), QRect(QPoint(0, 0), rect.size()));
    }

    if (rect.topLeft() != m_oldGeometry.topLeft())
        repaint(QRegion(rect));
}

EGLSurface QAndroidPlatformOpenGLWindow::eglSurface(EGLConfig config)
{
    if (QAndroidEventDispatcherStopper::stopped() || QGuiApplication::applicationState() == Qt::ApplicationSuspended)
        return m_eglSurface;

    QMutexLocker lock(&m_surfaceMutex);

    if (m_nativeSurfaceId == -1) {
        AndroidDeadlockProtector protector;
        if (!protector.acquire())
            return m_eglSurface;

        const bool windowStaysOnTop = bool(window()->flags() & Qt::WindowStaysOnTopHint);
        m_nativeSurfaceId = QtAndroid::createSurface(this, geometry(), windowStaysOnTop, 32);
        m_surfaceWaitCondition.wait(&m_surfaceMutex);
    }

    if (m_eglSurface == EGL_NO_SURFACE) {
        m_surfaceMutex.unlock();
        checkNativeSurface(config);
        m_surfaceMutex.lock();
    }
    return m_eglSurface;
}

bool QAndroidPlatformOpenGLWindow::checkNativeSurface(EGLConfig config)
{
    QMutexLocker lock(&m_surfaceMutex);
    if (m_nativeSurfaceId == -1 || !m_androidSurfaceObject.isValid())
        return false; // makeCurrent is NOT needed.

    createEgl(config);

    // we've create another surface, the window should be repainted
    QRect availableGeometry = screen()->availableGeometry();
    if (geometry().width() > 0 && geometry().height() > 0 && availableGeometry.width() > 0 && availableGeometry.height() > 0)
        QWindowSystemInterface::handleExposeEvent(window(), QRegion(QRect(QPoint(), geometry().size())));
    return true; // makeCurrent is needed!
}

void QAndroidPlatformOpenGLWindow::applicationStateChanged(Qt::ApplicationState state)
{
    QAndroidPlatformWindow::applicationStateChanged(state);
    if (state <=  Qt::ApplicationHidden) {
        lockSurface();
        if (m_nativeSurfaceId != -1) {
            QtAndroid::destroySurface(m_nativeSurfaceId);
            m_nativeSurfaceId = -1;
        }
        clearEgl();
        unlockSurface();
    }
}

void QAndroidPlatformOpenGLWindow::createEgl(EGLConfig config)
{
    clearEgl();
    QJniEnvironment env;
    m_nativeWindow = ANativeWindow_fromSurface(env.jniEnv(), m_androidSurfaceObject.object());
    m_androidSurfaceObject = QJniObject();
    m_eglSurface = eglCreateWindowSurface(m_eglDisplay, config, m_nativeWindow, NULL);
    m_format = q_glFormatFromConfig(m_eglDisplay, config, window()->requestedFormat());
    if (Q_UNLIKELY(m_eglSurface == EGL_NO_SURFACE)) {
        EGLint error = eglGetError();
        eglTerminate(m_eglDisplay);
        qFatal("EGL Error : Could not create the egl surface: error = 0x%x\n", error);
    }
}

QSurfaceFormat QAndroidPlatformOpenGLWindow::format() const
{
    if (m_nativeWindow == 0)
        return window()->requestedFormat();

    return m_format;
}

void QAndroidPlatformOpenGLWindow::clearEgl()
{
    if (m_eglSurface != EGL_NO_SURFACE) {
        eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglDestroySurface(m_eglDisplay, m_eglSurface);
        m_eglSurface = EGL_NO_SURFACE;
    }

    if (m_nativeWindow) {
        ANativeWindow_release(m_nativeWindow);
        m_nativeWindow = 0;
    }
}

void QAndroidPlatformOpenGLWindow::surfaceChanged(JNIEnv *jniEnv, jobject surface, int w, int h)
{
    Q_UNUSED(jniEnv);
    Q_UNUSED(w);
    Q_UNUSED(h);

    lockSurface();
    m_androidSurfaceObject = surface;
    if (surface) // wait until we have a valid surface to draw into
        m_surfaceWaitCondition.wakeOne();
    unlockSurface();

    if (surface) {
        // repaint the window, when we have a valid surface
        QRect availableGeometry = screen()->availableGeometry();
        if (geometry().width() > 0 && geometry().height() > 0 && availableGeometry.width() > 0 && availableGeometry.height() > 0)
            QWindowSystemInterface::handleExposeEvent(window(), QRegion(QRect(QPoint(), geometry().size())));
    }
}

QT_END_NAMESPACE
