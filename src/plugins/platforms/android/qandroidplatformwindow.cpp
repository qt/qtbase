// Copyright (C) 2014 BogDan Vatra <bogdan@kde.org>
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qandroidplatformwindow.h"
#include "qandroidplatformopenglcontext.h"
#include "qandroidplatformscreen.h"

#include "androidjnimain.h"

#include <qguiapplication.h>
#include <qpa/qwindowsysteminterface.h>
#include <private/qhighdpiscaling_p.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcQpaWindow, "qt.qpa.window")

Q_CONSTINIT static QBasicAtomicInt winIdGenerator = Q_BASIC_ATOMIC_INITIALIZER(0);

QAndroidPlatformWindow::QAndroidPlatformWindow(QWindow *window)
    : QPlatformWindow(window), m_nativeQtWindow(QNativeInterface::QAndroidApplication::context()),
      m_androidSurfaceObject(nullptr)
{
    m_nativeViewId = m_nativeQtWindow.callMethod<jint>("getId");
    m_windowFlags = Qt::Widget;
    m_windowState = Qt::WindowNoState;
    // the surfaceType is overwritten in QAndroidPlatformOpenGLWindow ctor so let's save
    // the fact that it's a raster window for now
    m_isRaster = window->surfaceType() == QSurface::RasterSurface;
    m_windowId = winIdGenerator.fetchAndAddRelaxed(1) + 1;
    setWindowState(window->windowStates());

    // the following is in relation to the virtual geometry
    const bool forceMaximize = m_windowState & (Qt::WindowMaximized | Qt::WindowFullScreen);
    const QRect nativeScreenGeometry = platformScreen()->availableGeometry();
    if (forceMaximize) {
        setGeometry(nativeScreenGeometry);
    } else {
        const QRect requestedNativeGeometry = QHighDpi::toNativePixels(window->geometry(), window);
        const QRect availableDeviceIndependentGeometry = (window->parent())
                ? window->parent()->geometry()
                : QHighDpi::fromNativePixels(nativeScreenGeometry, window);
        // initialGeometry returns in native pixels
        const QRect finalNativeGeometry = QPlatformWindow::initialGeometry(
                window, requestedNativeGeometry, availableDeviceIndependentGeometry.width(),
                availableDeviceIndependentGeometry.height());
        if (requestedNativeGeometry != finalNativeGeometry)
            setGeometry(finalNativeGeometry);
    }
    platformScreen()->addWindow(this);
}

QAndroidPlatformWindow::~QAndroidPlatformWindow()
{
    platformScreen()->removeWindow(this);
}


void QAndroidPlatformWindow::lower()
{
    platformScreen()->lower(this);
}

void QAndroidPlatformWindow::raise()
{
    updateSystemUiVisibility();
    platformScreen()->raise(this);
}

QMargins QAndroidPlatformWindow::safeAreaMargins() const
{
    if ((m_windowState & Qt::WindowMaximized) && (window()->flags() & Qt::MaximizeUsingFullscreenGeometryHint)) {
        QRect availableGeometry = platformScreen()->availableGeometry();
        return QMargins(availableGeometry.left(), availableGeometry.top(),
                        availableGeometry.right(), availableGeometry.bottom());
    } else {
        return QPlatformWindow::safeAreaMargins();
    }
}

void QAndroidPlatformWindow::setGeometry(const QRect &rect)
{
    QPlatformWindow::setGeometry(rect);
    QWindowSystemInterface::handleGeometryChange(window(), rect);
}

void QAndroidPlatformWindow::setVisible(bool visible)
{
    if (visible) {
        updateSystemUiVisibility();
        if ((m_windowState & Qt::WindowFullScreen)
                || ((m_windowState & Qt::WindowMaximized) && (window()->flags() & Qt::MaximizeUsingFullscreenGeometryHint))) {
            setGeometry(platformScreen()->geometry());
        } else if (m_windowState & Qt::WindowMaximized) {
            setGeometry(platformScreen()->availableGeometry());
        }
        requestActivateWindow();
    } else if (window() == qGuiApp->focusWindow()) {
        platformScreen()->topVisibleWindowChanged();
    }

    QRect availableGeometry = screen()->availableGeometry();
    if (geometry().width() > 0 && geometry().height() > 0 && availableGeometry.width() > 0 && availableGeometry.height() > 0)
        QPlatformWindow::setVisible(visible);
}

void QAndroidPlatformWindow::setWindowState(Qt::WindowStates state)
{
    if (m_windowState == state)
        return;

    QPlatformWindow::setWindowState(state);
    m_windowState = state;

    if (window()->isVisible())
        updateSystemUiVisibility();
}

void QAndroidPlatformWindow::setWindowFlags(Qt::WindowFlags flags)
{
    if (m_windowFlags == flags)
        return;

    m_windowFlags = flags;
}

Qt::WindowFlags QAndroidPlatformWindow::windowFlags() const
{
    return m_windowFlags;
}

void QAndroidPlatformWindow::setParent(const QPlatformWindow *window)
{
    // even though we do not yet support child windows properly, any windows getting a parent
    // should be removed from screen's window stack which is only for top level windows,
    // and respectively any window becoming top level should go in there
    if (window) {
        platformScreen()->removeWindow(this);
    } else {
        platformScreen()->addWindow(this);
    }
}

QAndroidPlatformScreen *QAndroidPlatformWindow::platformScreen() const
{
    return static_cast<QAndroidPlatformScreen *>(window()->screen()->handle());
}

void QAndroidPlatformWindow::propagateSizeHints()
{
    //shut up warning from default implementation
}

void QAndroidPlatformWindow::requestActivateWindow()
{
    if (!blockedByModal())
        raise();
}

void QAndroidPlatformWindow::updateSystemUiVisibility()
{
    Qt::WindowFlags flags = window()->flags();
    bool isNonRegularWindow = flags & (Qt::Popup | Qt::Dialog | Qt::Sheet) & ~Qt::Window;
    if (!isNonRegularWindow) {
        if (m_windowState & Qt::WindowFullScreen)
            QtAndroid::setSystemUiVisibility(QtAndroid::SYSTEM_UI_VISIBILITY_FULLSCREEN);
        else if (flags & Qt::MaximizeUsingFullscreenGeometryHint)
            QtAndroid::setSystemUiVisibility(QtAndroid::SYSTEM_UI_VISIBILITY_TRANSLUCENT);
        else
            QtAndroid::setSystemUiVisibility(QtAndroid::SYSTEM_UI_VISIBILITY_NORMAL);
    }
}

bool QAndroidPlatformWindow::isExposed() const
{
    return qApp->applicationState() > Qt::ApplicationHidden
            && window()->isVisible()
            && !window()->geometry().isEmpty();
}

void QAndroidPlatformWindow::applicationStateChanged(Qt::ApplicationState)
{
    QRegion region;
    if (isExposed())
        region = QRect(QPoint(), geometry().size());

    QWindowSystemInterface::handleExposeEvent(window(), region);
    QWindowSystemInterface::flushWindowSystemEvents();
}

void QAndroidPlatformWindow::createSurface()
{
    const QRect rect = geometry();
    jint x = 0, y = 0, w = -1, h = -1;
    if (!rect.isNull()) {
        x = rect.x();
        y = rect.y();
        w = std::max(rect.width(), 1);
        h = std::max(rect.height(), 1);
    }

    const bool windowStaysOnTop = bool(window()->flags() & Qt::WindowStaysOnTopHint);

    m_nativeQtWindow.callMethod<void>("createSurface", windowStaysOnTop, x, y, w, h, 32);
    m_surfaceCreated = true;
}

void QAndroidPlatformWindow::destroySurface()
{
    if (m_surfaceCreated) {
        m_nativeQtWindow.callMethod<void>("destroySurface");
        m_surfaceCreated = false;
    }
}

void QAndroidPlatformWindow::setSurfaceGeometry(const QRect &geometry)
{
    if (!m_surfaceCreated)
        return;

    jint x = 0;
    jint y = 0;
    jint w = -1;
    jint h = -1;
    if (!geometry.isNull()) {
        x = geometry.x();
        y = geometry.y();
        w = geometry.width();
        h = geometry.height();
    }
    m_nativeQtWindow.callMethod<void>("setSurfaceGeometry", x, y, w, h);
}

void QAndroidPlatformWindow::onSurfaceChanged(QtJniTypes::Surface surface)
{
    lockSurface();
    m_androidSurfaceObject = surface;
    if (m_androidSurfaceObject.isValid()) // wait until we have a valid surface to draw into
         m_surfaceWaitCondition.wakeOne();
    unlockSurface();

    if (m_androidSurfaceObject.isValid()) {
        // repaint the window, when we have a valid surface
        sendExpose();
    }
}

void QAndroidPlatformWindow::sendExpose() const
{
    QRect availableGeometry = screen()->availableGeometry();
    if (!geometry().isNull() && !availableGeometry.isNull()) {
        QWindowSystemInterface::handleExposeEvent(window(),
                                                  QRegion(QRect(QPoint(), geometry().size())));
    }
}

bool QAndroidPlatformWindow::blockedByModal() const
{
    QWindow *modalWindow = QGuiApplication::modalWindow();
    return modalWindow && modalWindow != window();
}

void QAndroidPlatformWindow::setSurface(JNIEnv *env, jobject object, jint windowId,
                                        QtJniTypes::Surface surface)
{
    Q_UNUSED(env)
    Q_UNUSED(object)

    if (!qGuiApp)
        return;

    const QList<QWindow*> windows = qGuiApp->allWindows();
    for (QWindow * window : windows) {
        if (!window->handle())
            continue;
        QAndroidPlatformWindow *platformWindow =
                                static_cast<QAndroidPlatformWindow *>(window->handle());
        if (platformWindow->nativeViewId() == windowId)
            platformWindow->onSurfaceChanged(surface);
    }
}

bool QAndroidPlatformWindow::registerNatives(QJniEnvironment &env)
{
    if (!env.registerNativeMethods(QtJniTypes::Traits<QtJniTypes::QtWindow>::className(),
                                  {Q_JNI_NATIVE_SCOPED_METHOD(setSurface, QAndroidPlatformWindow)})) {
        qCCritical(lcQpaWindow) << "RegisterNatives failed for"
                                << QtJniTypes::Traits<QtJniTypes::QtWindow>::className();
        return false;
    }
    return true;
}

QT_END_NAMESPACE
