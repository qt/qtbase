// Copyright (C) 2014 BogDan Vatra <bogdan@kde.org>
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QDebug>
#include <QTime>

#include <qpa/qwindowsysteminterface.h>

#include "qandroidplatformscreen.h"
#include "qandroidplatformbackingstore.h"
#include "qandroidplatformintegration.h"
#include "qandroidplatformwindow.h"
#include "androidjnimain.h"
#include "androidjnimenu.h"
#include "androiddeadlockprotector.h"

#include <android/bitmap.h>
#include <android/native_window_jni.h>
#include <qguiapplication.h>

#include <QtCore/QJniObject>
#include <QtCore/QJniEnvironment>
#include <QtGui/QGuiApplication>
#include <QtGui/QWindow>
#include <QtGui/private/qwindow_p.h>
#include <vector>

QT_BEGIN_NAMESPACE

#ifdef QANDROIDPLATFORMSCREEN_DEBUG
class ScopedProfiler
{
public:
    ScopedProfiler(const QString &msg)
    {
        m_msg = msg;
        m_timer.start();
    }
    ~ScopedProfiler()
    {
        qDebug() << m_msg << m_timer.elapsed();
    }

private:
    QTime m_timer;
    QString m_msg;
};

# define PROFILE_SCOPE ScopedProfiler ___sp___(__func__)
#else
# define PROFILE_SCOPE
#endif

Q_DECLARE_JNI_CLASS(Display, "android/view/Display")
Q_DECLARE_JNI_CLASS(DisplayMetrics, "android/util/DisplayMetrics")
Q_DECLARE_JNI_CLASS(Resources, "android/content/res/Resources")
Q_DECLARE_JNI_CLASS(Size, "android/util/Size")
Q_DECLARE_JNI_CLASS(QtNative, "org/qtproject/qt/android/QtNative")

Q_DECLARE_JNI_TYPE(DisplayMode, "Landroid/view/Display$Mode;")

QAndroidPlatformScreen::QAndroidPlatformScreen(const QJniObject &displayObject)
    : QObject(), QPlatformScreen()
{
    m_availableGeometry = QAndroidPlatformIntegration::m_defaultAvailableGeometry;
    m_size = QAndroidPlatformIntegration::m_defaultScreenSize;
    m_physicalSize = QAndroidPlatformIntegration::m_defaultPhysicalSize;

    // Raster only apps should set QT_ANDROID_RASTER_IMAGE_DEPTH to 16
    // is way much faster than 32
    if (qEnvironmentVariableIntValue("QT_ANDROID_RASTER_IMAGE_DEPTH") == 16) {
        m_format = QImage::Format_RGB16;
        m_depth = 16;
    } else {
        m_format = QImage::Format_ARGB32_Premultiplied;
        m_depth = 32;
    }

    connect(qGuiApp, &QGuiApplication::applicationStateChanged, this,
            &QAndroidPlatformScreen::applicationStateChanged);

    if (!displayObject.isValid())
        return;

    m_name = displayObject.callObjectMethod<jstring>("getName").toString();
    m_refreshRate = displayObject.callMethod<jfloat>("getRefreshRate");
    m_displayId = displayObject.callMethod<jint>("getDisplayId");

    const QJniObject context = QNativeInterface::QAndroidApplication::context();
    const auto displayContext = context.callMethod<QtJniTypes::Context>("createDisplayContext",
                                                displayObject.object<QtJniTypes::Display>());

    const auto sizeObj = QJniObject::callStaticMethod<QtJniTypes::Size>(
                                                QtJniTypes::className<QtJniTypes::QtNative>(),
                                                "getDisplaySize",
                                                displayContext.object<QtJniTypes::Context>(),
                                                displayObject.object<QtJniTypes::Display>());
    m_size = QSize(sizeObj.callMethod<int>("getWidth"), sizeObj.callMethod<int>("getHeight"));

    const auto resources = displayContext.callMethod<QtJniTypes::Resources>("getResources");
    const auto metrics = resources.callMethod<QtJniTypes::DisplayMetrics>("getDisplayMetrics");
    const float xdpi = metrics.getField<float>("xdpi");
    const float ydpi = metrics.getField<float>("ydpi");

    // Potentially densityDpi could be used instead of xpdi/ydpi to do the calculation,
    // but the results are not consistent with devices specs.
    // (https://issuetracker.google.com/issues/194120500)
    m_physicalSize.setWidth(qRound(m_size.width() / xdpi * 25.4));
    m_physicalSize.setHeight(qRound(m_size.height() / ydpi * 25.4));

    if (QNativeInterface::QAndroidApplication::sdkVersion() >= 23) {
        const QJniObject currentMode = displayObject.callObjectMethod<QtJniTypes::DisplayMode>("getMode");
        m_currentMode = currentMode.callMethod<jint>("getModeId");

        const QJniObject supportedModes = displayObject.callObjectMethod<QtJniTypes::DisplayMode[]>(
            "getSupportedModes");
        const auto modeArray = jobjectArray(supportedModes.object());

        QJniEnvironment env;
        const auto size = env->GetArrayLength(modeArray);
        for (jsize i = 0; i < size; ++i) {
            const auto mode = QJniObject::fromLocalRef(env->GetObjectArrayElement(modeArray, i));
            m_modes << QPlatformScreen::Mode {
                .size = QSize { mode.callMethod<jint>("getPhysicalWidth"),
                                mode.callMethod<jint>("getPhysicalHeight") },
                .refreshRate = mode.callMethod<jfloat>("getRefreshRate")
            };
        }
    }
}

QAndroidPlatformScreen::~QAndroidPlatformScreen()
{
    if (m_surfaceId != -1) {
        QtAndroid::destroySurface(m_surfaceId);
        m_surfaceWaitCondition.wakeOne();
        releaseSurface();
    }
}

QWindow *QAndroidPlatformScreen::topWindow() const
{
    for (QAndroidPlatformWindow *w : m_windowStack) {
        if (w->window()->type() == Qt::Window ||
                w->window()->type() == Qt::Popup ||
                w->window()->type() == Qt::Dialog) {
            return w->window();
        }
    }
    return 0;
}

QWindow *QAndroidPlatformScreen::topLevelAt(const QPoint &p) const
{
    for (QAndroidPlatformWindow *w : m_windowStack) {
        if (w->geometry().contains(p, false) && w->window()->isVisible())
            return w->window();
    }
    return 0;
}

bool QAndroidPlatformScreen::event(QEvent *event)
{
    if (event->type() == QEvent::UpdateRequest) {
        doRedraw();
        m_updatePending = false;
        return true;
    }
    return QObject::event(event);
}

void QAndroidPlatformScreen::addWindow(QAndroidPlatformWindow *window)
{
    if (window->parent() && window->isRaster())
        return;

    if (m_windowStack.contains(window))
        return;

    m_windowStack.prepend(window);
    if (window->isRaster()) {
        m_rasterSurfaces.ref();
        setDirty(window->geometry());
    }

    QWindow *w = topWindow();
    QWindowSystemInterface::handleWindowActivated(w, Qt::ActiveWindowFocusReason);
    topWindowChanged(w);
}

void QAndroidPlatformScreen::removeWindow(QAndroidPlatformWindow *window)
{
    if (window->parent() && window->isRaster())
        return;

    m_windowStack.removeOne(window);

    if (m_windowStack.contains(window))
        qWarning() << "Failed to remove window";

    if (window->isRaster()) {
        m_rasterSurfaces.deref();
        setDirty(window->geometry());
    }

    QWindow *w = topWindow();
    QWindowSystemInterface::handleWindowActivated(w, Qt::ActiveWindowFocusReason);
    topWindowChanged(w);
}

void QAndroidPlatformScreen::raise(QAndroidPlatformWindow *window)
{
    if (window->parent() && window->isRaster())
        return;

    int index = m_windowStack.indexOf(window);
    if (index <= 0)
        return;
    m_windowStack.move(index, 0);
    if (window->isRaster()) {
        setDirty(window->geometry());
    }
    QWindow *w = topWindow();
    QWindowSystemInterface::handleWindowActivated(w, Qt::ActiveWindowFocusReason);
    topWindowChanged(w);
}

void QAndroidPlatformScreen::lower(QAndroidPlatformWindow *window)
{
    if (window->parent() && window->isRaster())
        return;

    int index = m_windowStack.indexOf(window);
    if (index == -1 || index == (m_windowStack.size() - 1))
        return;
    m_windowStack.move(index, m_windowStack.size() - 1);
    if (window->isRaster()) {
        setDirty(window->geometry());
    }
    QWindow *w = topWindow();
    QWindowSystemInterface::handleWindowActivated(w, Qt::ActiveWindowFocusReason);
    topWindowChanged(w);
}

void QAndroidPlatformScreen::scheduleUpdate()
{
    if (!m_updatePending) {
        m_updatePending = true;
        QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest));
    }
}

void QAndroidPlatformScreen::setDirty(const QRect &rect)
{
    QRect intersection = rect.intersected(m_availableGeometry);
    m_dirtyRect |= intersection;
    scheduleUpdate();
}

void QAndroidPlatformScreen::setPhysicalSize(const QSize &size)
{
    m_physicalSize = size;
}

void QAndroidPlatformScreen::setSize(const QSize &size)
{
    m_size = size;
    QWindowSystemInterface::handleScreenGeometryChange(QPlatformScreen::screen(), geometry(), availableGeometry());
}

void QAndroidPlatformScreen::setSizeParameters(const QSize &physicalSize, const QSize &size,
                                               const QRect &availableGeometry)
{
    // The goal of this method is to set all geometry-related parameters
    // at the same time and generate only one screen geometry change event.
    m_physicalSize = physicalSize;
    m_size = size;
    // If available geometry has changed, the event will be handled in
    // setAvailableGeometry. Otherwise we need to explicitly handle it to
    // retain the behavior, because setSize() does the handling unconditionally.
    if (m_availableGeometry != availableGeometry) {
        setAvailableGeometry(availableGeometry);
    } else {
        QWindowSystemInterface::handleScreenGeometryChange(QPlatformScreen::screen(), geometry(),
                                                           this->availableGeometry());
    }
}

int QAndroidPlatformScreen::displayId() const
{
    return m_displayId;
}

void QAndroidPlatformScreen::setRefreshRate(qreal refreshRate)
{
    if (refreshRate == m_refreshRate)
        return;
    m_refreshRate = refreshRate;
    QWindowSystemInterface::handleScreenRefreshRateChange(QPlatformScreen::screen(), refreshRate);
}

void QAndroidPlatformScreen::setOrientation(Qt::ScreenOrientation orientation)
{
    QWindowSystemInterface::handleScreenOrientationChange(QPlatformScreen::screen(), orientation);
}

void QAndroidPlatformScreen::setAvailableGeometry(const QRect &rect)
{
    QMutexLocker lock(&m_surfaceMutex);
    if (m_availableGeometry == rect)
        return;

    QRect oldGeometry = m_availableGeometry;

    m_availableGeometry = rect;
    QWindowSystemInterface::handleScreenGeometryChange(QPlatformScreen::screen(), geometry(), availableGeometry());
    resizeMaximizedWindows();

    if (oldGeometry.width() == 0 && oldGeometry.height() == 0 && rect.width() > 0 && rect.height() > 0) {
        QList<QWindow *> windows = QGuiApplication::allWindows();
        for (int i = 0; i < windows.size(); ++i) {
            QWindow *w = windows.at(i);
            if (w->handle()) {
                QRect geometry = w->handle()->geometry();
                if (geometry.width() > 0 && geometry.height() > 0)
                    QWindowSystemInterface::handleExposeEvent(w, QRect(QPoint(0, 0), geometry.size()));
            }
        }
    }

    if (m_surfaceId != -1) {
        releaseSurface();
        QtAndroid::setSurfaceGeometry(m_surfaceId, rect);
    }
}

void QAndroidPlatformScreen::applicationStateChanged(Qt::ApplicationState state)
{
    for (QAndroidPlatformWindow *w : std::as_const(m_windowStack))
        w->applicationStateChanged(state);

    if (state <=  Qt::ApplicationHidden) {
        lockSurface();
        QtAndroid::destroySurface(m_surfaceId);
        m_surfaceId = -1;
        releaseSurface();
        unlockSurface();
    }
}

void QAndroidPlatformScreen::topWindowChanged(QWindow *w)
{
    QtAndroidMenu::setActiveTopLevelWindow(w);

    if (w != 0) {
        QAndroidPlatformWindow *platformWindow = static_cast<QAndroidPlatformWindow *>(w->handle());
        if (platformWindow != 0)
            platformWindow->updateSystemUiVisibility();
    }
}

int QAndroidPlatformScreen::rasterSurfaces()
{
    return m_rasterSurfaces;
}

void QAndroidPlatformScreen::doRedraw(QImage* screenGrabImage)
{
    PROFILE_SCOPE;
    if (!QtAndroid::activity())
        return;

    if (m_dirtyRect.isEmpty())
        return;

    // Stop if there are no visible raster windows. If we only have RasterGLSurface
    // windows that have renderToTexture children (i.e. they need the OpenGL path) then
    // we do not need an overlay surface.
    bool hasVisibleRasterWindows = false;
    for (QAndroidPlatformWindow *window : std::as_const(m_windowStack)) {
        if (window->window()->isVisible() && window->isRaster() && !qt_window_private(window->window())->compositing) {
            hasVisibleRasterWindows = true;
            break;
        }
    }
    if (!hasVisibleRasterWindows) {
        lockSurface();
        if (m_surfaceId != -1) {
            QtAndroid::destroySurface(m_surfaceId);
            releaseSurface();
            m_surfaceId = -1;
        }
        unlockSurface();
        return;
    }
    QMutexLocker lock(&m_surfaceMutex);
    if (m_surfaceId == -1 && m_rasterSurfaces) {
        m_surfaceId = QtAndroid::createSurface(this, geometry(), true, m_depth);
        AndroidDeadlockProtector protector;
        if (!protector.acquire())
            return;
        m_surfaceWaitCondition.wait(&m_surfaceMutex);
    }

    if (!m_nativeSurface)
        return;

    ANativeWindow_Buffer nativeWindowBuffer;
    ARect nativeWindowRect;
    nativeWindowRect.top = m_dirtyRect.top();
    nativeWindowRect.left = m_dirtyRect.left();
    nativeWindowRect.bottom = m_dirtyRect.bottom() + 1; // for some reason that I don't understand the QRect bottom needs to +1 to be the same with ARect bottom
    nativeWindowRect.right = m_dirtyRect.right() + 1; // same for the right

    int ret;
    if ((ret = ANativeWindow_lock(m_nativeSurface, &nativeWindowBuffer, &nativeWindowRect)) < 0) {
        qWarning() << "ANativeWindow_lock() failed! error=" << ret;
        return;
    }

    int bpp = 4;
    if (nativeWindowBuffer.format == WINDOW_FORMAT_RGB_565) {
        bpp = 2;
        m_pixelFormat = QImage::Format_RGB16;
    }

    QImage screenImage(reinterpret_cast<uchar *>(nativeWindowBuffer.bits)
                       , nativeWindowBuffer.width, nativeWindowBuffer.height
                       , nativeWindowBuffer.stride * bpp , m_pixelFormat);

    QPainter compositePainter(&screenImage);
    compositePainter.setCompositionMode(QPainter::CompositionMode_Source);

    QRegion visibleRegion(m_dirtyRect);
    for (QAndroidPlatformWindow *window : std::as_const(m_windowStack)) {
        if (!window->window()->isVisible()
                || qt_window_private(window->window())->compositing
                || !window->isRaster())
            continue;

        for (const QRect &rect : std::vector<QRect>(visibleRegion.begin(), visibleRegion.end())) {
            QRect targetRect = window->geometry();
            targetRect &= rect;

            if (targetRect.isNull())
                continue;

            visibleRegion -= targetRect;
            QRect windowRect = targetRect.translated(-window->geometry().topLeft());
            QAndroidPlatformBackingStore *backingStore = static_cast<QAndroidPlatformWindow *>(window)->backingStore();
            if (backingStore)
                compositePainter.drawImage(targetRect.topLeft(), backingStore->toImage(), windowRect);
        }
    }

    for (const QRect &rect : visibleRegion)
        compositePainter.fillRect(rect, QColor(Qt::transparent));

    ret = ANativeWindow_unlockAndPost(m_nativeSurface);
    if (ret >= 0)
        m_dirtyRect = QRect();

    if (screenGrabImage) {
        if (screenGrabImage->size() != screenImage.size()) {
            uchar* bytes = static_cast<uchar*>(malloc(screenImage.height() * screenImage.bytesPerLine()));
            *screenGrabImage = QImage(bytes, screenImage.width(), screenImage.height(),
                                      screenImage.bytesPerLine(), m_pixelFormat,
                                      [](void* ptr){ if (ptr) free (ptr);});
        }
        memcpy(screenGrabImage->bits(),
               screenImage.bits(),
               screenImage.bytesPerLine() * screenImage.height());
    }
    m_repaintOccurred = true;
}

QPixmap QAndroidPlatformScreen::doScreenShot(QRect grabRect)
{
    if (!m_repaintOccurred)
       return QPixmap::fromImage(m_lastScreenshot.copy(grabRect));
    QRect tmp = m_dirtyRect;
    m_dirtyRect = geometry();
    doRedraw(&m_lastScreenshot);
    m_dirtyRect = tmp;
    m_repaintOccurred = false;
    return QPixmap::fromImage(m_lastScreenshot.copy(grabRect));
}

static const int androidLogicalDpi = 72;

QDpi QAndroidPlatformScreen::logicalDpi() const
{
    qreal lDpi = QtAndroid::pixelDensity() * androidLogicalDpi;
    return QDpi(lDpi, lDpi);
}

QDpi QAndroidPlatformScreen::logicalBaseDpi() const
{
    return QDpi(androidLogicalDpi, androidLogicalDpi);
}

Qt::ScreenOrientation QAndroidPlatformScreen::orientation() const
{
    return QAndroidPlatformIntegration::m_orientation;
}

Qt::ScreenOrientation QAndroidPlatformScreen::nativeOrientation() const
{
    return QAndroidPlatformIntegration::m_nativeOrientation;
}

void QAndroidPlatformScreen::surfaceChanged(JNIEnv *env, jobject surface, int w, int h)
{
    lockSurface();
    if (surface && w > 0  && h > 0) {
        releaseSurface();
        m_nativeSurface = ANativeWindow_fromSurface(env, surface);
        QMetaObject::invokeMethod(this, "setDirty", Qt::QueuedConnection, Q_ARG(QRect, QRect(0, 0, w, h)));
    } else {
        releaseSurface();
    }
    unlockSurface();
    m_surfaceWaitCondition.wakeOne();
}

void QAndroidPlatformScreen::releaseSurface()
{
    if (m_nativeSurface) {
        ANativeWindow_release(m_nativeSurface);
        m_nativeSurface = 0;
    }
}

/*!
    This function is called when Qt needs to be able to grab the content of a window.

    Returns the content of the window specified with the WId handle within the boundaries of
    QRect(x, y, width, height).
*/
QPixmap QAndroidPlatformScreen::grabWindow(WId window, int x, int y, int width, int height) const
{
    QRectF screenshotRect(x, y, width,  height);
    QWindow* wnd = 0;
    if (window)
    {
        const auto windowList = qApp->allWindows();
        for (QWindow *w : windowList)
            if (w->winId() == window) {
                wnd = w;
                break;
            }
    }
    if (wnd) {
        const qreal factor = logicalDpi().first / androidLogicalDpi; //HighDPI factor;
        QRectF wndRect = wnd->geometry();
        if (wnd->parent())
            wndRect.moveTopLeft(wnd->parent()->mapToGlobal(wndRect.topLeft().toPoint()));
        if (!qFuzzyCompare(factor, 1))
            wndRect = QRectF(wndRect.left() * factor, wndRect.top() * factor,
                            wndRect.width() * factor, wndRect.height() * factor);

        if (!screenshotRect.isEmpty()) {
            screenshotRect.moveTopLeft(wndRect.topLeft() + screenshotRect.topLeft());
            screenshotRect = screenshotRect.intersected(wndRect);
        } else {
            screenshotRect = wndRect;
        }
    } else {
        screenshotRect = screenshotRect.isValid() ? screenshotRect : geometry();
    }
    return const_cast<QAndroidPlatformScreen *>(this)->doScreenShot(screenshotRect.toRect());
}

QT_END_NAMESPACE
