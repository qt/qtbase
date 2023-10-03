/****************************************************************************
**
** Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "qandroidplatformintegration.h"

#include "androidjniaccessibility.h"
#include "androidjnimain.h"
#include "qabstracteventdispatcher.h"
#include "qandroideventdispatcher.h"
#include "qandroidplatformaccessibility.h"
#include "qandroidplatformbackingstore.h"
#include "qandroidplatformclipboard.h"
#include "qandroidplatformfontdatabase.h"
#include "qandroidplatformforeignwindow.h"
#include "qandroidplatformoffscreensurface.h"
#include "qandroidplatformopenglcontext.h"
#include "qandroidplatformopenglwindow.h"
#include "qandroidplatformscreen.h"
#include "qandroidplatformservices.h"
#include "qandroidplatformtheme.h"
#include "qandroidsystemlocale.h"

#include <QGuiApplication>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QThread>
#include <QtCore/QJniObject>
#include <QtGui/private/qeglpbuffer_p.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/private/qoffscreensurface_p.h>
#include <qpa/qplatformoffscreensurface.h>
#include <qpa/qplatformwindow.h>
#include <qpa/qwindowsysteminterface.h>

#include <jni.h>

#if QT_CONFIG(vulkan)
#include "qandroidplatformvulkanwindow.h"
#include "qandroidplatformvulkaninstance.h"
#endif

#include <QtGui/qpa/qplatforminputcontextfactory_p.h>

QT_BEGIN_NAMESPACE

QSize QAndroidPlatformIntegration::m_defaultScreenSize = QSize(320, 455);
QRect QAndroidPlatformIntegration::m_defaultAvailableGeometry = QRect(0, 0, 320, 455);
QSize QAndroidPlatformIntegration::m_defaultPhysicalSize = QSize(50, 71);

Qt::ScreenOrientation QAndroidPlatformIntegration::m_orientation = Qt::PrimaryOrientation;
Qt::ScreenOrientation QAndroidPlatformIntegration::m_nativeOrientation = Qt::PrimaryOrientation;

bool QAndroidPlatformIntegration::m_showPasswordEnabled = false;
static bool m_running = false;

void *QAndroidPlatformNativeInterface::nativeResourceForIntegration(const QByteArray &resource)
{
    if (resource=="JavaVM")
        return QtAndroid::javaVM();
    if (resource == "QtActivity")
        return QtAndroid::activity();
    if (resource == "QtService")
        return QtAndroid::service();
    if (resource == "AndroidStyleData") {
        if (m_androidStyle) {
            if (m_androidStyle->m_styleData.isEmpty())
                m_androidStyle->m_styleData = AndroidStyle::loadStyleData();
            return &m_androidStyle->m_styleData;
        }
        else
            return nullptr;
    }
    if (resource == "AndroidStandardPalette") {
        if (m_androidStyle)
            return &m_androidStyle->m_standardPalette;

        return nullptr;
    }
    if (resource == "AndroidQWidgetFonts") {
        if (m_androidStyle)
            return &m_androidStyle->m_QWidgetsFonts;

        return nullptr;
    }
    if (resource == "AndroidDeviceName") {
        static QString deviceName = QtAndroid::deviceName();
        return &deviceName;
    }
    return 0;
}

void *QAndroidPlatformNativeInterface::nativeResourceForWindow(const QByteArray &resource, QWindow *window)
{
#if QT_CONFIG(vulkan)
    if (resource == "vkSurface") {
        if (window->surfaceType() == QSurface::VulkanSurface) {
            QAndroidPlatformVulkanWindow *w = static_cast<QAndroidPlatformVulkanWindow *>(window->handle());
            // return a pointer to the VkSurfaceKHR, not the value
            return w ? w->vkSurface() : nullptr;
        }
    }
#else
    Q_UNUSED(resource);
    Q_UNUSED(window);
#endif
    return nullptr;
}

void *QAndroidPlatformNativeInterface::nativeResourceForContext(const QByteArray &resource, QOpenGLContext *context)
{
    if (QEGLPlatformContext *platformContext = static_cast<QEGLPlatformContext *>(context->handle())) {
        if (resource == "eglcontext")
            return platformContext->eglContext();
        else if (resource == "eglconfig")
            return platformContext->eglConfig();
        else if (resource == "egldisplay")
            return platformContext->eglDisplay();
    }
    return nullptr;
}

void QAndroidPlatformNativeInterface::customEvent(QEvent *event)
{
    if (event->type() != QEvent::User)
        return;

    QMutexLocker lock(QtAndroid::platformInterfaceMutex());
    QAndroidPlatformIntegration *api = static_cast<QAndroidPlatformIntegration *>(QGuiApplicationPrivate::platformIntegration());
    QtAndroid::setAndroidPlatformIntegration(api);

#ifndef QT_NO_ACCESSIBILITY
    // Android accessibility activation event might have been already received
    api->accessibility()->setActive(QtAndroidAccessibility::isActive());
#endif // QT_NO_ACCESSIBILITY

    if (!m_running) {
        m_running = true;
        QtAndroid::notifyQtAndroidPluginRunning(m_running);
    }
    api->flushPendingUpdates();
}

QAndroidPlatformIntegration::QAndroidPlatformIntegration(const QStringList &paramList)
    : m_touchDevice(nullptr)
#ifndef QT_NO_ACCESSIBILITY
    , m_accessibility(nullptr)
#endif
{
    Q_UNUSED(paramList);
    m_androidPlatformNativeInterface = new QAndroidPlatformNativeInterface();

    m_eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (Q_UNLIKELY(m_eglDisplay == EGL_NO_DISPLAY))
        qFatal("Could not open egl display");

    EGLint major, minor;
    if (Q_UNLIKELY(!eglInitialize(m_eglDisplay, &major, &minor)))
        qFatal("Could not initialize egl display");

    if (Q_UNLIKELY(!eglBindAPI(EGL_OPENGL_ES_API)))
        qFatal("Could not bind GL_ES API");

    m_primaryScreen = new QAndroidPlatformScreen();
    QWindowSystemInterface::handleScreenAdded(m_primaryScreen);
    m_primaryScreen->setSizeParameters(m_defaultPhysicalSize, m_defaultScreenSize,
                                       m_defaultAvailableGeometry);

    m_mainThread = QThread::currentThread();

    m_androidFDB = new QAndroidPlatformFontDatabase();
    m_androidPlatformServices = new QAndroidPlatformServices();

#ifndef QT_NO_CLIPBOARD
    m_androidPlatformClipboard = new QAndroidPlatformClipboard();
#endif

    m_androidSystemLocale = new QAndroidSystemLocale;

#ifndef QT_NO_ACCESSIBILITY
        m_accessibility = new QAndroidPlatformAccessibility();
#endif // QT_NO_ACCESSIBILITY

    QJniObject javaActivity(QtAndroid::activity());
    if (!javaActivity.isValid())
        javaActivity = QtAndroid::service();

    if (javaActivity.isValid()) {
        QJniObject resources = javaActivity.callObjectMethod("getResources", "()Landroid/content/res/Resources;");
        QJniObject configuration = resources.callObjectMethod("getConfiguration", "()Landroid/content/res/Configuration;");

        int touchScreen = configuration.getField<jint>("touchscreen");
        if (touchScreen == QJniObject::getStaticField<jint>("android/content/res/Configuration", "TOUCHSCREEN_FINGER")
                || touchScreen == QJniObject::getStaticField<jint>("android/content/res/Configuration", "TOUCHSCREEN_STYLUS"))
        {
            QJniObject pm = javaActivity.callObjectMethod("getPackageManager", "()Landroid/content/pm/PackageManager;");
            Q_ASSERT(pm.isValid());
            int maxTouchPoints = 1;
            if (pm.callMethod<jboolean>("hasSystemFeature","(Ljava/lang/String;)Z",
                                     QJniObject::getStaticObjectField("android/content/pm/PackageManager",
                                                                      "FEATURE_TOUCHSCREEN_MULTITOUCH_JAZZHAND",
                                                                      "Ljava/lang/String;").object())) {
                maxTouchPoints = 10;
            } else if (pm.callMethod<jboolean>("hasSystemFeature","(Ljava/lang/String;)Z",
                                            QJniObject::getStaticObjectField("android/content/pm/PackageManager",
                                                                             "FEATURE_TOUCHSCREEN_MULTITOUCH_DISTINCT",
                                                                             "Ljava/lang/String;").object())) {
                maxTouchPoints = 4;
            } else if (pm.callMethod<jboolean>("hasSystemFeature","(Ljava/lang/String;)Z",
                                            QJniObject::getStaticObjectField("android/content/pm/PackageManager",
                                                                             "FEATURE_TOUCHSCREEN_MULTITOUCH",
                                                                             "Ljava/lang/String;").object())) {
                maxTouchPoints = 2;
            }

            m_touchDevice = new QPointingDevice("Android touchscreen", 1,
                                                QInputDevice::DeviceType::TouchScreen,
                                                QPointingDevice::PointerType::Finger,
                                                QPointingDevice::Capability::Position
                                                    | QPointingDevice::Capability::Area
                                                    | QPointingDevice::Capability::Pressure
                                                    | QPointingDevice::Capability::NormalizedPosition,
                                                maxTouchPoints,
                                                0);
            QWindowSystemInterface::registerInputDevice(m_touchDevice);
        }

        auto contentResolver = javaActivity.callObjectMethod("getContentResolver", "()Landroid/content/ContentResolver;");
        Q_ASSERT(contentResolver.isValid());
        QJniObject txtShowPassValue = QJniObject::callStaticObjectMethod(
                                                        "android/provider/Settings$System",
                                                        "getString",
                                                        "(Landroid/content/ContentResolver;Ljava/lang/String;)Ljava/lang/String;",
                                                        contentResolver.object(),
                                                        QJniObject::getStaticObjectField("android/provider/Settings$System",
                                                                                         "TEXT_SHOW_PASSWORD",
                                                                                         "Ljava/lang/String;").object());
        if (txtShowPassValue.isValid()) {
            bool ok = false;
            const int txtShowPass = txtShowPassValue.toString().toInt(&ok);
            m_showPasswordEnabled = ok ? (txtShowPass == 1) : false;
        }
    }

    // We can't safely notify the jni bridge that we're up and running just yet, so let's postpone
    // it for now.
    QCoreApplication::postEvent(m_androidPlatformNativeInterface, new QEvent(QEvent::User));
}

static bool needsBasicRenderloopWorkaround()
{
    static bool needsWorkaround =
            QtAndroid::deviceName().compare(QLatin1String("samsung SM-T211"), Qt::CaseInsensitive) == 0
            || QtAndroid::deviceName().compare(QLatin1String("samsung SM-T210"), Qt::CaseInsensitive) == 0
            || QtAndroid::deviceName().compare(QLatin1String("samsung SM-T215"), Qt::CaseInsensitive) == 0;
    return needsWorkaround;
}

void QAndroidPlatformIntegration::initialize()
{
    const QString icStr = QPlatformInputContextFactory::requested();
    if (icStr.isNull())
        m_inputContext.reset(new QAndroidInputContext);
    else
        m_inputContext.reset(QPlatformInputContextFactory::create(icStr));
}

bool QAndroidPlatformIntegration::hasCapability(Capability cap) const
{
    switch (cap) {
        case ApplicationState: return true;
        case ThreadedPixmaps: return true;
        case NativeWidgets: return QtAndroid::activity();
        case OpenGL: return QtAndroid::activity();
        case ForeignWindows: return QtAndroid::activity();
        case ThreadedOpenGL: return !needsBasicRenderloopWorkaround() && QtAndroid::activity();
        case RasterGLSurface: return QtAndroid::activity();
        case TopStackedNativeChildWindows: return false;
        case MaximizeUsingFullscreenGeometry: return true;
        default:
            return QPlatformIntegration::hasCapability(cap);
    }
}

QPlatformBackingStore *QAndroidPlatformIntegration::createPlatformBackingStore(QWindow *window) const
{
    if (!QtAndroid::activity())
        return nullptr;

    return new QAndroidPlatformBackingStore(window);
}

QPlatformOpenGLContext *QAndroidPlatformIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    if (!QtAndroid::activity())
        return nullptr;
    QSurfaceFormat format(context->format());
    format.setAlphaBufferSize(8);
    format.setRedBufferSize(8);
    format.setGreenBufferSize(8);
    format.setBlueBufferSize(8);
    auto ctx = new QAndroidPlatformOpenGLContext(format, context->shareHandle(), m_eglDisplay);
    return ctx;
}

QOpenGLContext *QAndroidPlatformIntegration::createOpenGLContext(EGLContext context, EGLDisplay display, QOpenGLContext *shareContext) const
{
    return QEGLPlatformContext::createFrom<QAndroidPlatformOpenGLContext>(context, display, m_eglDisplay, shareContext);
}

QPlatformOffscreenSurface *QAndroidPlatformIntegration::createPlatformOffscreenSurface(QOffscreenSurface *surface) const
{
    if (!QtAndroid::activity())
        return nullptr;

    QSurfaceFormat format(surface->requestedFormat());
    format.setAlphaBufferSize(8);
    format.setRedBufferSize(8);
    format.setGreenBufferSize(8);
    format.setBlueBufferSize(8);

    return new QEGLPbuffer(m_eglDisplay, format, surface);
}

QOffscreenSurface *QAndroidPlatformIntegration::createOffscreenSurface(ANativeWindow *nativeSurface) const
{
    if (!QtAndroid::activity() || !nativeSurface)
        return nullptr;

    auto *surface = new QOffscreenSurface;
    auto *surfacePrivate = QOffscreenSurfacePrivate::get(surface);
    surfacePrivate->platformOffscreenSurface = new QAndroidPlatformOffscreenSurface(nativeSurface, m_eglDisplay, surface);
    return surface;
}

QPlatformWindow *QAndroidPlatformIntegration::createPlatformWindow(QWindow *window) const
{
    if (!QtAndroid::activity())
        return nullptr;

#if QT_CONFIG(vulkan)
    if (window->surfaceType() == QSurface::VulkanSurface)
        return new QAndroidPlatformVulkanWindow(window);
#endif

    return new QAndroidPlatformOpenGLWindow(window, m_eglDisplay);
}

QPlatformWindow *QAndroidPlatformIntegration::createForeignWindow(QWindow *window, WId nativeHandle) const
{
    return new QAndroidPlatformForeignWindow(window, nativeHandle);
}

QAbstractEventDispatcher *QAndroidPlatformIntegration::createEventDispatcher() const
{
    return new QAndroidEventDispatcher;
}

QAndroidPlatformIntegration::~QAndroidPlatformIntegration()
{
    if (m_eglDisplay != EGL_NO_DISPLAY)
        eglTerminate(m_eglDisplay);

    delete m_androidPlatformNativeInterface;
    delete m_androidFDB;
    delete m_androidSystemLocale;

#ifndef QT_NO_CLIPBOARD
    delete m_androidPlatformClipboard;
#endif

    QtAndroid::setAndroidPlatformIntegration(NULL);
}

QPlatformFontDatabase *QAndroidPlatformIntegration::fontDatabase() const
{
    return m_androidFDB;
}

#ifndef QT_NO_CLIPBOARD
QPlatformClipboard *QAndroidPlatformIntegration::clipboard() const
{
    return m_androidPlatformClipboard;
}
#endif

QPlatformInputContext *QAndroidPlatformIntegration::inputContext() const
{
    return m_inputContext.data();
}

QPlatformNativeInterface *QAndroidPlatformIntegration::nativeInterface() const
{
    return m_androidPlatformNativeInterface;
}

QPlatformServices *QAndroidPlatformIntegration::services() const
{
    return m_androidPlatformServices;
}

QVariant QAndroidPlatformIntegration::styleHint(StyleHint hint) const
{
    switch (hint) {
    case PasswordMaskDelay:
        // this number is from a hard-coded value in Android code (cf. PasswordTransformationMethod)
        return m_showPasswordEnabled ? 1500 : 0;
    case ShowIsMaximized:
        return true;
    default:
        return QPlatformIntegration::styleHint(hint);
    }
}

Qt::WindowState QAndroidPlatformIntegration::defaultWindowState(Qt::WindowFlags flags) const
{
    // Don't maximize dialogs on Android
    if (flags & Qt::Dialog & ~Qt::Window)
        return Qt::WindowNoState;

    return QPlatformIntegration::defaultWindowState(flags);
}

static const QLatin1String androidThemeName("android");
QStringList QAndroidPlatformIntegration::themeNames() const
{
    return QStringList(QString(androidThemeName));
}

QPlatformTheme *QAndroidPlatformIntegration::createPlatformTheme(const QString &name) const
{
    if (androidThemeName == name)
        return QAndroidPlatformTheme::instance(m_androidPlatformNativeInterface);

    return 0;
}

void QAndroidPlatformIntegration::setDefaultDisplayMetrics(int availableLeft, int availableTop,
                                                           int availableWidth, int availableHeight,
                                                           int physicalWidth, int physicalHeight,
                                                           int screenWidth, int screenHeight)
{
    m_defaultAvailableGeometry = QRect(availableLeft, availableTop,
                                       availableWidth, availableHeight);
    m_defaultPhysicalSize = QSize(physicalWidth, physicalHeight);
    m_defaultScreenSize = QSize(screenWidth, screenHeight);
}

void QAndroidPlatformIntegration::setScreenOrientation(Qt::ScreenOrientation currentOrientation,
                                                       Qt::ScreenOrientation nativeOrientation)
{
    m_orientation = currentOrientation;
    m_nativeOrientation = nativeOrientation;
}

void QAndroidPlatformIntegration::flushPendingUpdates()
{
    if (m_primaryScreen) {
        m_primaryScreen->setSizeParameters(m_defaultPhysicalSize, m_defaultScreenSize,
                                           m_defaultAvailableGeometry);
    }
}

#ifndef QT_NO_ACCESSIBILITY
QPlatformAccessibility *QAndroidPlatformIntegration::accessibility() const
{
    return m_accessibility;
}
#endif

void QAndroidPlatformIntegration::setAvailableGeometry(const QRect &availableGeometry)
{
    if (m_primaryScreen)
        QMetaObject::invokeMethod(m_primaryScreen, "setAvailableGeometry", Qt::AutoConnection, Q_ARG(QRect, availableGeometry));
}

void QAndroidPlatformIntegration::setPhysicalSize(int width, int height)
{
    if (m_primaryScreen)
        QMetaObject::invokeMethod(m_primaryScreen, "setPhysicalSize", Qt::AutoConnection, Q_ARG(QSize, QSize(width, height)));
}

void QAndroidPlatformIntegration::setScreenSize(int width, int height)
{
    if (m_primaryScreen)
        QMetaObject::invokeMethod(m_primaryScreen, "setSize", Qt::AutoConnection, Q_ARG(QSize, QSize(width, height)));
}

QPlatformTheme::Appearance QAndroidPlatformIntegration::m_appearance = QPlatformTheme::Appearance::Light;

void QAndroidPlatformIntegration::setAppearance(QPlatformTheme::Appearance newAppearance)
{
    if (m_appearance == newAppearance)
        return;
    m_appearance = newAppearance;

    QMetaObject::invokeMethod(qGuiApp,
                    [] () { QAndroidPlatformTheme::instance()->updateAppearance();});
}

void QAndroidPlatformIntegration::setScreenSizeParameters(const QSize &physicalSize,
                                                          const QSize &screenSize,
                                                          const QRect &availableGeometry)
{
    if (m_primaryScreen) {
        QMetaObject::invokeMethod(m_primaryScreen, "setSizeParameters", Qt::AutoConnection,
                                  Q_ARG(QSize, physicalSize), Q_ARG(QSize, screenSize),
                                  Q_ARG(QRect, availableGeometry));
    }
}

void QAndroidPlatformIntegration::setRefreshRate(qreal refreshRate)
{
    if (m_primaryScreen)
        QMetaObject::invokeMethod(m_primaryScreen, "setRefreshRate", Qt::AutoConnection,
                                  Q_ARG(qreal, refreshRate));
}
#if QT_CONFIG(vulkan)

QPlatformVulkanInstance *QAndroidPlatformIntegration::createPlatformVulkanInstance(QVulkanInstance *instance) const
{
    return new QAndroidPlatformVulkanInstance(instance);
}

#endif // QT_CONFIG(vulkan)

QT_END_NAMESPACE
