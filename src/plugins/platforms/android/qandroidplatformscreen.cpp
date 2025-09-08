// Copyright (C) 2014 BogDan Vatra <bogdan@kde.org>
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QDebug>
#include <QTime>

#include <qpa/qwindowsysteminterface.h>

#include "qandroidplatformscreen.h"
#include "qandroidplatformintegration.h"
#include "qandroidplatformwindow.h"
#include "androidjnimain.h"
#include "androidjnimenu.h"

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
Q_DECLARE_JNI_CLASS(QtDisplayManager, "org/qtproject/qt/android/QtDisplayManager")
Q_DECLARE_JNI_CLASS(QtWindowInterface, "org/qtproject/qt/android/QtWindowInterface")

Q_DECLARE_JNI_CLASS(DisplayMode, "android/view/Display$Mode")

QAndroidPlatformScreen::QAndroidPlatformScreen(const QJniObject &displayObject)
    : QObject(), QPlatformScreen()
{
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
    m_size = sizeForDisplayId(m_displayId);
    m_availableGeometry = defaultAvailableGeometry();

    const auto context = QNativeInterface::QAndroidApplication::context();
    const auto resources = context.callMethod<QtJniTypes::Resources>("getResources");
    const auto metrics = resources.callMethod<QtJniTypes::DisplayMetrics>("getDisplayMetrics");
    m_xdpi = QtJniTypes::QtDisplayManager::callStaticMethod<jfloat>("getXDpi", metrics);
    m_ydpi = QtJniTypes::QtDisplayManager::callStaticMethod<jfloat>("getYDpi", metrics);

    // Potentially densityDpi could be used instead of xpdi/ydpi to do the calculation,
    // but the results are not consistent with devices specs.
    // (https://issuetracker.google.com/issues/194120500)
    setPhysicalSizeFromPixels(m_size);

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
}

QSize QAndroidPlatformScreen::sizeForDisplayId(int displayId)
{
    using namespace QtJniTypes;
    const auto context = QNativeInterface::QAndroidApplication::context();
    const auto display = QtDisplayManager::callStaticMethod<Display>(
            "getDisplay", context, displayId);
    const auto sizeObj = QtDisplayManager::callStaticMethod<Size>(
                            "getDisplaySize", context, display);

    return QSize(sizeObj.callMethod<int>("getWidth"), sizeObj.callMethod<int>("getHeight"));

}

QWindow *QAndroidPlatformScreen::topVisibleWindow() const
{
    for (QAndroidPlatformWindow *w : m_windowStack) {
        Qt::WindowType type = w->window()->type();
        if (w->window()->isVisible() &&
                (type == Qt::Window || type == Qt::Popup || type == Qt::Dialog)) {
            return w->window();
        }
    }
    return nullptr;
}

QWindow *QAndroidPlatformScreen::topLevelAt(const QPoint &p) const
{
    for (QAndroidPlatformWindow *w : m_windowStack) {
        if (w->geometry().contains(p, false) && w->window()->isVisible())
            return w->window();
    }
    return 0;
}

void QAndroidPlatformScreen::addWindow(QAndroidPlatformWindow *window)
{
    if (window->parent() && window->isRaster())
        return;

    if (m_windowStack.contains(window))
        return;

    m_windowStack.prepend(window);

    AndroidBackendRegister *reg = QtAndroid::backendRegister();
    reg->callInterface<QtJniTypes::QtWindowInterface, void>("addTopLevelWindow",
                                                            window->nativeWindow());

    if (window->window()->isVisible())
        topVisibleWindowChanged();
}

void QAndroidPlatformScreen::removeWindow(QAndroidPlatformWindow *window)
{
    m_windowStack.removeOne(window);

    if (m_windowStack.contains(window))
        qWarning() << "Failed to remove window";

    AndroidBackendRegister *reg = QtAndroid::backendRegister();
    reg->callInterface<QtJniTypes::QtWindowInterface, void>("removeTopLevelWindow",
                                                            window->nativeViewId());

    topVisibleWindowChanged();
}

void QAndroidPlatformScreen::raise(QAndroidPlatformWindow *window)
{
    int index = m_windowStack.indexOf(window);
    if (index < 0)
        return;
    if (index > 0) {
        m_windowStack.move(index, 0);

        AndroidBackendRegister *reg = QtAndroid::backendRegister();
        reg->callInterface<QtJniTypes::QtWindowInterface, void>("bringChildToFront",
                                                                window->nativeViewId());
    }
    topVisibleWindowChanged();
}

void QAndroidPlatformScreen::lower(QAndroidPlatformWindow *window)
{
    int index = m_windowStack.indexOf(window);
    if (index == -1 || index == (m_windowStack.size() - 1))
        return;
    m_windowStack.move(index, m_windowStack.size() - 1);

    AndroidBackendRegister *reg = QtAndroid::backendRegister();
    reg->callInterface<QtJniTypes::QtWindowInterface, void>("bringChildToBack",
                                                            window->nativeViewId());

    topVisibleWindowChanged();
}

void QAndroidPlatformScreen::setPhysicalSize(const QSize &size)
{
    m_physicalSize = size;
}

void QAndroidPlatformScreen::setPhysicalSizeFromPixels(const QSize &size)
{
    m_physicalSize = QSize(
        qRound(double(size.width()) / m_xdpi * 25.4),
        qRound(double(size.height()) / m_ydpi * 25.4));
}

void QAndroidPlatformScreen::setSize(const QSize &size)
{
    m_size = size;
    QWindowSystemInterface::handleScreenGeometryChange(
        QPlatformScreen::screen(), geometry(), availableGeometry());
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
    if (m_availableGeometry == rect)
        return;

    QRect oldGeometry = m_availableGeometry;

    m_availableGeometry = rect;
    m_size = sizeForDisplayId(m_displayId);
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
}

void QAndroidPlatformScreen::applicationStateChanged(Qt::ApplicationState state)
{
    for (QAndroidPlatformWindow *w : std::as_const(m_windowStack))
        w->applicationStateChanged(state);
}

void QAndroidPlatformScreen::topVisibleWindowChanged()
{
    QWindow *w = topVisibleWindow();
    QWindowSystemInterface::handleFocusWindowChanged(w, Qt::ActiveWindowFocusReason);
    QtAndroidMenu::setActiveTopLevelWindow(w);
    if (w && w->handle()) {
        QAndroidPlatformWindow *platformWindow = static_cast<QAndroidPlatformWindow *>(w->handle());
        if (platformWindow) {
            platformWindow->updateSystemUiVisibility();
            platformWindow->updateFocusedEditText();
        }
    }
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

QRect &QAndroidPlatformScreen::defaultAvailableGeometry()
{
    static QRect defaultAvailableGeometry;
    return defaultAvailableGeometry;
}
QT_END_NAMESPACE
