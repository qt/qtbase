/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/qtextstream.h>
#include <QtGui/qwindow.h>
#include <qpa/qwindowsysteminterface.h>
#include <qpa/qplatformcursor.h>
#include <QtPlatformSupport/private/qopenglcompositor_p.h>

#include "qeglfsscreen.h"
#include "qeglfswindow.h"
#include "qeglfshooks.h"

QT_BEGIN_NAMESPACE

QEglFSScreen::QEglFSScreen(EGLDisplay dpy)
    : m_dpy(dpy),
      m_surface(EGL_NO_SURFACE),
      m_cursor(0)
{
    m_cursor = qt_egl_device_integration()->createCursor(this);
}

QEglFSScreen::~QEglFSScreen()
{
    delete m_cursor;
    QOpenGLCompositor::destroy();
}

QRect QEglFSScreen::geometry() const
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

qreal QEglFSScreen::pixelDensity() const
{
    return qt_egl_device_integration()->pixelDensity();
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
    const QOpenGLCompositor *compositor = QOpenGLCompositor::instance();
    const QList<QOpenGLCompositorWindow *> windows = compositor->windows();

    // Generate enter and leave events like a real windowing system would do.
    if (windows.isEmpty())
        return;

    // First window is always fullscreen.
    if (windows.count() == 1) {
        QWindow *window = windows[0]->sourceWindow();
        if (m_pointerWindow != window) {
            m_pointerWindow = window;
            QWindowSystemInterface::handleEnterEvent(window, window->mapFromGlobal(pos), pos);
        }
        return;
    }

    QWindow *enter = 0, *leave = 0;
    for (int i = windows.count() - 1; i >= 0; --i) {
        QWindow *window = windows[i]->sourceWindow();
        const QRect geom = window->geometry();
        if (geom.contains(pos)) {
            if (m_pointerWindow != window) {
                leave = m_pointerWindow;
                m_pointerWindow = window;
                enter = window;
            }
            break;
        }
    }

    if (enter && leave)
        QWindowSystemInterface::handleEnterLeaveEvent(enter, leave, enter->mapFromGlobal(pos), pos);
}

QPixmap QEglFSScreen::grabWindow(WId wid, int x, int y, int width, int height) const
{
    QOpenGLCompositor *compositor = QOpenGLCompositor::instance();
    const QList<QOpenGLCompositorWindow *> windows = compositor->windows();
    Q_ASSERT(!windows.isEmpty());

    QImage img;

    if (static_cast<QEglFSWindow *>(windows.first()->sourceWindow()->handle())->isRaster()) {
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

    foreach (QOpenGLCompositorWindow *w, windows) {
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

    return QPixmap();
}

QT_END_NAMESPACE
