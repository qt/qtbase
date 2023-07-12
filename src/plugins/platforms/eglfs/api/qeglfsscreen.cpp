// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/qtextstream.h>
#include <QtGui/qwindow.h>
#include <QtGui/private/qguiapplication_p.h>
#include <qpa/qwindowsysteminterface.h>
#include <qpa/qplatformcursor.h>
#ifndef QT_NO_OPENGL
# include <QtOpenGL/private/qopenglcompositor_p.h>
#endif

#include "qeglfsscreen_p.h"
#include "qeglfswindow_p.h"
#include "qeglfshooks_p.h"

QT_BEGIN_NAMESPACE

QEglFSScreen::QEglFSScreen(EGLDisplay dpy)
    : m_dpy(dpy),
      m_surface(EGL_NO_SURFACE),
      m_cursor(nullptr)
{
    m_cursor = qt_egl_device_integration()->createCursor(this);
}

QEglFSScreen::~QEglFSScreen()
{
    delete m_cursor;
}

QRect QEglFSScreen::geometry() const
{
    QRect r = rawGeometry();

    static int rotation = qEnvironmentVariableIntValue("QT_QPA_EGLFS_ROTATION");
    switch (rotation) {
    case 0:
    case 180:
    case -180:
        break;
    case 90:
    case -90: {
        int h = r.height();
        r.setHeight(r.width());
        r.setWidth(h);
        break;
    }
    default:
        qWarning("Invalid rotation %d specified in QT_QPA_EGLFS_ROTATION", rotation);
        break;
    }

    return r;
}

QRect QEglFSScreen::rawGeometry() const
{
    return QRect(QPoint(0, 0), qt_egl_device_integration()->screenSize());
}

int QEglFSScreen::depth() const
{
    return qt_egl_device_integration()->screenDepth();
}

QImage::Format QEglFSScreen::format() const
{
    return qt_egl_device_integration()->screenFormat();
}

QSizeF QEglFSScreen::physicalSize() const
{
    return qt_egl_device_integration()->physicalScreenSize();
}

QDpi QEglFSScreen::logicalDpi() const
{
    return qt_egl_device_integration()->logicalDpi();
}

QDpi QEglFSScreen::logicalBaseDpi() const
{
    return qt_egl_device_integration()->logicalBaseDpi();
}

Qt::ScreenOrientation QEglFSScreen::nativeOrientation() const
{
    return qt_egl_device_integration()->nativeOrientation();
}

Qt::ScreenOrientation QEglFSScreen::orientation() const
{
    return qt_egl_device_integration()->orientation();
}

QPlatformCursor *QEglFSScreen::cursor() const
{
    return m_cursor;
}

qreal QEglFSScreen::refreshRate() const
{
    return qt_egl_device_integration()->refreshRate();
}

void QEglFSScreen::setPrimarySurface(EGLSurface surface)
{
    m_surface = surface;
}

void QEglFSScreen::handleCursorMove(const QPoint &pos)
{
#ifndef QT_NO_OPENGL
    const QOpenGLCompositor *compositor = QOpenGLCompositor::instance();
    const QList<QOpenGLCompositorWindow *> windows = compositor->windows();
    QEglFSIntegration *platformIntegration = static_cast<QEglFSIntegration *>(QGuiApplicationPrivate::platformIntegration());

    // Generate enter and leave events like a real windowing system would do.
    if (windows.isEmpty())
        return;

    // First window is always fullscreen.
    if (windows.size() == 1) {
        QWindow *window = windows[0]->sourceWindow();
        if (platformIntegration->pointerWindow() != window) {
            platformIntegration->setPointerWindow(window);
            QWindowSystemInterface::handleEnterEvent(window, window->mapFromGlobal(pos), pos);
        }
        return;
    }

    QWindow *enter = nullptr, *leave = nullptr;
    for (int i = windows.size() - 1; i >= 0; --i) {
        QWindow *window = windows[i]->sourceWindow();
        const QRect geom = window->geometry();
        if (geom.contains(pos)) {
            if (platformIntegration->pointerWindow() != window) {
                leave = platformIntegration->pointerWindow();
                platformIntegration->setPointerWindow(window);
                enter = window;
            }
            break;
        }
    }

    if (enter && leave) {
        QWindowSystemInterface::handleEnterLeaveEvent(enter, leave, enter->mapFromGlobal(pos), pos);
    } else if (enter) {
        QWindowSystemInterface::handleEnterEvent(enter, enter->mapFromGlobal(pos), pos);
    } else if (leave) {
        QWindowSystemInterface::handleLeaveEvent(leave);
    }
#else
    Q_UNUSED(pos);
#endif
}

QPixmap QEglFSScreen::grabWindow(WId wid, int x, int y, int width, int height) const
{
#ifndef QT_NO_OPENGL
    QOpenGLCompositor *compositor = QOpenGLCompositor::instance();
    const QList<QOpenGLCompositorWindow *> windows = compositor->windows();
    Q_ASSERT(!windows.isEmpty());

    QImage img;

    QEglFSWindow *primaryWin = static_cast<QEglFSWindow *>(windows.first()->sourceWindow()->handle());
    if (primaryWin->isRaster() || primaryWin->backingStore()) {
        // Request the compositor to render everything into an FBO and read it back. This
        // is of course slow, but it's safe and reliable. It will not include the mouse
        // cursor, which is a plus.
        img = compositor->grab();
    } else {
        // Just a single OpenGL window without compositing. Do not support this case for now. Doing
        // glReadPixels is not an option since it would read from the back buffer which may have
        // undefined content when calling right after a swapBuffers (unless preserved swap is
        // available and enabled, but we have no support for that).
        qWarning("grabWindow: Not supported for non-composited OpenGL content. Use QQuickWindow::grabWindow() instead.");
        return QPixmap();
    }

    if (!wid) {
        const QSize screenSize = geometry().size();
        if (width < 0)
            width = screenSize.width() - x;
        if (height < 0)
            height = screenSize.height() - y;
        return QPixmap::fromImage(img).copy(x, y, width, height);
    }

    for (QOpenGLCompositorWindow *w : windows) {
        const QWindow *window = w->sourceWindow();
        if (window->winId() == wid) {
            const QRect geom = window->geometry();
            if (width < 0)
                width = geom.width() - x;
            if (height < 0)
                height = geom.height() - y;
            QRect rect(geom.topLeft() + QPoint(x, y), QSize(width, height));
            rect &= window->geometry();
            return QPixmap::fromImage(img).copy(rect);
        }
    }
#else // QT_NO_OPENGL
    Q_UNUSED(wid);
    Q_UNUSED(x);
    Q_UNUSED(y);
    Q_UNUSED(width);
    Q_UNUSED(height);
#endif
    return QPixmap();
}

QWindow *QEglFSScreen::topLevelAt(const QPoint &point) const
{
#ifndef QT_NO_OPENGL
    QOpenGLCompositor *compositor = QOpenGLCompositor::instance();
    const QList<QOpenGLCompositorWindow *> windows = compositor->windows();
    const int windowCount = windows.size();

    // Higher z-order is at the end of the list
    for (int i = windowCount - 1; i >= 0; i--) {
        QWindow *window = windows[i]->sourceWindow();
        if (window->isVisible() && window->geometry().contains(point))
            return window;
    }
#endif

    return QPlatformScreen::topLevelAt(point);
}

QT_END_NAMESPACE
