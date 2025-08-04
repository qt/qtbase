// Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qandroidplatformintegration.h"

#if QT_CONFIG(accessibility)
#include "androidjniaccessibility.h"
#endif
#include "androidjnimain.h"
#include "qabstracteventdispatcher.h"
#include "qandroideventdispatcher.h"
#if QT_CONFIG(accessibility)
#include "qandroidplatformaccessibility.h"
#endif
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
#include <QtGui/private/qrhibackingstore_p.h>
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

using namespace Qt::StringLiterals;

Qt::ScreenOrientation QAndroidPlatformIntegration::m_orientation = Qt::PrimaryOrientation;
Qt::ScreenOrientation QAndroidPlatformIntegration::m_nativeOrientation = Qt::PrimaryOrientation;

bool QAndroidPlatformIntegration::m_showPasswordEnabled = false;

Q_DECLARE_JNI_CLASS(QtDisplayManager, "org/qtproject/qt/android/QtDisplayManager")
Q_DECLARE_JNI_CLASS(Display, "android/view/Display")

Q_DECLARE_JNI_CLASS(List, "java/util/List")

namespace {

QAndroidPlatformScreen* createScreenForDisplayId(int displayId)
{
    const QJniObject display = QtJniTypes::QtDisplayManager::callStaticMethod<QtJniTypes::Display>(
            "getDisplay", QtAndroidPrivate::context(), displayId);
    if (!display.isValid())
        return nullptr;
    return new QAndroidPlatformScreen(display);
}

static bool isValidAndroidContextForRendering()
{
    return QtAndroid::isQtApplication() ? QtAndroidPrivate::activity().isValid()
                                        : QtAndroidPrivate::context().isValid();
}

} // anonymous namespace

void *QAndroidPlatformNativeInterface::nativeResourceForIntegration(const QByteArray &resource)
{
    if (resource=="JavaVM")
        return QtAndroidPrivate::javaVM();
    if (resource == "QtActivity") {
        extern Q_CORE_EXPORT jobject qt_androidActivity();
        return qt_androidActivity();
    }
    if (resource == "QtService") {
        extern Q_CORE_EXPORT jobject qt_androidService();
        return qt_androidService();
    }
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

#if QT_CONFIG(accessibility)
    // Android accessibility activation event might have been already received
    api->accessibility()->setActive(QtAndroidAccessibility::isActive());
#endif // QT_CONFIG(accessibility)

    api->flushPendingUpdates();
}

QAndroidPlatformIntegration::QAndroidPlatformIntegration(const QStringList &paramList)
    : m_touchDevice(nullptr)
#if QT_CONFIG(accessibility)
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

    using namespace QtJniTypes;
    m_primaryDisplayId = Display::getStaticField<jint>("DEFAULT_DISPLAY");
    const QJniObject nativeDisplaysList = QtDisplayManager::callStaticMethod<List>(
                "getAvailableDisplays", QtAndroidPrivate::context());

    const int numberOfAvailableDisplays = nativeDisplaysList.callMethod<jint>("size");
    for (int i = 0; i < numberOfAvailableDisplays; ++i) {
        const QJniObject display =
                nativeDisplaysList.callObjectMethod<jobject, jint>("get", jint(i));
        const int displayId = display.callMethod<jint>("getDisplayId");
        const bool isPrimary = (m_primaryDisplayId == displayId);
        auto screen = new QAndroidPlatformScreen(display);

        if (isPrimary)
            m_primaryScreen = screen;

        QWindowSystemInterface::handleScreenAdded(screen, isPrimary);
        m_screens[displayId] = screen;
    }

    if (numberOfAvailableDisplays == 0) {
        // If no displays are found, add a dummy display
        auto defaultScreen = new QAndroidPlatformScreen(QJniObject {});
        m_primaryScreen = defaultScreen;
        QWindowSystemInterface::handleScreenAdded(defaultScreen, true);
    }

    m_mainThread = QThread::currentThread();

    m_androidFDB = new QAndroidPlatformFontDatabase();
    m_androidPlatformServices.reset(new QAndroidPlatformServices);

#ifndef QT_NO_CLIPBOARD
    m_androidPlatformClipboard = new QAndroidPlatformClipboard();
#endif

    m_androidSystemLocale = new QAndroidSystemLocale;

#if QT_CONFIG(accessibility)
        m_accessibility = new QAndroidPlatformAccessibility();
#endif // QT_CONFIG(accessibility)

    QJniObject javaActivity = QtAndroidPrivate::activity();
    if (!javaActivity.isValid())
        javaActivity = QtAndroidPrivate::service();

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

            QWindowSystemInterface::registerInputDevice(
                    new QInputDevice("Virtual keyboard"_L1, 0, QInputDevice::DeviceType::Keyboard,
                                     {}, qApp));
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
            QtAndroid::deviceName().compare("samsung SM-T211"_L1, Qt::CaseInsensitive) == 0
            || QtAndroid::deviceName().compare("samsung SM-T210"_L1, Qt::CaseInsensitive) == 0
            || QtAndroid::deviceName().compare("samsung SM-T215"_L1, Qt::CaseInsensitive) == 0;
    return needsWorkaround;
}

void QAndroidPlatformIntegration::initialize()
{
    const auto icStrs = QPlatformInputContextFactory::requested();
    if (icStrs.isEmpty())
        m_inputContext.reset(new QAndroidInputContext);
    else
        m_inputContext.reset(QPlatformInputContextFactory::create(icStrs));
}

bool QAndroidPlatformIntegration::hasCapability(Capability cap) const
{
    switch (cap) {
        case ApplicationState: return true;
        case ThreadedPixmaps: return true;
        case NativeWidgets: return QtAndroidPrivate::activity().isValid();
        case OpenGL:
            return isValidAndroidContextForRendering();
        case ForeignWindows:
            return isValidAndroidContextForRendering();
        case ThreadedOpenGL:
            return !needsBasicRenderloopWorkaround() && isValidAndroidContextForRendering();
        case RasterGLSurface: return QtAndroidPrivate::activity().isValid();
        case TopStackedNativeChildWindows: return false;
        case MaximizeUsingFullscreenGeometry: return true;
        // FIXME QTBUG-118849 - we do not implement grabWindow() anymore, calling it will return
        // a null QPixmap also for raster windows - for OpenGL windows this was always true
        case ScreenWindowGrabbing: return false;
        default:
            return QPlatformIntegration::hasCapability(cap);
    }
}

QPlatformBackingStore *QAndroidPlatformIntegration::createPlatformBackingStore(QWindow *window) const
{
    if (!QtAndroidPrivate::activity().isValid())
        return nullptr;

    return new QRhiBackingStore(window);
}

QPlatformOpenGLContext *QAndroidPlatformIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    if (!isValidAndroidContextForRendering())
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
    if (!QtAndroidPrivate::activity().isValid())
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
    if (!QtAndroidPrivate::activity().isValid() || !nativeSurface)
        return nullptr;

    auto *surface = new QOffscreenSurface;
    auto *surfacePrivate = QOffscreenSurfacePrivate::get(surface);
    surfacePrivate->platformOffscreenSurface = new QAndroidPlatformOffscreenSurface(nativeSurface, m_eglDisplay, surface);
    return surface;
}

QPlatformWindow *QAndroidPlatformIntegration::createPlatformWindow(QWindow *window) const
{
    if (!isValidAndroidContextForRendering())
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
    return m_androidPlatformServices.data();
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

static const auto androidThemeName = "android"_L1;
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

void QAndroidPlatformIntegration::setScreenOrientation(Qt::ScreenOrientation currentOrientation,
                                                       Qt::ScreenOrientation nativeOrientation)
{
    m_orientation = currentOrientation;
    m_nativeOrientation = nativeOrientation;
}

void QAndroidPlatformIntegration::flushPendingUpdates()
{
    if (m_primaryScreen)
        m_primaryScreen->setAvailableGeometry(m_primaryScreen->availableGeometry());
}

#if QT_CONFIG(accessibility)
QPlatformAccessibility *QAndroidPlatformIntegration::accessibility() const
{
    return m_accessibility;
}
#endif

extern "C" JNIEXPORT bool JNICALL
Java_org_qtproject_qt_android_QtNativeAccessibility_accessibilitySupported(JNIEnv *, jobject)
{
    #if QT_CONFIG(accessibility)
        return true;
    #endif // QT_CONFIG(accessibility)

    return false;
}

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

Qt::ColorScheme QAndroidPlatformIntegration::m_colorScheme = Qt::ColorScheme::Light;

void QAndroidPlatformIntegration::updateColorScheme(Qt::ColorScheme colorScheme)
{
    if (m_colorScheme == colorScheme)
        return;
    m_colorScheme = colorScheme;

    QMetaObject::invokeMethod(qGuiApp,
                    [] () { QAndroidPlatformTheme::instance()->updateColorScheme();});
}

void QAndroidPlatformIntegration::setRefreshRate(qreal refreshRate)
{
    if (m_primaryScreen)
        QMetaObject::invokeMethod(m_primaryScreen, "setRefreshRate", Qt::AutoConnection,
                                  Q_ARG(qreal, refreshRate));
}

void QAndroidPlatformIntegration::handleScreenAdded(int displayId)
{
    auto result = m_screens.insert(displayId, nullptr);
    if (result.first->second == nullptr) {
        auto it = result.first;
        it->second = createScreenForDisplayId(displayId);
        if (it->second == nullptr)
            return;
        const bool isPrimary = (m_primaryDisplayId == displayId);
        if (isPrimary)
            m_primaryScreen = it->second;
        QWindowSystemInterface::handleScreenAdded(it->second, isPrimary);
    } else {
        qWarning() << "Display with id" << displayId << "already exists.";
    }
}

void QAndroidPlatformIntegration::handleScreenChanged(int displayId)
{
    auto it = m_screens.find(displayId);
    if (it == m_screens.end() || it->second == nullptr) {
        handleScreenAdded(displayId);
    }

    if (QAndroidPlatformScreen *screen = it->second) {
        QSize size = QAndroidPlatformScreen::sizeForDisplayId(displayId);
        if (screen->geometry().size() != size) {
            screen->setPhysicalSizeFromPixels(size);
            screen->setSize(size);
        }
    }

    // We do not do handle changes in rotation, refresh rate and density
    // as they are done under QtDisplayManager.
}

void QAndroidPlatformIntegration::handleScreenRemoved(int displayId)
{
    auto it = m_screens.find(displayId);

    if (it == m_screens.end())
        return;

    if (it->second != nullptr)
        QWindowSystemInterface::handleScreenRemoved(it->second);

    m_screens.erase(it);
}

#if QT_CONFIG(vulkan)

QPlatformVulkanInstance *QAndroidPlatformIntegration::createPlatformVulkanInstance(QVulkanInstance *instance) const
{
    return new QAndroidPlatformVulkanInstance(instance);
}

#endif // QT_CONFIG(vulkan)

QT_END_NAMESPACE
