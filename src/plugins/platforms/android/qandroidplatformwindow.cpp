// Copyright (C) 2014 BogDan Vatra <bogdan@kde.org>
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qandroidplatformwindow.h"
#include "androidbackendregister.h"
#include "qandroidplatformopenglcontext.h"
#include "qandroidplatformscreen.h"

#include "androidjnimain.h"

#include <qguiapplication.h>
#include <qpa/qwindowsysteminterface.h>
#include <private/qhighdpiscaling_p.h>
#include <private/qjnihelpers_p.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcQpaWindow, "qt.qpa.window")

Q_DECLARE_JNI_CLASS(QtWindowInterface, "org/qtproject/qt/android/QtWindowInterface")
Q_DECLARE_JNI_CLASS(QtInputInterface, "org/qtproject/qt/android/QtInputInterface")
Q_DECLARE_JNI_CLASS(QtInputConnectionListener,
                    "org/qtproject/qt/android/QtInputConnection$QtInputConnectionListener")
Q_DECLARE_JNI_CLASS(QtDisplayManager, "org/qtproject/qt/android/QtWindowInterface")

QAndroidPlatformWindow::QAndroidPlatformWindow(QWindow *window)
    : QPlatformWindow(window), m_nativeQtWindow(nullptr),
      m_surfaceContainerType(SurfaceContainer::TextureView), m_nativeParentQtWindow(nullptr),
      m_androidSurfaceObject(nullptr)
{
    if (window->surfaceType() == QSurface::RasterSurface)
        window->setSurfaceType(QSurface::OpenGLSurface);
}

void QAndroidPlatformWindow::initialize()
{
    if (isEmbeddingContainer())
        return;

    QWindow *window = QPlatformWindow::window();

    if (parent()) {
        QAndroidPlatformWindow *androidParent = static_cast<QAndroidPlatformWindow*>(parent());
        if (!androidParent->isEmbeddingContainer())
            m_nativeParentQtWindow = androidParent->nativeWindow();
    }

    AndroidBackendRegister *reg = QtAndroid::backendRegister();
    QtJniTypes::QtInputConnectionListener listener =
            reg->callInterface<QtJniTypes::QtInputInterface, QtJniTypes::QtInputConnectionListener>(
                    "getInputConnectionListener");

    m_nativeQtWindow = QJniObject::construct<QtJniTypes::QtWindow>(
            QNativeInterface::QAndroidApplication::context(),
            isForeignWindow(), m_nativeParentQtWindow, listener);
    m_nativeViewId = m_nativeQtWindow.callMethod<jint>("getId");

    m_windowFlags = Qt::Widget;
    m_windowState = Qt::WindowNoState;
    // the surfaceType is overwritten in QAndroidPlatformOpenGLWindow ctor so let's save
    // the fact that it's a raster window for now
    m_isRaster = window->surfaceType() == QSurface::RasterSurface;
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
        setGeometry(finalNativeGeometry);
    }

    if (window->isTopLevel())
        platformScreen()->addWindow(this);

    static bool ok = false;
    static const int value = qEnvironmentVariableIntValue("QT_ANDROID_SURFACE_CONTAINER_TYPE", &ok);
    if (ok) {
        static const SurfaceContainer type = static_cast<SurfaceContainer>(value);
        if (type == SurfaceContainer::SurfaceView || type == SurfaceContainer::TextureView)
            m_surfaceContainerType = type;
    } else if (platformScreen()->windows().size() <= 1) {
        // TODO should handle case where this changes at runtime -> need to change existing window
        // into TextureView (or perhaps not, if the parent window would be SurfaceView, as long as
        // onTop was false it would stay below the children)
        m_surfaceContainerType = SurfaceContainer::SurfaceView;
    }
    qCDebug(lcQpaWindow) << "Window" << m_nativeViewId << "using surface container type"
                         << static_cast<int>(m_surfaceContainerType);
}

QAndroidPlatformWindow::~QAndroidPlatformWindow()
{
    const auto guard = destructionGuard();
    if (window()->isTopLevel())
        platformScreen()->removeWindow(this);
}


void QAndroidPlatformWindow::lower()
{
    if (m_nativeParentQtWindow.isValid()) {
        m_nativeParentQtWindow.callMethod<void>("bringChildToBack", nativeViewId());
        return;
    }
    platformScreen()->lower(this);
}

void QAndroidPlatformWindow::raise()
{
    if (m_nativeParentQtWindow.isValid()) {
        m_nativeParentQtWindow.callMethod<void>("bringChildToFront", nativeViewId());
        QWindowSystemInterface::handleFocusWindowChanged(window(), Qt::ActiveWindowFocusReason);
        return;
    }
    updateSystemUiVisibility();
    platformScreen()->raise(this);
}

QMargins QAndroidPlatformWindow::safeAreaMargins() const
{
    return m_safeAreaMargins;
}

void QAndroidPlatformWindow::setSafeAreaMargins(const QMargins safeMargins)
{
    m_safeAreaMargins = safeMargins;
}

void QAndroidPlatformWindow::setGeometry(const QRect &rect)
{
    if (!isEmbeddingContainer()) {
        Q_ASSERT(m_nativeQtWindow.isValid());

        jint x = 0;
        jint y = 0;
        jint w = -1;
        jint h = -1;
        if (!rect.isNull()) {
            x = rect.x();
            y = rect.y();
            w = rect.width();
            h = rect.height();
        }
        m_nativeQtWindow.callMethod<void>("setGeometry", x, y, w, h);
    }

    QWindowSystemInterface::handleGeometryChange(window(), rect);
}

void QAndroidPlatformWindow::setVisible(bool visible)
{
    if (isEmbeddingContainer())
        return;

    if (window()->isTopLevel()) {
        if (!visible && window() == qGuiApp->focusWindow()) {
            platformScreen()->topVisibleWindowChanged();
        } else {
            updateSystemUiVisibility();
            if ((m_windowState & Qt::WindowFullScreen)
                || (window()->flags() & Qt::ExpandedClientAreaHint)) {
                setGeometry(platformScreen()->geometry());
            } else if (m_windowState & Qt::WindowMaximized) {
                setGeometry(platformScreen()->availableGeometry());
            }
            requestActivateWindow();
        }
    }

    m_nativeQtWindow.callMethod<void>("setVisible", visible);

    if (geometry().isEmpty() || screen()->availableGeometry().isEmpty())
        return;

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
    using namespace QtJniTypes;

    if (window) {
        auto androidWindow = static_cast<const QAndroidPlatformWindow*>(window);
        if (androidWindow->isEmbeddingContainer())
            return;
        // If we were a top level window, remove from screen
        if (!m_nativeParentQtWindow.isValid())
            platformScreen()->removeWindow(this);

        const QtWindow parentWindow = androidWindow->nativeWindow();
        // If this was a child window of another window, the java method takes care of that
        m_nativeQtWindow.callMethod<void, QtWindow>("setParent", parentWindow.object());
        m_nativeParentQtWindow = parentWindow;
    } else if (QtAndroid::isQtApplication()) {
        m_nativeQtWindow.callMethod<void, QtWindow>("setParent", nullptr);
        m_nativeParentQtWindow = QJniObject();
        platformScreen()->addWindow(this);
    }
}

WId QAndroidPlatformWindow::winId() const
{
    return m_nativeQtWindow.isValid() ? reinterpret_cast<WId>(m_nativeQtWindow.object()) : 0L;
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
    // raise() will handle differences between top level and child windows, and requesting focus
    if (!blockedByModal())
        raise();
}

void QAndroidPlatformWindow::updateSystemUiVisibility()
{
    const int flags = window()->flags();
    const bool isNonRegularWindow = flags & (Qt::Popup | Qt::Dialog | Qt::Sheet) & ~Qt::Window;
    if (!isNonRegularWindow) {
        const bool isFullScreen = (m_windowState & Qt::WindowFullScreen);
        const bool expandedToCutout = (flags & Qt::ExpandedClientAreaHint);
        QtAndroid::backendRegister()->callInterface<QtJniTypes::QtWindowInterface, void>(
            "setSystemUiVisibility", isFullScreen, expandedToCutout);
    }
}

void QAndroidPlatformWindow::updateFocusedEditText()
{
    m_nativeQtWindow.callMethod<void>("updateFocusedEditText");
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
    const bool windowStaysOnTop = bool(window()->flags() & Qt::WindowStaysOnTopHint);
    const bool isOpaque = !format().hasAlpha() && qFuzzyCompare(window()->opacity(), qreal(1.0));

    m_nativeQtWindow.callMethod<void>("createSurface", windowStaysOnTop, 32, isOpaque,
                                      m_surfaceContainerType);
    m_androidSurfaceCreated = true;
}

void QAndroidPlatformWindow::destroySurface()
{
    if (m_androidSurfaceCreated) {
        m_nativeQtWindow.callMethod<void>("destroySurface");
        m_androidSurfaceCreated = false;
    }
}

void QAndroidPlatformWindow::onSurfaceChanged(QtJniTypes::Surface surface)
{
    lockSurface();
    const bool surfaceIsValid = surface.isValid();
    qCDebug(lcQpaWindow) << "onSurfaceChanged(): valid Surface received" << surfaceIsValid;
    m_androidSurfaceObject = surface;
    if (surfaceIsValid) {
        // wait until we have a valid surface to draw into
        m_surfaceWaitCondition.wakeOne();
    } else {
        clearSurface();
    }

    sendExpose();

    unlockSurface();
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

bool QAndroidPlatformWindow::isEmbeddingContainer() const
{
    // Returns true if the window is a wrapper for a foreign window solely to allow embedding Qt
    // into a native Android app, in which case we should not try to control it more than we "need" to
    return !QtAndroid::isQtApplication() && window()->isTopLevel();
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
        const auto guard = platformWindow->destructionGuard();
        if (!platformWindow->m_androidSurfaceCreated)
            continue;
        if (platformWindow->nativeViewId() == windowId)
            platformWindow->onSurfaceChanged(surface);
    }
}

void QAndroidPlatformWindow::windowFocusChanged(JNIEnv *env, jobject object,
                                          jboolean focus, jint windowId)
{
    Q_UNUSED(env)
    Q_UNUSED(object)
    QWindow* window = QtAndroid::windowFromId(windowId);
    Q_ASSERT_X(window, "QAndroidPlatformWindow", "windowFocusChanged event window should exist");
    if (focus) {
        QWindowSystemInterface::handleFocusWindowChanged(window);
    } else if (!focus && window == qGuiApp->focusWindow()) {
        // Clear focus if current window has lost focus
        QWindowSystemInterface::handleFocusWindowChanged(nullptr);
    }
}

void QAndroidPlatformWindow::safeAreaMarginsChanged(JNIEnv *env, jobject object,
                                                    QtJniTypes::Insets insets, jint id)
{
    Q_UNUSED(env)
    Q_UNUSED(object)

    if (!qGuiApp)
        return;

    if (!insets.isValid())
        return;

    QAndroidPlatformWindow *pWindow = nullptr;
    for (QWindow *window : qGuiApp->allWindows()) {
        if (!window->handle())
            continue;
        QAndroidPlatformWindow *pw = static_cast<QAndroidPlatformWindow *>(window->handle());
        if (pw->nativeViewId() == id) {
            pWindow = pw;
            break;
        }
    }

    if (!pWindow)
        return;

    QMargins safeMargins = QMargins(
                insets.getField<int>("left"),
                insets.getField<int>("top"),
                insets.getField<int>("right"),
                insets.getField<int>("bottom"));

    if (safeMargins != pWindow->safeAreaMargins()) {
        pWindow->setSafeAreaMargins(safeMargins);
        QWindowSystemInterface::handleSafeAreaMarginsChanged(pWindow->window());
    }
}

static void updateWindows(JNIEnv *env, jobject object)
{
    Q_UNUSED(env)
    Q_UNUSED(object)

    if (QGuiApplication::instance() != nullptr) {
        const auto tlw = QGuiApplication::topLevelWindows();
        for (QWindow *w : tlw) {

            // Skip non-platform windows, e.g., offscreen windows.
            if (!w->handle())
                continue;

            const QRect availableGeometry = w->screen()->availableGeometry();
            const QRect geometry = w->geometry();
            const bool isPositiveGeometry = (geometry.width() > 0 && geometry.height() > 0);
            const bool isPositiveAvailableGeometry =
                (availableGeometry.width() > 0 && availableGeometry.height() > 0);

            if (isPositiveGeometry && isPositiveAvailableGeometry) {
                const QRegion region = QRegion(QRect(QPoint(), w->geometry().size()));
                QWindowSystemInterface::handleExposeEvent(w, region);
            }
        }
    }
}
Q_DECLARE_JNI_NATIVE_METHOD(updateWindows)

/*
    Due to calls originating from Android, it is possible for native methods to
    try to manipulate any given instance of QAndroidPlatformWindow when it is
    already being destroyed. So we use this to guard against that. It is called
    in the destructor, and should also be called in any function registered to
    be called from java that may touch an instance of QAndroidPlatformWindow.
 */
QMutexLocker<QMutex> QAndroidPlatformWindow::destructionGuard()
{
    return QMutexLocker(&m_destructionMutex);
}

bool QAndroidPlatformWindow::registerNatives(QJniEnvironment &env)
{
    if (!env.registerNativeMethods(QtJniTypes::Traits<QtJniTypes::QtWindow>::className(),
            {
                Q_JNI_NATIVE_METHOD(updateWindows),
                Q_JNI_NATIVE_SCOPED_METHOD(setSurface, QAndroidPlatformWindow),
                Q_JNI_NATIVE_SCOPED_METHOD(windowFocusChanged, QAndroidPlatformWindow),
                Q_JNI_NATIVE_SCOPED_METHOD(safeAreaMarginsChanged, QAndroidPlatformWindow)
            })) {
        qCCritical(lcQpaWindow) << "RegisterNatives failed for"
                                << QtJniTypes::Traits<QtJniTypes::QtWindow>::className();
        return false;
    }
    return true;
}

QT_END_NAMESPACE
