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

#include <QtCore/qtextstream.h>
#include <qpa/qwindowsysteminterface.h>
#include <qpa/qplatformintegration.h>
#include <private/qguiapplication_p.h>
#include <private/qwindow_p.h>
#ifndef QT_NO_OPENGL
# include <QtGui/private/qopenglcontext_p.h>
# include <QtGui/QOpenGLContext>
# include <QtPlatformCompositorSupport/private/qopenglcompositorbackingstore_p.h>
#endif
#include <QtEglSupport/private/qeglconvenience_p.h>

#include "qeglfswindow_p.h"
#ifndef QT_NO_OPENGL
# include "qeglfscursor_p.h"
#endif
#include "qeglfshooks_p.h"
#include "qeglfsdeviceintegration_p.h"

QT_BEGIN_NAMESPACE

QEglFSWindow::QEglFSWindow(QWindow *w)
    : QPlatformWindow(w),
#ifndef QT_NO_OPENGL
      m_backingStore(nullptr),
      m_rasterCompositingContext(nullptr),
#endif
      m_winId(0),
      m_surface(EGL_NO_SURFACE),
      m_window(0)
{
}

QEglFSWindow::~QEglFSWindow()
{
    destroy();
}

static WId newWId()
{
    static WId id = 0;

    if (id == std::numeric_limits<WId>::max())
        qWarning("QEGLPlatformWindow: Out of window IDs");

    return ++id;
}

void QEglFSWindow::create()
{
    if (m_flags.testFlag(Created))
        return;

    m_winId = newWId();

    if (window()->type() == Qt::Desktop) {
        QRect fullscreenRect(QPoint(), screen()->availableGeometry().size());
        QWindowSystemInterface::handleGeometryChange(window(), fullscreenRect);
        return;
    }

    m_flags = Created;

    if (window()->type() == Qt::Desktop)
        return;

    // Stop if there is already a window backed by a native window and surface. Additional
    // raster windows will not have their own native window, surface and context. Instead,
    // they will be composited onto the root window's surface.
    QEglFSScreen *screen = this->screen();
#ifndef QT_NO_OPENGL
    QOpenGLCompositor *compositor = QOpenGLCompositor::instance();
    if (screen->primarySurface() != EGL_NO_SURFACE) {
        if (Q_UNLIKELY(!isRaster() || !compositor->targetWindow())) {
#if !defined(Q_OS_ANDROID) || defined(Q_OS_ANDROID_EMBEDDED)
            // We can have either a single OpenGL window or multiple raster windows.
            // Other combinations cannot work.
            qFatal("EGLFS: OpenGL windows cannot be mixed with others.");
#endif
            return;
        }
        m_format = compositor->targetWindow()->format();
        return;
    }
#endif // QT_NO_OPENGL

    m_flags |= HasNativeWindow;
    setGeometry(QRect()); // will become fullscreen

    resetSurface();

    if (Q_UNLIKELY(m_surface == EGL_NO_SURFACE)) {
        EGLint error = eglGetError();
        eglTerminate(screen->display());
        qFatal("EGL Error : Could not create the egl surface: error = 0x%x\n", error);
    }

    screen->setPrimarySurface(m_surface);

#ifndef QT_NO_OPENGL
    if (isRaster()) {
        m_rasterCompositingContext = new QOpenGLContext;
        m_rasterCompositingContext->setShareContext(qt_gl_global_share_context());
        m_rasterCompositingContext->setFormat(m_format);
        m_rasterCompositingContext->setScreen(window()->screen());
        if (Q_UNLIKELY(!m_rasterCompositingContext->create()))
            qFatal("EGLFS: Failed to create compositing context");
        compositor->setTarget(m_rasterCompositingContext, window(), screen->rawGeometry());
        compositor->setRotation(qEnvironmentVariableIntValue("QT_QPA_EGLFS_ROTATION"));
        // If there is a "root" window into which raster and QOpenGLWidget content is
        // composited, all other contexts must share with its context.
        if (!qt_gl_global_share_context())
            qt_gl_set_global_share_context(m_rasterCompositingContext);
    }
#endif // QT_NO_OPENGL
}

void QEglFSWindow::destroy()
{
    if (!m_flags.testFlag(Created))
        return; // already destroyed

#ifndef QT_NO_OPENGL
    QOpenGLCompositor::instance()->removeWindow(this);
#endif

    QEglFSScreen *screen = this->screen();
    if (m_flags.testFlag(HasNativeWindow)) {
#ifndef QT_NO_OPENGL
        QEglFSCursor *cursor = qobject_cast<QEglFSCursor *>(screen->cursor());
        if (cursor)
            cursor->resetResources();
#endif
        if (screen->primarySurface() == m_surface)
            screen->setPrimarySurface(EGL_NO_SURFACE);

        invalidateSurface();

#ifndef QT_NO_OPENGL
        QOpenGLCompositor::destroy();
        delete m_rasterCompositingContext;
#endif
    }

    m_flags = { };
}

void QEglFSWindow::invalidateSurface()
{
    if (m_surface != EGL_NO_SURFACE) {
        eglDestroySurface(screen()->display(), m_surface);
        m_surface = EGL_NO_SURFACE;
    }
    qt_egl_device_integration()->destroyNativeWindow(m_window);
    m_window = 0;
}

void QEglFSWindow::resetSurface()
{
    EGLDisplay display = screen()->display();
    QSurfaceFormat platformFormat = qt_egl_device_integration()->surfaceFormatFor(window()->requestedFormat());

    m_config = QEglFSDeviceIntegration::chooseConfig(display, platformFormat);
    m_format = q_glFormatFromConfig(display, m_config, platformFormat);
    const QSize surfaceSize = screen()->rawGeometry().size();
    m_window = qt_egl_device_integration()->createNativeWindow(this, surfaceSize, m_format);
    m_surface = eglCreateWindowSurface(display, m_config, m_window, nullptr);
}

void QEglFSWindow::setVisible(bool visible)
{
#ifndef QT_NO_OPENGL
    QOpenGLCompositor *compositor = QOpenGLCompositor::instance();
    QList<QOpenGLCompositorWindow *> windows = compositor->windows();
    QWindow *wnd = window();

    if (wnd->type() != Qt::Desktop) {
        if (visible) {
            compositor->addWindow(this);
        } else {
            compositor->removeWindow(this);
            windows = compositor->windows();
            if (windows.size())
                windows.last()->sourceWindow()->requestActivate();
        }
    }
#else
    QWindow *wnd = window();
#endif
    QWindowSystemInterface::handleExposeEvent(wnd, QRect(QPoint(0, 0), wnd->geometry().size()));

    if (visible)
        QWindowSystemInterface::flushWindowSystemEvents(QEventLoop::ExcludeUserInputEvents);
}

void QEglFSWindow::setGeometry(const QRect &r)
{
    QRect rect = r;
    if (m_flags.testFlag(HasNativeWindow))
        rect = screen()->availableGeometry();

    QPlatformWindow::setGeometry(rect);

    QWindowSystemInterface::handleGeometryChange(window(), rect);

    const QRect lastReportedGeometry = qt_window_private(window())->geometry;
    if (rect != lastReportedGeometry)
        QWindowSystemInterface::handleExposeEvent(window(), QRect(QPoint(0, 0), rect.size()));
}

QRect QEglFSWindow::geometry() const
{
    // For yet-to-become-fullscreen windows report the geometry covering the entire
    // screen. This is particularly important for Quick where the root object may get
    // sized to some geometry queried before calling create().
    if (!m_flags.testFlag(Created) && screen()->primarySurface() == EGL_NO_SURFACE)
        return screen()->availableGeometry();

    return QPlatformWindow::geometry();
}

void QEglFSWindow::requestActivateWindow()
{
#ifndef QT_NO_OPENGL
    if (window()->type() != Qt::Desktop)
        QOpenGLCompositor::instance()->moveToTop(this);
#endif
    QWindow *wnd = window();
    QWindowSystemInterface::handleWindowActivated(wnd);
    QWindowSystemInterface::handleExposeEvent(wnd, QRect(QPoint(0, 0), wnd->geometry().size()));
}

void QEglFSWindow::raise()
{
    QWindow *wnd = window();
    if (wnd->type() != Qt::Desktop) {
#ifndef QT_NO_OPENGL
        QOpenGLCompositor::instance()->moveToTop(this);
#endif
        QWindowSystemInterface::handleExposeEvent(wnd, QRect(QPoint(0, 0), wnd->geometry().size()));
    }
}

void QEglFSWindow::lower()
{
#ifndef QT_NO_OPENGL
    QOpenGLCompositor *compositor = QOpenGLCompositor::instance();
    QList<QOpenGLCompositorWindow *> windows = compositor->windows();
    if (window()->type() != Qt::Desktop && windows.count() > 1) {
        int idx = windows.indexOf(this);
        if (idx > 0) {
            compositor->changeWindowIndex(this, idx - 1);
            QWindowSystemInterface::handleExposeEvent(windows.last()->sourceWindow(),
                                                      QRect(QPoint(0, 0), windows.last()->sourceWindow()->geometry().size()));
        }
    }
#endif
}

EGLSurface QEglFSWindow::surface() const
{
    return m_surface != EGL_NO_SURFACE ? m_surface : screen()->primarySurface();
}

QSurfaceFormat QEglFSWindow::format() const
{
    return m_format;
}

EGLNativeWindowType QEglFSWindow::eglWindow() const
{
    return m_window;
}

QEglFSScreen *QEglFSWindow::screen() const
{
    return static_cast<QEglFSScreen *>(QPlatformWindow::screen());
}

bool QEglFSWindow::isRaster() const
{
    const QWindow::SurfaceType type = window()->surfaceType();
    return type == QSurface::RasterSurface || type == QSurface::RasterGLSurface;
}

#ifndef QT_NO_OPENGL
QWindow *QEglFSWindow::sourceWindow() const
{
    return window();
}

const QPlatformTextureList *QEglFSWindow::textures() const
{
    if (m_backingStore)
        return m_backingStore->textures();

    return nullptr;
}

void QEglFSWindow::endCompositing()
{
    if (m_backingStore)
        m_backingStore->notifyComposited();
}
#endif

WId QEglFSWindow::winId() const
{
    return m_winId;
}

void QEglFSWindow::setOpacity(qreal)
{
    if (!isRaster())
        qWarning("QEglFSWindow: Cannot set opacity for non-raster windows");

    // Nothing to do here. The opacity is stored in the QWindow.
}

QT_END_NAMESPACE
