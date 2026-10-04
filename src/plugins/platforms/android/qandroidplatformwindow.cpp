// Copyright (C) 2014 BogDan Vatra <bogdan@kde.org>
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qandroidplatformwindow.h"
#include "androidbackendregister.h"
#include "qandroideventdispatcher.h"
#include "qandroidplatformscreen.h"

#include "androidjnimain.h"

#include <qguiapplication.h>
#include <qpa/qwindowsysteminterface.h>
#include <qthread.h>
#include <private/qhighdpiscaling_p.h>
#include <private/qjnihelpers_p.h>

#include <android/native_window.h>
#include <android/native_window_jni.h>

#include <memory>
#include <optional>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcQpaWindow, "qt.qpa.window")

Q_DECLARE_JNI_CLASS(QtWindowInterface, "org/qtproject/qt/android/QtWindowInterface")
Q_DECLARE_JNI_CLASS(QtInputInterface, "org/qtproject/qt/android/QtInputInterface")
Q_DECLARE_JNI_CLASS(QtInputConnectionListener,
                    "org/qtproject/qt/android/QtInputConnection$QtInputConnectionListener")
Q_DECLARE_JNI_CLASS(QtDisplayManager, "org/qtproject/qt/android/QtWindowInterface")
Q_DECLARE_JNI_CLASS(QtWindowInsetsController, "org/qtproject/qt/android/QtWindowInsetsController")

QAndroidPlatformWindow::QAndroidPlatformWindow(QWindow *window)
    : QPlatformWindow(window), m_nativeQtWindow(nullptr),
      m_surfaceContainerType(SurfaceContainer::TextureView), m_nativeParentQtWindow(nullptr)
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

    // the surfaceType is overwritten in QAndroidPlatformOpenGLWindow ctor so let's save
    // the fact that it's a raster window for now
    m_isRaster = window->surfaceType() == QSurface::RasterSurface;

    // the following is in relation to the virtual geometry
    const bool forceMaximize = window->windowStates() & (Qt::WindowMaximized | Qt::WindowFullScreen);
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
    releaseSurface();
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
        return;
    }
    applySystemUi(window()->windowStates(), window()->flags());
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
    QWindowSystemInterface::handleGeometryChange(window(), rect);
    updateNativeWindow(window()->isVisible());
}

void QAndroidPlatformWindow::updateNativeWindow(bool visible)
{
    if (isEmbeddingContainer())
        return;
    // A maximized or full screen window fills the root layout, so Android
    // resizes it in the pass that rotates, before its Surface is redrawn.
    const bool fillsLayout = window()->isTopLevel()
            && window()->windowStates() & (Qt::WindowMaximized | Qt::WindowFullScreen);
    const QRect rect = fillsLayout ? QRect() : geometry();
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
    m_nativeQtWindow.callMethod<void>("updateWindow", x, y, w, h, visible);
}

void QAndroidPlatformWindow::setVisible(bool visible)
{
    if (isEmbeddingContainer())
        return;

    if (window()->isTopLevel()) {
        if (!visible && window() == qGuiApp->focusWindow()) {
            platformScreen()->topVisibleWindowChanged();
        } else {
            const Qt::WindowStates states = window()->windowStates();
            const Qt::WindowFlags flags = window()->flags();
            applySystemUi(states, flags);
            if (states & Qt::WindowFullScreen || flags & Qt::ExpandedClientAreaHint)
                setGeometry(platformScreen()->geometry());
            else if (states & Qt::WindowMaximized)
                setGeometry(platformScreen()->availableGeometry());
            requestActivateWindow();
        }
    }

    if (visible && !isEmbeddingContainer()) {
        const bool onTop = window()->flags() & Qt::WindowStaysOnTopHint;
        const bool isOpaque = !format().hasAlpha()
                && qFuzzyCompare(window()->opacity(), qreal(1.0));
        m_nativeQtWindow.callMethod<void>("configureSurface", onTop, isOpaque,
                                          m_surfaceContainerType);
    }
    updateNativeWindow(visible);

    if (geometry().isEmpty() || screen()->availableGeometry().isEmpty())
        return;

    QPlatformWindow::setVisible(visible);
    sendChildExposeEvents();
}

void QAndroidPlatformWindow::setWindowState(Qt::WindowStates state)
{
    QPlatformWindow::setWindowState(state);

    if (window()->isVisible())
        applySystemUi(state, window()->flags());
    // Whether the window fills the root layout depends on the state.
    updateNativeWindow(window()->isVisible());
}

void QAndroidPlatformWindow::setWindowFlags(Qt::WindowFlags flags)
{
    QPlatformWindow::setWindowFlags(flags);

    if (window()->isVisible())
        applySystemUi(window()->windowStates(), flags);
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
    if (blockedByModal())
        return;
    raise();
    // For child windows, raise() only updates "Z-order", so focus must be requested explicitly.
    if (m_nativeParentQtWindow.isValid())
        QWindowSystemInterface::handleFocusWindowChanged(window(), Qt::ActiveWindowFocusReason);
}

namespace {
// The extended system UI modes Qt can take ownership of. Plain "normal" is not
// a mode Qt owns, it is the absence of ownership, represented by an empty
// optional, so window state the app manages itself is never touched by Qt.
enum class SystemUiMode { Expanded, FullScreen };

std::optional<SystemUiMode> systemUiModeFor(Qt::WindowStates states, Qt::WindowFlags flags)
{
    if (states & Qt::WindowFullScreen)
        return SystemUiMode::FullScreen;
    if (flags & Qt::ExpandedClientAreaHint)
        return SystemUiMode::Expanded;
    return std::nullopt;
}

// The mode Qt currently owns for the activity window, only ever touched on the
// gui thread. Empty means Qt owns nothing and leaves the system UI to the app.
std::optional<SystemUiMode> g_ownedSystemUiMode;

void applySystemUiMode(std::optional<SystemUiMode> mode)
{
    g_ownedSystemUiMode = mode;

    auto iface = qGuiApp->nativeInterface<QNativeInterface::QAndroidApplication>();
    iface->runOnAndroidMainThread([mode, iface]() {
        using namespace QtJniTypes;
        auto activity = iface->context().object<Activity>();
        if (mode == SystemUiMode::FullScreen)
            QtWindowInsetsController::callStaticMethod("showFullScreen", activity);
        else if (mode == SystemUiMode::Expanded)
            QtWindowInsetsController::callStaticMethod("showExpanded", activity);
        else
            QtWindowInsetsController::callStaticMethod("showNormal", activity);
    });
}
} // unnamed namespace

void QAndroidPlatformWindow::applySystemUi(Qt::WindowStates states, Qt::WindowFlags flags)
{
    const bool isNonRegularWindow = flags & (Qt::Popup | Qt::Dialog | Qt::Sheet) & ~Qt::Window;
    if (isNonRegularWindow)
        return;

    const std::optional<SystemUiMode> mode = systemUiModeFor(states, flags);
    if (mode != g_ownedSystemUiMode)
        applySystemUiMode(mode);
}

void QAndroidPlatformWindow::applySystemUiNative(JNIEnv *env, jobject object)
{
    Q_UNUSED(env)
    Q_UNUSED(object)

    if (!qGuiApp)
        return;

    // Some lifecycle events reset window state on the OS side. Re-assert the
    // mode Qt owns, if any. When Qt owns nothing the app keeps its own state.
    QMetaObject::invokeMethod(qGuiApp, [] {
        if (g_ownedSystemUiMode)
            applySystemUiMode(g_ownedSystemUiMode);
    });
}

void QAndroidPlatformWindow::updateFocusedEditText()
{
    m_nativeQtWindow.callMethod<void>("updateFocusedEditText");
}

bool QAndroidPlatformWindow::isExposed() const
{
    // Not exposed until Android provides a Surface, so QRhi never starts
    // without a GPU surface. A child is on screen only while its parent is.
    const QWindow *parent = window()->parent();
    return m_surfaceValid
            && (!parent || !parent->handle() || parent->handle()->isExposed())
            && qApp->applicationState() > Qt::ApplicationHidden
            && window()->isVisible()
            && !window()->geometry().isEmpty();
}

void QAndroidPlatformWindow::applicationStateChanged(Qt::ApplicationState)
{
    QRegion region;
    if (isExposed())
        region = QRect(QPoint(), geometry().size());

    QWindowSystemInterface::handleExposeEvent(window(), region);
    sendChildExposeEvents();
    QWindowSystemInterface::flushWindowSystemEvents();
}

void QAndroidPlatformWindow::sendChildExposeEvents() const
{
    // Child exposure depends on this window's, so it changes with it.
    for (QObject *object : window()->children()) {
        auto *child = qobject_cast<QWindow *>(object);
        if (!child || !child->handle())
            continue;
        auto *platformChild = static_cast<QAndroidPlatformWindow *>(child->handle());
        QRegion region;
        if (platformChild->isExposed())
            region = QRect(QPoint(), platformChild->geometry().size());
        QWindowSystemInterface::handleExposeEvent(child, region);
        platformChild->sendChildExposeEvents();
    }
}

void QAndroidPlatformWindow::ensureNativeWindow()
{
    if (m_nativeWindow || QThread::currentThread() != qApp->thread())
        return;
    if (!m_nativeQtWindow.isValid())
        return;
    // Ask Java for the Surface directly instead of waiting for the queued
    // surfaceCreated, which would re-enter user code from inside makeCurrent().
    auto surface = m_nativeQtWindow.callMethod<QtJniTypes::Surface>("getSurface");
    if (surface.isValid() && surface.callMethod<jboolean>("isValid")) {
        if (ANativeWindow *native = ANativeWindow_fromSurface(
                    QJniEnvironment::getJniEnv(), surface.object())) {
            m_nativeWindow = native;
        }
    }
}

void QAndroidPlatformWindow::onSurfaceCreated(QtJniTypes::Surface surface)
{
    qCDebug(lcQpaWindow) << "onSurfaceCreated() for windowId" << m_nativeViewId;
    if (isForeignWindow() || isEmbeddingContainer() || !surface.isValid())
        return;

    ANativeWindow *native = ANativeWindow_fromSurface(QJniEnvironment::getJniEnv(),
                                                      surface.object());
    if (!native) {
        qCWarning(lcQpaWindow) << "ANativeWindow_fromSurface returned null for windowId"
                               << m_nativeViewId;
        return;
    }

    if (native == m_nativeWindow) {
        // Already fetched by ensureNativeWindow() or re-sent by
        // Android, so keep the GPU surface and drop the extra reference.
        ANativeWindow_release(native);
    } else {
        if (m_nativeWindow) {
            // New Surface without a surfaceDestroyed. Release the old GPU
            // surface first, else eglCreateWindowSurface() hits EGL_BAD_ALLOC.
            QPlatformSurfaceEvent surfaceEvent(QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed);
            QGuiApplication::sendEvent(window(), &surfaceEvent);
            releaseSurface();
        }
        m_nativeWindow = native;

        // Pairs with SurfaceAboutToBeDestroyed. QWidget recreates its native
        // state (winId) only on SurfaceCreated.
        QPlatformSurfaceEvent surfaceEvent(QPlatformSurfaceEvent::SurfaceCreated);
        QGuiApplication::sendEvent(window(), &surfaceEvent);
    }

    m_surfaceValid = true;

    // Without a non-empty expose the QWindow never becomes exposed and
    // renders nothing.
    QWindowSystemInterface::handleExposeEvent(window(),
                                              QRegion(QRect(QPoint(), geometry().size())));
    sendChildExposeEvents();
}

void QAndroidPlatformWindow::onRedrawNeeded()
{
    qCDebug(lcQpaWindow) << "onRedrawNeeded() for windowId" << m_nativeViewId;
    // Synchronous, so the frame for the current size is done when this returns.
    if (isExposed()) {
        QWindowSystemInterface::handleExposeEvent<QWindowSystemInterface::SynchronousDelivery>(
                window(), QRegion(QRect(QPoint(), geometry().size())));
    }
}

void QAndroidPlatformWindow::onSurfaceDestroyed()
{
    qCDebug(lcQpaWindow) << "onSurfaceDestroyed() for windowId" << m_nativeViewId;
    if (isForeignWindow() || isEmbeddingContainer())
        return;

    // Unexpose first so the scene graph stops scheduling frames and doesn't
    // rebuild its swapchain against the missing surface.
    m_surfaceValid = false;
    QWindowSystemInterface::handleExposeEvent<QWindowSystemInterface::SynchronousDelivery>(
            window(), QRegion());
    sendChildExposeEvents();

    // Returns once the scene graph released its swapchain, so the render thread
    // no longer uses the GPU surface.
    QPlatformSurfaceEvent surfaceEvent(QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed);
    QGuiApplication::sendEvent(window(), &surfaceEvent);

    releaseSurface();
}

void QAndroidPlatformWindow::releaseSurface()
{
    destroyWindowSurface();
    if (m_nativeWindow) {
        ANativeWindow_release(m_nativeWindow);
        m_nativeWindow = nullptr;
    }
}

void QAndroidPlatformWindow::destroySurface()
{
    releaseSurface();
    // Embedding containers never construct the Java window, so there is
    // nothing to destroy.
    if (m_nativeQtWindow.isValid())
        m_nativeQtWindow.callMethod<void>("destroySurface");
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

QAndroidPlatformWindow *QAndroidPlatformWindow::findWindowById(jint windowId)
{
    const QList<QWindow*> windows = qGuiApp->allWindows();
    for (QWindow *w : windows) {
        if (!w->handle())
            continue;
        auto *pw = static_cast<QAndroidPlatformWindow *>(w->handle());
        if (pw->nativeViewId() == windowId)
            return pw;
    }
    return nullptr;
}

void QAndroidPlatformWindow::surfaceCreated(JNIEnv *env, jobject object, jint windowId,
                                            QtJniTypes::Surface surface)
{
    Q_UNUSED(env)
    Q_UNUSED(object)
    if (!qGuiApp)
        return;
    // Queued, not blocking, so the Android UI thread keeps compositing.
    // Blocking here showed black bands flickering during transitions.
    QJniObject keepalive(surface);
    QMetaObject::invokeMethod(qApp, [windowId, keepalive]() {
        if (auto *pw = findWindowById(windowId))
            pw->onSurfaceCreated(QtJniTypes::Surface(keepalive));
    }, Qt::QueuedConnection);
}

void QAndroidPlatformWindow::surfaceRedrawNeeded(JNIEnv *env, jobject object, jint windowId,
                                                QtJniTypes::Runnable drawingFinished)
{
    Q_UNUSED(env)
    Q_UNUSED(object)
    QJniObject finished(drawingFinished);
    // A parked Qt thread can't draw, and Android must not wait for it.
    if (!qGuiApp || (QtAndroid::blockEventLoopsWhenSuspended()
                     && QAndroidEventDispatcherStopper::stopped())) {
        finished.callMethod<void>("run");
        return;
    }
    // Android keeps the old frame until drawingFinished runs, which may happen
    // on any thread, so no wait here.
    QMetaObject::invokeMethod(qApp, [windowId, finished]() {
        if (auto *pw = findWindowById(windowId))
            pw->onRedrawNeeded();
        finished.callMethod<void>("run");
    }, Qt::QueuedConnection);
}

void QAndroidPlatformWindow::surfaceDestroyed(JNIEnv *env, jobject object, jint windowId,
                                             QtJniTypes::Runnable surfaceReleased)
{
    Q_UNUSED(env)
    Q_UNUSED(object)
    QJniObject released(surfaceReleased);
    const auto finish = [released] {
        if (released.isValid())
            released.callMethod<void>("run");
    };
    if (!qGuiApp) {
        finish();
        return;
    }
    // A parked Qt thread cannot process queued teardown until resume.
    if (QtAndroid::blockEventLoopsWhenSuspended()
            && QAndroidEventDispatcherStopper::stopped()) {
        if (auto *pw = findWindowById(windowId))
            pw->releaseSurface();
        finish();
        return;
    }
    QMetaObject::invokeMethod(qApp, [windowId, finish]() {
        if (auto *pw = findWindowById(windowId))
            pw->onSurfaceDestroyed();
        finish();
    }, Qt::QueuedConnection);
}

void QAndroidPlatformWindow::windowFocusChanged(JNIEnv *env, jobject object,
                                          jboolean focus, jint windowId)
{
    Q_UNUSED(env)
    Q_UNUSED(object)
    if (!qGuiApp)
        return;
    // Queued like the surface handlers, so focus never arrives before the
    // surface state it depends on. Handling it inline broke QQuickWidget focus.
    QMetaObject::invokeMethod(qApp, [windowId, focus]() {
        QWindow *window = QtAndroid::windowFromId(windowId);
        if (!window) {
            qCWarning(lcQpaWindow,
                "windowFocusChanged event received for non-existing window %d", windowId);
            return;
        }
        if (focus)
            QWindowSystemInterface::handleFocusWindowChanged(window);
        else if (window == qGuiApp->focusWindow())
            QWindowSystemInterface::handleFocusWindowChanged(nullptr);
    }, Qt::QueuedConnection);
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

bool QAndroidPlatformWindow::registerNatives(QJniEnvironment &env)
{
    if (!env.registerNativeMethods(QtJniTypes::Traits<QtJniTypes::QtWindow>::className(),
            {
                Q_JNI_NATIVE_METHOD(updateWindows),
                Q_JNI_NATIVE_SCOPED_METHOD(surfaceCreated, QAndroidPlatformWindow),
                Q_JNI_NATIVE_SCOPED_METHOD(surfaceRedrawNeeded, QAndroidPlatformWindow),
                Q_JNI_NATIVE_SCOPED_METHOD(surfaceDestroyed, QAndroidPlatformWindow),
                Q_JNI_NATIVE_SCOPED_METHOD(windowFocusChanged, QAndroidPlatformWindow),
                Q_JNI_NATIVE_SCOPED_METHOD(safeAreaMarginsChanged, QAndroidPlatformWindow)
            })) {
        qCCritical(lcQpaWindow) << "RegisterNatives failed for"
                                << QtJniTypes::Traits<QtJniTypes::QtWindow>::className();
        return false;
    }

    if (!env.registerNativeMethods(
            QtJniTypes::Traits<QtJniTypes::QtWindowInsetsController>::className(),
            {
                Q_JNI_NATIVE_SCOPED_METHOD(applySystemUiNative, QAndroidPlatformWindow)
            })) {
        qCCritical(lcQpaWindow) << "RegisterNatives failed for"
                << QtJniTypes::Traits<QtJniTypes::QtWindowInsetsController>::className();
        return false;
    }
    return true;
}

QT_END_NAMESPACE
