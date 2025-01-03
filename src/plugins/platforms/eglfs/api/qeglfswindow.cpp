// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/qtextstream.h>
#include <qpa/qwindowsysteminterface.h>
#include <qpa/qplatformintegration.h>
#include <private/qguiapplication_p.h>
#include <private/qwindow_p.h>
#ifndef QT_NO_OPENGL
# include <QtGui/private/qopenglcontext_p.h>
# include <QtGui/QOpenGLContext>
# include <QtOpenGL/private/qopenglcompositorbackingstore_p.h>
#endif
#include <QtGui/private/qeglconvenience_p.h>

#include "qeglfswindow_p.h"
#ifndef QT_NO_OPENGL
# include "qeglfscursor_p.h"
#endif
#include "qeglfshooks_p.h"
#include "qeglfsdeviceintegration_p.h"

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(qLcEglDevDebug)

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
        if (Q_UNLIKELY(isRaster() != (compositor->targetWindow() != nullptr))) {
#  ifndef Q_OS_ANDROID
            // We can have either a single OpenGL window or multiple raster windows.
            // Other combinations cannot work.
            qFatal("EGLFS: OpenGL windows cannot be mixed with others.");
#  endif
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
    compositor->setTargetWindow(window(), screen->rawGeometry());
    compositor->setRotation(qEnvironmentVariableIntValue("QT_QPA_EGLFS_ROTATION"));
#endif
}

void QEglFSWindow::setBackingStore(QOpenGLCompositorBackingStore *backingStore)
{
#ifndef QT_NO_OPENGL
    if (!m_rasterCompositingContext) {
        m_rasterCompositingContext = new QOpenGLContext;
        m_rasterCompositingContext->setShareContext(qt_gl_global_share_context());
        m_rasterCompositingContext->setFormat(m_format);
        m_rasterCompositingContext->setScreen(window()->screen());
        if (Q_UNLIKELY(!m_rasterCompositingContext->create()))
            qFatal("EGLFS: Failed to create compositing context");
        // If there is a "root" window into which raster and QOpenGLWidget content is
        // composited, all other contexts must share with its context.
        if (!qt_gl_global_share_context())
            qt_gl_set_global_share_context(m_rasterCompositingContext);
    }
    QOpenGLCompositor *compositor = QOpenGLCompositor::instance();
    compositor->setTargetContext(m_rasterCompositingContext);
#endif
    m_backingStore = backingStore;
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
        if (qt_gl_global_share_context() == m_rasterCompositingContext)
            qt_gl_set_global_share_context(nullptr);
        delete m_rasterCompositingContext;
#endif
    }

    m_flags = { };
}

void QEglFSWindow::invalidateSurface()
{
    if (m_surface != EGL_NO_SURFACE) {
        qCDebug(qLcEglDevDebug) << Q_FUNC_INFO << " about to destroy EGLSurface: " << m_surface;

        bool ok = eglDestroySurface(screen()->display(), m_surface);

        if (!ok) {
            qCWarning(qLcEglDevDebug, "QEglFSWindow::invalidateSurface() eglDestroySurface failed!"
                                      " Follow-up errors or memory leaks are possible."
                                      " eglGetError(): %x", eglGetError());
        }

        if (eglGetCurrentSurface(EGL_READ) == m_surface ||
                eglGetCurrentSurface(EGL_DRAW) == m_surface) {
            bool ok = eglMakeCurrent(eglGetCurrentDisplay(), EGL_NO_DISPLAY, EGL_NO_DISPLAY, EGL_NO_CONTEXT);
            qCDebug(qLcEglDevDebug) << Q_FUNC_INFO << " due to eglDestroySurface on *currently* bound surface"
                                    << "we just called eglMakeCurrent(..,0,0,0)! It returned: " << ok;
        }

        if (screen()->primarySurface() == m_surface)
            screen()->setPrimarySurface(EGL_NO_SURFACE);


        m_surface = EGL_NO_SURFACE;
        m_flags = m_flags & ~Created;
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
    QWindowSystemInterface::handleWindowActivated(wnd, Qt::ActiveWindowFocusReason);
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
    if (window()->type() != Qt::Desktop && windows.size() > 1) {
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
    if (!isRaster() && !backingStore())
        qWarning("QEglFSWindow: Cannot set opacity for non-raster windows");

    // Nothing to do here. The opacity is stored in the QWindow.
}

QT_END_NAMESPACE
