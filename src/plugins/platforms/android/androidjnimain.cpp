// Copyright (C) 2014 BogDan Vatra <bogdan@kde.org>
// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <dlfcn.h>
#include <pthread.h>
#include <qplugin.h>
#include <semaphore.h>

#include "androidcontentfileengine.h"
#include "qandroidapkfileengine.h"
#include "androidjniinput.h"
#include "androidjnimain.h"
#include "androidjnimenu.h"
#include "androidwindowembedding.h"
#include "qandroidassetsfileenginehandler.h"
#include "qandroideventdispatcher.h"
#include "qandroidplatformdialoghelpers.h"
#include "qandroidplatformintegration.h"
#if QT_CONFIG(clipboard)
#include "qandroidplatformclipboard.h"
#endif
#if QT_CONFIG(accessibility)
#include "androidjniaccessibility.h"
#endif
#include "qandroidplatformscreen.h"
#include "qandroidplatformwindow.h"

#include <android/api-level.h>
#include <android/asset_manager_jni.h>
#include <android/bitmap.h>

#include <QtCore/private/qjnihelpers_p.h>
#include <QtCore/qbasicatomic.h>
#include <QtCore/qjnienvironment.h>
#include <QtCore/qjniobject.h>
#include <QtCore/qprocess.h>
#include <QtCore/qresource.h>
#include <QtCore/qscopeguard.h>
#include <QtCore/qthread.h>
#include <QtCore/private/qandroiditemmodelproxy_p.h>
#include <QtCore/private/qandroidmodelindexproxy_p.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/private/qhighdpiscaling_p.h>

#include <qpa/qwindowsysteminterface.h>


using namespace Qt::StringLiterals;

QT_BEGIN_NAMESPACE

static jclass m_applicationClass  = nullptr;
static AAssetManager *m_assetManager = nullptr;
static jobject m_assets = nullptr;
static jobject m_resourcesObj = nullptr;

static jclass m_qtActivityClass = nullptr;
static jclass m_qtServiceClass = nullptr;

static int m_pendingApplicationState = -1;
static QBasicMutex m_platformMutex;

static jclass m_bitmapClass  = nullptr;
static jmethodID m_createBitmapMethodID = nullptr;
static jobject m_ARGB_8888_BitmapConfigValue = nullptr;
static jobject m_RGB_565_BitmapConfigValue = nullptr;

static jclass m_bitmapDrawableClass = nullptr;
static jmethodID m_bitmapDrawableConstructorMethodID = nullptr;

extern "C" typedef int (*Main)(int, char **); //use the standard main method to start the application
static Main m_main = nullptr;
static void *m_mainLibraryHnd = nullptr;
static QList<QByteArray> m_applicationParams;
static sem_t m_exitSemaphore, m_terminateSemaphore;

static QAndroidPlatformIntegration *m_androidPlatformIntegration = nullptr;

static double m_density = 1.0;

static AndroidAssetsFileEngineHandler *m_androidAssetsFileEngineHandler = nullptr;
static AndroidContentFileEngineHandler *m_androidContentFileEngineHandler = nullptr;
static QAndroidApkFileEngineHandler *m_androidApkFileEngineHandler = nullptr;

static AndroidBackendRegister *m_backendRegister = nullptr;

static const char m_qtTag[] = "Qt";
static const char m_classErrorMsg[] = "Can't find class \"%s\"";
static const char m_methodErrorMsg[] = "Can't find method \"%s%s\"";

Q_CONSTINIT static QBasicAtomicInt startQtAndroidPluginCalled = Q_BASIC_ATOMIC_INITIALIZER(0);

#if QT_CONFIG(accessibility)
Q_DECLARE_JNI_CLASS(QtAccessibilityInterface, "org/qtproject/qt/android/QtAccessibilityInterface");
#endif

namespace QtAndroid
{
    QBasicMutex *platformInterfaceMutex()
    {
        return &m_platformMutex;
    }

    void setAndroidPlatformIntegration(QAndroidPlatformIntegration *androidPlatformIntegration)
    {
        m_androidPlatformIntegration = androidPlatformIntegration;
        QtAndroid::notifyNativePluginIntegrationReady((bool)m_androidPlatformIntegration);

        // flush the pending state if necessary.
        if (m_androidPlatformIntegration && (m_pendingApplicationState != -1)) {
            if (m_pendingApplicationState == Qt::ApplicationActive)
                QtAndroidPrivate::handleResume();
            else if (m_pendingApplicationState == Qt::ApplicationInactive)
                QtAndroidPrivate::handlePause();
            QWindowSystemInterface::handleApplicationStateChanged(Qt::ApplicationState(m_pendingApplicationState));
        }

        m_pendingApplicationState = -1;
    }

    QAndroidPlatformIntegration *androidPlatformIntegration()
    {
        return m_androidPlatformIntegration;
    }

    QWindow *topLevelWindowAt(const QPoint &globalPos)
    {
        return m_androidPlatformIntegration
               ? m_androidPlatformIntegration->screen()->topLevelAt(globalPos)
               : 0;
    }

    QWindow *windowFromId(int windowId)
    {
        if (!qGuiApp)
            return nullptr;

        for (QWindow *w : qGuiApp->allWindows()) {
            if (!w->handle())
                continue;
            QAndroidPlatformWindow *window = static_cast<QAndroidPlatformWindow *>(w->handle());
            if (window->nativeViewId() == windowId)
                return w;
        }
        return nullptr;
    }

    double pixelDensity()
    {
        return m_density;
    }

    AAssetManager *assetManager()
    {
        return m_assetManager;
    }

    jclass applicationClass()
    {
        return m_applicationClass;
    }

    bool isQtApplication()
    {
        // Returns true if the app is a Qt app, i.e. Qt controls the whole app and
        // the Activity/Service is created by Qt. Returns false if instead Qt is
        // embedded into a native Android app, where the Activity/Service is created
        // by the user, outside of Qt, and Qt content is added as a view.
        JNIEnv *env = QJniEnvironment::getJniEnv();
        auto activity = QtAndroidPrivate::activity();
        if (activity.isValid())
            return env->IsInstanceOf(activity.object(), m_qtActivityClass);
        auto service = QtAndroidPrivate::service();
        if (service.isValid())
            return env->IsInstanceOf(QtAndroidPrivate::service().object(), m_qtServiceClass);
        // return true as default as Qt application is our default use case.
        // famous last words: we should not end up here
        return true;
    }

#if QT_CONFIG(accessibility)
    void notifyAccessibilityLocationChange(uint accessibilityObjectId)
    {
        m_backendRegister->callInterface<QtJniTypes::QtAccessibilityInterface, void>(
                "notifyLocationChange", accessibilityObjectId);
    }

    void notifyObjectHide(uint accessibilityObjectId, uint parentObjectId)
    {
        m_backendRegister->callInterface<QtJniTypes::QtAccessibilityInterface, void>(
                "notifyObjectHide", accessibilityObjectId, parentObjectId);
    }

    void notifyObjectShow(uint parentObjectId)
    {
        m_backendRegister->callInterface<QtJniTypes::QtAccessibilityInterface, void>(
                "notifyObjectShow", parentObjectId);
    }

    void notifyObjectFocus(uint accessibilityObjectId)
    {
        m_backendRegister->callInterface<QtJniTypes::QtAccessibilityInterface, void>(
                "notifyObjectFocus", accessibilityObjectId);
    }

    void notifyValueChanged(uint accessibilityObjectId, jstring value)
    {
        m_backendRegister->callInterface<QtJniTypes::QtAccessibilityInterface, void>(
                "notifyValueChanged", accessibilityObjectId, value);
    }

    void notifyDescriptionOrNameChanged(uint accessibilityObjectId, const QString &value)
    {
        m_backendRegister->callInterface<QtJniTypes::QtAccessibilityInterface, void>(
                "notifyDescriptionOrNameChanged", accessibilityObjectId, value);
    }

    void notifyScrolledEvent(uint accessibilityObjectId)
    {
        m_backendRegister->callInterface<QtJniTypes::QtAccessibilityInterface, void>(
                "notifyScrolledEvent", accessibilityObjectId);
    }

    void notifyAnnouncementEvent(uint accessibilityObjectId, const QString &message)
    {
        m_backendRegister->callInterface<QtJniTypes::QtAccessibilityInterface, void>(
                "notifyAnnouncementEvent", accessibilityObjectId, message);
    }
#endif //QT_CONFIG(accessibility)

    void notifyNativePluginIntegrationReady(bool ready)
    {
        QJniObject::callStaticMethod<void>(m_applicationClass,
                                           "notifyNativePluginIntegrationReady",
                                           ready);
    }

    jobject createBitmap(QImage img, JNIEnv *env)
    {
        if (!m_bitmapClass)
            return 0;

        if (img.format() != QImage::Format_RGBA8888 && img.format() != QImage::Format_RGB16)
            img = std::move(img).convertToFormat(QImage::Format_RGBA8888);

        jobject bitmap = env->CallStaticObjectMethod(m_bitmapClass,
                                                     m_createBitmapMethodID,
                                                     img.width(),
                                                     img.height(),
                                                     img.format() == QImage::Format_RGBA8888
                                                        ? m_ARGB_8888_BitmapConfigValue
                                                        : m_RGB_565_BitmapConfigValue);
        if (!bitmap)
            return 0;

        AndroidBitmapInfo info;
        if (AndroidBitmap_getInfo(env, bitmap, &info) < 0) {
            env->DeleteLocalRef(bitmap);
            return 0;
        }

        void *pixels;
        if (AndroidBitmap_lockPixels(env, bitmap, &pixels) < 0) {
            env->DeleteLocalRef(bitmap);
            return 0;
        }

        if (info.stride == uint(img.bytesPerLine())
                && info.width == uint(img.width())
                && info.height == uint(img.height())) {
            memcpy(pixels, img.constBits(), info.stride * info.height);
        } else {
            uchar *bmpPtr = static_cast<uchar *>(pixels);
            const unsigned width = qMin(info.width, (uint)img.width()); //should be the same
            const unsigned height = qMin(info.height, (uint)img.height()); //should be the same
            for (unsigned y = 0; y < height; y++, bmpPtr += info.stride)
                memcpy(bmpPtr, img.constScanLine(y), width);
        }
        AndroidBitmap_unlockPixels(env, bitmap);
        return bitmap;
    }

    jobject createBitmap(int width, int height, QImage::Format format, JNIEnv *env)
    {
        if (format != QImage::Format_RGBA8888
                && format != QImage::Format_RGB16)
            return 0;

        return env->CallStaticObjectMethod(m_bitmapClass,
                                                     m_createBitmapMethodID,
                                                     width,
                                                     height,
                                                     format == QImage::Format_RGB16
                                                        ? m_RGB_565_BitmapConfigValue
                                                        : m_ARGB_8888_BitmapConfigValue);
    }

    jobject createBitmapDrawable(jobject bitmap, JNIEnv *env)
    {
        if (!bitmap || !m_bitmapDrawableClass || !m_resourcesObj)
            return 0;

        return env->NewObject(m_bitmapDrawableClass,
                              m_bitmapDrawableConstructorMethodID,
                              m_resourcesObj,
                              bitmap);
    }

    const char *classErrorMsgFmt()
    {
        return m_classErrorMsg;
    }

    const char *methodErrorMsgFmt()
    {
        return m_methodErrorMsg;
    }

    const char *qtTagText()
    {
        return m_qtTag;
    }

    QString deviceName()
    {
        QString manufacturer = QJniObject::getStaticObjectField("android/os/Build", "MANUFACTURER", "Ljava/lang/String;").toString();
        QString model = QJniObject::getStaticObjectField("android/os/Build", "MODEL", "Ljava/lang/String;").toString();

        return manufacturer + u' ' + model;
    }

    void setViewVisibility(jobject view, bool visible)
    {
        QJniObject::callStaticMethod<void>(m_applicationClass,
                                           "setViewVisibility",
                                           "(Landroid/view/View;Z)V",
                                           view,
                                           visible);
    }

    bool blockEventLoopsWhenSuspended()
    {
        static bool block = qEnvironmentVariableIntValue("QT_BLOCK_EVENT_LOOPS_WHEN_SUSPENDED");
        return block;
    }

    jobject assets()
    {
        return m_assets;
    }

    AndroidBackendRegister *backendRegister()
    {
        return m_backendRegister;
    }

} // namespace QtAndroid

static bool initJavaReferences(QJniEnvironment &env);

static jboolean startQtAndroidPlugin(JNIEnv *env, jobject /*object*/, jstring paramsString)
{
    Q_UNUSED(env)
    // Init all the Java refs, if they haven't already been initialized. They get initialized
    // when the library is loaded, but in case Qt is terminated, they are cleared, and in case
    // Qt is then started again JNI_OnLoad will not be called again, since the library is already
    // loaded - in that case we need to init again here, hence the check.
    // TODO QTBUG-130614 QtCore also inits some Java references in qjnihelpers - we probably
    // want to reset those, too.
    QJniEnvironment qEnv;
    if (!qEnv.isValid()) {
        __android_log_print(ANDROID_LOG_FATAL, "Qt", "Failed to initialize the JNI Environment");
        return JNI_ERR;
    }
    if (!initJavaReferences(qEnv))
        return false;

    m_androidPlatformIntegration = nullptr;
    // File engine handler instantiation registers the handler
    m_androidAssetsFileEngineHandler = new AndroidAssetsFileEngineHandler();
    m_androidContentFileEngineHandler = new AndroidContentFileEngineHandler();
    m_androidApkFileEngineHandler = new QAndroidApkFileEngineHandler();
    m_mainLibraryHnd = nullptr;
    m_backendRegister = new AndroidBackendRegister();

    const QStringList argsList = QProcess::splitCommand(QJniObject(paramsString).toString());

    for (const QString &arg : argsList)
        m_applicationParams.append(arg.toUtf8());

    // Go home
    QDir::setCurrent(QDir::homePath());

    //look for main()
    if (m_applicationParams.length()) {
        // Obtain a handle to the main library (the library that contains the main() function).
        // This library should already be loaded, and calling dlopen() will just return a reference to it.
        m_mainLibraryHnd = dlopen(m_applicationParams.constFirst().data(), 0);
        if (Q_UNLIKELY(!m_mainLibraryHnd)) {
            qCritical() << "dlopen failed:" << dlerror();
            return false;
        }
        m_main = (Main)dlsym(m_mainLibraryHnd, "main");
    } else {
        qWarning("No main library was specified; searching entire process (this is slow!)");
        m_main = (Main)dlsym(RTLD_DEFAULT, "main");
    }

    if (Q_UNLIKELY(!m_main)) {
        qCritical() << "dlsym failed:" << dlerror() << Qt::endl
                    << "Could not find main method";
        return false;
    }

    if (sem_init(&m_exitSemaphore, 0, 0) == -1)
        return false;

    if (sem_init(&m_terminateSemaphore, 0, 0) == -1)
        return false;

    return true;
}

static void waitForServiceSetup(JNIEnv *env, jclass /*clazz*/)
{
    Q_UNUSED(env);
    // The service must wait until the QCoreApplication starts otherwise onBind will be
    // called too early
    if (QtAndroidPrivate::service().isValid() && QtAndroid::isQtApplication())
        QtAndroidPrivate::waitForServiceSetup();
}

static void startQtApplication(JNIEnv */*env*/, jclass /*clazz*/)
{
    {
        JNIEnv* env = nullptr;
        JavaVMAttachArgs args;
        args.version = JNI_VERSION_1_6;
        args.name = "QtMainThread";
        args.group = NULL;
        JavaVM *vm = QJniEnvironment::javaVM();
        if (vm)
            vm->AttachCurrentThread(&env, &args);
    }

    // Register type for invokeMethod() calls.
    qRegisterMetaType<Qt::ScreenOrientation>("Qt::ScreenOrientation");

    // Register resources if they are available
    if (QFile{QStringLiteral("assets:/android_rcc_bundle.rcc")}.exists())
        QResource::registerResource(QStringLiteral("assets:/android_rcc_bundle.rcc"));

    const int argc = m_applicationParams.size();
    QVarLengthArray<char *> argv(argc + 1);
    for (int i = 0; i < argc; i++)
        argv[i] = m_applicationParams[i].data();
    argv[argc] = nullptr;

    startQtAndroidPluginCalled.fetchAndAddRelease(1);
    const int ret = m_main(argc, argv.data());
    qInfo() << "main() returned" << ret;

    if (m_mainLibraryHnd) {
        int res = dlclose(m_mainLibraryHnd);
        if (res < 0)
            qWarning() << "dlclose failed:" << dlerror();
    }

    if (m_applicationClass) {
        const auto quitMethodName = QtAndroid::isQtApplication() ? "quitApp" : "quitQt";
        QJniObject::callStaticMethod<void>(m_applicationClass, quitMethodName);
    }

    sem_post(&m_terminateSemaphore);
    sem_wait(&m_exitSemaphore);
    sem_destroy(&m_exitSemaphore);

    // We must call exit() to ensure that all global objects will be destructed
    if (!qEnvironmentVariableIsSet("QT_ANDROID_NO_EXIT_CALL"))
        exit(ret);
}

static void quitQtCoreApplication(JNIEnv *env, jclass /*clazz*/)
{
    Q_UNUSED(env);
    QCoreApplication::quit();
}

static void quitQtAndroidPlugin(JNIEnv *env, jclass /*clazz*/)
{
    Q_UNUSED(env);
    m_androidPlatformIntegration = nullptr;
    delete m_androidAssetsFileEngineHandler;
    m_androidAssetsFileEngineHandler = nullptr;
    delete m_androidContentFileEngineHandler;
    m_androidContentFileEngineHandler = nullptr;
    delete m_androidApkFileEngineHandler;
    m_androidApkFileEngineHandler = nullptr;
}

static void clearJavaReferences(JNIEnv *env)
{
    if (m_applicationClass) {
        env->DeleteGlobalRef(m_applicationClass);
        m_applicationClass = nullptr;
    }
    if (m_resourcesObj) {
        env->DeleteGlobalRef(m_resourcesObj);
        m_resourcesObj = nullptr;
    }
    if (m_bitmapClass) {
        env->DeleteGlobalRef(m_bitmapClass);
        m_bitmapClass = nullptr;
    }
    if (m_ARGB_8888_BitmapConfigValue) {
        env->DeleteGlobalRef(m_ARGB_8888_BitmapConfigValue);
        m_ARGB_8888_BitmapConfigValue = nullptr;
    }
    if (m_RGB_565_BitmapConfigValue) {
        env->DeleteGlobalRef(m_RGB_565_BitmapConfigValue);
        m_RGB_565_BitmapConfigValue = nullptr;
    }
    if (m_bitmapDrawableClass) {
        env->DeleteGlobalRef(m_bitmapDrawableClass);
        m_bitmapDrawableClass = nullptr;
    }
    if (m_assets) {
        env->DeleteGlobalRef(m_assets);
        m_assets = nullptr;
    }
    if (m_qtActivityClass) {
        env->DeleteGlobalRef(m_qtActivityClass);
        m_qtActivityClass = nullptr;
    }
    if (m_qtServiceClass) {
        env->DeleteGlobalRef(m_qtServiceClass);
        m_qtServiceClass = nullptr;
    }
}

static void terminateQt(JNIEnv *env, jclass /*clazz*/)
{
    // QAndroidEventDispatcherStopper is stopped when the user uses the task manager to kill the application
    if (QAndroidEventDispatcherStopper::instance()->stopped()) {
        QAndroidEventDispatcherStopper::instance()->startAll();
        QCoreApplication::quit();
        QAndroidEventDispatcherStopper::instance()->goingToStop(false);
    }

    if (startQtAndroidPluginCalled.loadAcquire())
        sem_wait(&m_terminateSemaphore);

    sem_destroy(&m_terminateSemaphore);

    clearJavaReferences(env);

    m_androidPlatformIntegration = nullptr;
    delete m_androidAssetsFileEngineHandler;
    m_androidAssetsFileEngineHandler = nullptr;
    delete m_androidContentFileEngineHandler;
    m_androidContentFileEngineHandler = nullptr;
    delete m_androidApkFileEngineHandler;
    m_androidApkFileEngineHandler = nullptr;
    delete m_backendRegister;
    m_backendRegister = nullptr;
    sem_post(&m_exitSemaphore);
}

static void handleLayoutSizeChanged(JNIEnv * /*env*/, jclass /*clazz*/,
                                    jint availableWidth, jint availableHeight)
{
    QMutexLocker lock(&m_platformMutex);
    // available geometry always starts from top left
    const QRect availableGeometry(0, 0, availableWidth, availableHeight);
    if (m_androidPlatformIntegration)
        m_androidPlatformIntegration->setAvailableGeometry(availableGeometry);
    else if (QAndroidPlatformScreen::defaultAvailableGeometry().isNull())
        QAndroidPlatformScreen::defaultAvailableGeometry() = availableGeometry;
}
Q_DECLARE_JNI_NATIVE_METHOD(handleLayoutSizeChanged)

static void updateApplicationState(JNIEnv */*env*/, jobject /*thiz*/, jint state)
{
    QMutexLocker lock(&m_platformMutex);
    if (!m_main || !m_androidPlatformIntegration) {
        m_pendingApplicationState = state;
        return;
    }

    // We're about to call user code from the Android thread, since we don't know
    //the side effects we'll unlock first!
    lock.unlock();
    if (state == Qt::ApplicationActive)
        QtAndroidPrivate::handleResume();
    else if (state == Qt::ApplicationInactive)
        QtAndroidPrivate::handlePause();
    lock.relock();
    if (!m_androidPlatformIntegration)
        return;

    if (state <= Qt::ApplicationInactive) {
        // NOTE: sometimes we will receive two consecutive suspended notifications,
        // In the second suspended notification, QWindowSystemInterface::flushWindowSystemEvents()
        // will deadlock since the dispatcher has been stopped in the first suspended notification.
        // To avoid the deadlock we simply return if we found the event dispatcher has been stopped.
        if (QAndroidEventDispatcherStopper::instance()->stopped())
            return;

        // Don't send timers and sockets events anymore if we are going to hide all windows
        QAndroidEventDispatcherStopper::instance()->goingToStop(true);
        QWindowSystemInterface::handleApplicationStateChanged(Qt::ApplicationState(state));
        if (state == Qt::ApplicationSuspended)
            QAndroidEventDispatcherStopper::instance()->stopAll();
    } else {
        QAndroidEventDispatcherStopper::instance()->startAll();
        QWindowSystemInterface::handleApplicationStateChanged(Qt::ApplicationState(state));
        QAndroidEventDispatcherStopper::instance()->goingToStop(false);
    }
}

static void updateLocale(JNIEnv */*env*/, jobject /*thiz*/)
{
    QCoreApplication::postEvent(QCoreApplication::instance(), new QEvent(QEvent::LocaleChange));
    QCoreApplication::postEvent(QCoreApplication::instance(), new QEvent(QEvent::LanguageChange));
}

static void handleOrientationChanged(JNIEnv */*env*/, jobject /*thiz*/, jint newRotation, jint nativeOrientation)
{
    // Array of orientations rotated in 90 degree increments, counterclockwise
    // (same direction as Android measures angles)
    static const Qt::ScreenOrientation orientations[] = {
        Qt::PortraitOrientation,
        Qt::LandscapeOrientation,
        Qt::InvertedPortraitOrientation,
        Qt::InvertedLandscapeOrientation
    };

    // The Android API defines the following constants:
    // ROTATION_0 :   0
    // ROTATION_90 :  1
    // ROTATION_180 : 2
    // ROTATION_270 : 3
    // ORIENTATION_PORTRAIT :  1
    // ORIENTATION_LANDSCAPE : 2

    // and newRotation is how much the current orientation is rotated relative to nativeOrientation

    // which means that we can be really clever here :)
    Qt::ScreenOrientation screenOrientation = orientations[(nativeOrientation - 1 + newRotation) % 4];
    Qt::ScreenOrientation native = orientations[nativeOrientation - 1];

    QAndroidPlatformIntegration::setScreenOrientation(screenOrientation, native);
    QMutexLocker lock(&m_platformMutex);
    if (m_androidPlatformIntegration) {
        QAndroidPlatformScreen *screen = m_androidPlatformIntegration->screen();
        // Use invokeMethod to keep the certain order of the "geometry change"
        // and "orientation change" event handling.
        if (screen) {
            QMetaObject::invokeMethod(screen, "setOrientation", Qt::AutoConnection,
                                      Q_ARG(Qt::ScreenOrientation, screenOrientation));
        }
    }
}
Q_DECLARE_JNI_NATIVE_METHOD(handleOrientationChanged)

static void handleRefreshRateChanged(JNIEnv */*env*/, jclass /*cls*/, jfloat refreshRate)
{
    if (m_androidPlatformIntegration)
        m_androidPlatformIntegration->setRefreshRate(refreshRate);
}
Q_DECLARE_JNI_NATIVE_METHOD(handleRefreshRateChanged)

static void handleScreenAdded(JNIEnv */*env*/, jclass /*cls*/, jint displayId)
{
    if (m_androidPlatformIntegration)
        m_androidPlatformIntegration->handleScreenAdded(displayId);
}
Q_DECLARE_JNI_NATIVE_METHOD(handleScreenAdded)

static void handleScreenChanged(JNIEnv */*env*/, jclass /*cls*/, jint displayId)
{
    if (m_androidPlatformIntegration)
        m_androidPlatformIntegration->handleScreenChanged(displayId);
}
Q_DECLARE_JNI_NATIVE_METHOD(handleScreenChanged)

static void handleScreenRemoved(JNIEnv */*env*/, jclass /*cls*/, jint displayId)
{
    if (m_androidPlatformIntegration)
        m_androidPlatformIntegration->handleScreenRemoved(displayId);
}
Q_DECLARE_JNI_NATIVE_METHOD(handleScreenRemoved)

static void handleUiDarkModeChanged(JNIEnv */*env*/, jobject /*thiz*/, jint newUiMode)
{
    QAndroidPlatformIntegration::updateColorScheme(
        (newUiMode == 1 ) ? Qt::ColorScheme::Dark : Qt::ColorScheme::Light);
}
Q_DECLARE_JNI_NATIVE_METHOD(handleUiDarkModeChanged)

static void handleScreenDensityChanged(JNIEnv */*env*/, jclass /*cls*/, jdouble density)
{
    m_density = density;
}
Q_DECLARE_JNI_NATIVE_METHOD(handleScreenDensityChanged)

static void onActivityResult(JNIEnv */*env*/, jclass /*cls*/,
                             jint requestCode,
                             jint resultCode,
                             jobject data)
{
    QtAndroidPrivate::handleActivityResult(requestCode, resultCode, data);
}

static void onNewIntent(JNIEnv *env, jclass /*cls*/, jobject data)
{
    QtAndroidPrivate::handleNewIntent(env, data);
}

static jobject onBind(JNIEnv */*env*/, jclass /*cls*/, jobject intent)
{
    return QtAndroidPrivate::callOnBindListener(intent);
}

static JNINativeMethod methods[] = {
    { "startQtAndroidPlugin", "(Ljava/lang/String;)Z", (void *)startQtAndroidPlugin },
    { "startQtApplication", "()V", (void *)startQtApplication },
    { "quitQtAndroidPlugin", "()V", (void *)quitQtAndroidPlugin },
    { "quitQtCoreApplication", "()V", (void *)quitQtCoreApplication },
    { "terminateQt", "()V", (void *)terminateQt },
    { "waitForServiceSetup", "()V", (void *)waitForServiceSetup },
    { "updateApplicationState", "(I)V", (void *)updateApplicationState },
    { "onActivityResult", "(IILandroid/content/Intent;)V", (void *)onActivityResult },
    { "onNewIntent", "(Landroid/content/Intent;)V", (void *)onNewIntent },
    { "onBind", "(Landroid/content/Intent;)Landroid/os/IBinder;", (void *)onBind },
    { "updateLocale", "()V", (void *)updateLocale },
};

#define FIND_AND_CHECK_CLASS(CLASS_NAME) \
clazz = env->FindClass(CLASS_NAME); \
if (!clazz) { \
    __android_log_print(ANDROID_LOG_FATAL, m_qtTag, m_classErrorMsg, CLASS_NAME); \
    return false; \
}

#define GET_AND_CHECK_METHOD(VAR, CLASS, METHOD_NAME, METHOD_SIGNATURE) \
VAR = env->GetMethodID(CLASS, METHOD_NAME, METHOD_SIGNATURE); \
if (!VAR) { \
    __android_log_print(ANDROID_LOG_FATAL, m_qtTag, m_methodErrorMsg, METHOD_NAME, METHOD_SIGNATURE); \
    return false; \
}

#define GET_AND_CHECK_STATIC_METHOD(VAR, CLASS, METHOD_NAME, METHOD_SIGNATURE) \
VAR = env->GetStaticMethodID(CLASS, METHOD_NAME, METHOD_SIGNATURE); \
if (!VAR) { \
    __android_log_print(ANDROID_LOG_FATAL, m_qtTag, m_methodErrorMsg, METHOD_NAME, METHOD_SIGNATURE); \
    return false; \
}

#define GET_AND_CHECK_FIELD(VAR, CLASS, FIELD_NAME, FIELD_SIGNATURE) \
VAR = env->GetFieldID(CLASS, FIELD_NAME, FIELD_SIGNATURE); \
if (!VAR) { \
    __android_log_print(ANDROID_LOG_FATAL, m_qtTag, m_methodErrorMsg, FIELD_NAME, FIELD_SIGNATURE); \
    return false; \
}

#define GET_AND_CHECK_STATIC_FIELD(VAR, CLASS, FIELD_NAME, FIELD_SIGNATURE) \
VAR = env->GetStaticFieldID(CLASS, FIELD_NAME, FIELD_SIGNATURE); \
if (!VAR) { \
    __android_log_print(ANDROID_LOG_FATAL, m_qtTag, m_methodErrorMsg, FIELD_NAME, FIELD_SIGNATURE); \
    return false; \
}

Q_DECLARE_JNI_CLASS(QtDisplayManager, "org/qtproject/qt/android/QtDisplayManager")

static bool registerNatives(QJniEnvironment &env)
{
    bool success = env.registerNativeMethods(m_applicationClass,
                   methods, sizeof(methods) / sizeof(methods[0]));
    success &= env.registerNativeMethods(
            QtJniTypes::Traits<QtJniTypes::QtDisplayManager>::className(),
            {
                    Q_JNI_NATIVE_METHOD(handleLayoutSizeChanged),
                    Q_JNI_NATIVE_METHOD(handleOrientationChanged),
                    Q_JNI_NATIVE_METHOD(handleRefreshRateChanged),
                    Q_JNI_NATIVE_METHOD(handleScreenAdded),
                    Q_JNI_NATIVE_METHOD(handleScreenChanged),
                    Q_JNI_NATIVE_METHOD(handleScreenRemoved),
                    Q_JNI_NATIVE_METHOD(handleUiDarkModeChanged),
                    Q_JNI_NATIVE_METHOD(handleScreenDensityChanged)
            });

    success = success
        && QtAndroidInput::registerNatives(env)
        && QtAndroidMenu::registerNatives(env)
#if QT_CONFIG(accessibility)
        && QtAndroidAccessibility::registerNatives(env)
#endif
        && QtAndroidDialogHelpers::registerNatives(env)
#if QT_CONFIG(clipboard)
        && QAndroidPlatformClipboard::registerNatives(env)
#endif
        && QAndroidPlatformWindow::registerNatives(env)
        && QtAndroidWindowEmbedding::registerNatives(env)
        && AndroidBackendRegister::registerNatives()
        && QAndroidModelIndexProxy::registerNatives(env)
        && QAndroidItemModelProxy::registerAbstractNatives(env)
        && QAndroidItemModelProxy::registerProxyNatives(env);

    return success;
}

static bool initJavaReferences(QJniEnvironment &env)
{
    if (m_applicationClass)
        return true;

    jclass clazz;
    FIND_AND_CHECK_CLASS("org/qtproject/qt/android/QtNative");
    m_applicationClass = static_cast<jclass>(env->NewGlobalRef(clazz));

    jmethodID methodID;
    GET_AND_CHECK_STATIC_METHOD(methodID, m_applicationClass, "activity", "()Landroid/app/Activity;");

    jobject contextObject = env->CallStaticObjectMethod(m_applicationClass, methodID);
    if (!contextObject) {
        GET_AND_CHECK_STATIC_METHOD(methodID, m_applicationClass, "service", "()Landroid/app/Service;");
        contextObject = env->CallStaticObjectMethod(m_applicationClass, methodID);
    }

    if (!contextObject) {
        __android_log_print(ANDROID_LOG_FATAL,"Qt", "Failed to get Activity or Service object");
        return false;
    }
    const auto releaseContextObject = qScopeGuard([&env, contextObject]{
        env->DeleteLocalRef(contextObject);
    });

    FIND_AND_CHECK_CLASS("android/content/ContextWrapper");
    GET_AND_CHECK_METHOD(methodID, clazz, "getAssets", "()Landroid/content/res/AssetManager;");
    m_assets = env->NewGlobalRef(env->CallObjectMethod(contextObject, methodID));
    m_assetManager = AAssetManager_fromJava(env.jniEnv(), m_assets);

    GET_AND_CHECK_METHOD(methodID, clazz, "getResources", "()Landroid/content/res/Resources;");
    m_resourcesObj = env->NewGlobalRef(env->CallObjectMethod(contextObject, methodID));

    FIND_AND_CHECK_CLASS("android/graphics/Bitmap");
    m_bitmapClass = static_cast<jclass>(env->NewGlobalRef(clazz));
    GET_AND_CHECK_STATIC_METHOD(m_createBitmapMethodID, m_bitmapClass,
                                "createBitmap", "(IILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;");
    FIND_AND_CHECK_CLASS("android/graphics/Bitmap$Config");
    jfieldID fieldId;
    GET_AND_CHECK_STATIC_FIELD(fieldId, clazz, "ARGB_8888", "Landroid/graphics/Bitmap$Config;");
    m_ARGB_8888_BitmapConfigValue = env->NewGlobalRef(env->GetStaticObjectField(clazz, fieldId));
    GET_AND_CHECK_STATIC_FIELD(fieldId, clazz, "RGB_565", "Landroid/graphics/Bitmap$Config;");
    m_RGB_565_BitmapConfigValue = env->NewGlobalRef(env->GetStaticObjectField(clazz, fieldId));

    FIND_AND_CHECK_CLASS("android/graphics/drawable/BitmapDrawable");
    m_bitmapDrawableClass = static_cast<jclass>(env->NewGlobalRef(clazz));
    GET_AND_CHECK_METHOD(m_bitmapDrawableConstructorMethodID,
                         m_bitmapDrawableClass,
                         "<init>", "(Landroid/content/res/Resources;Landroid/graphics/Bitmap;)V");

    FIND_AND_CHECK_CLASS("org/qtproject/qt/android/QtActivityBase");
    m_qtActivityClass = static_cast<jclass>(env->NewGlobalRef(clazz));
    FIND_AND_CHECK_CLASS("org/qtproject/qt/android/QtServiceBase");
    m_qtServiceClass = static_cast<jclass>(env->NewGlobalRef(clazz));

    // The current thread will be the Qt thread, name it accordingly
    QThread::currentThread()->setObjectName("QtMainLoopThread");

    QWindowSystemInterfacePrivate::TabletEvent::setPlatformSynthesizesMouse(false);

    return true;
}

QT_END_NAMESPACE

Q_DECL_EXPORT jint JNICALL JNI_OnLoad(JavaVM */*vm*/, void */*reserved*/)
{
    static bool initialized = false;
    if (initialized)
        return JNI_VERSION_1_6;
    initialized = true;

    QT_USE_NAMESPACE

    QJniEnvironment env;
    if (!env.isValid()) {
        __android_log_print(ANDROID_LOG_FATAL, "Qt", "Failed to initialize the JNI Environment");
        return JNI_ERR;
    }

    if (!initJavaReferences(env))
        return JNI_ERR;

    if (!registerNatives(env)) {
        __android_log_print(ANDROID_LOG_FATAL, "Qt", "registerNatives failed");
        return JNI_ERR;
    }

    __android_log_print(ANDROID_LOG_INFO, "Qt", "Qt platform plugin started");
    return JNI_VERSION_1_6;
}
