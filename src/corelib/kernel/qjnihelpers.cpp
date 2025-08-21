// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qjnihelpers_p.h"

#include "qjnienvironment.h"
#include "qjniobject.h"
#include "qlist.h"
#include "qmutex.h"
#include "qsemaphore.h"
#include "qreadwritelock.h"
#include <QtCore/private/qcoreapplication_p.h>
#include <QtCore/private/qlocking_p.h>

#if QT_CONFIG(regularexpression)
#include <QtCore/qregularexpression.h>
#endif

#include <android/log.h>
#include <deque>
#include <memory>

QT_BEGIN_NAMESPACE

Q_DECLARE_JNI_CLASS(QtLoader, "org/qtproject/qt/android/QtLoader")
Q_DECLARE_JNI_CLASS(QtInputDelegate, "org/qtproject/qt/android/QtInputDelegate");
Q_DECLARE_JNI_CLASS(MotionEvent, "android/view/MotionEvent");
Q_DECLARE_JNI_CLASS(KeyEvent, "android/view/KeyEvent");

using namespace Qt::StringLiterals;

namespace QtAndroidPrivate {
    // *Listener virtual function implementations.
    // Defined out-of-line to pin the vtable/type_info.
    ActivityResultListener::~ActivityResultListener() {}
    NewIntentListener::~NewIntentListener() {}
    ResumePauseListener::~ResumePauseListener() {}
    void ResumePauseListener::handlePause() {}
    void ResumePauseListener::handleResume() {}
}

static JavaVM *g_javaVM = nullptr;
static jobject g_jActivity = nullptr;
static jobject g_jService = nullptr;
static jobject g_jClassLoader = nullptr;

Q_CONSTINIT static QtAndroidPrivate::OnBindListener *g_onBindListener;
Q_CONSTINIT static QBasicMutex g_onBindListenerMutex;
Q_GLOBAL_STATIC(QSemaphore, g_waitForServiceSetupSemaphore);
Q_CONSTINIT static QBasicAtomicInt g_serviceSetupLockers = Q_BASIC_ATOMIC_INITIALIZER(0);

Q_GLOBAL_STATIC(QReadWriteLock, g_updateMutex);

static jboolean updateNativeActivity(JNIEnv *env, jclass = nullptr)
{

    jclass jQtNative = env->FindClass("org/qtproject/qt/android/QtNative");
    if (QJniEnvironment::checkAndClearExceptions(env))
        return JNI_FALSE;

    jmethodID activityMethodID =
            env->GetStaticMethodID(jQtNative, "activity", "()Landroid/app/Activity;");
    if (QJniEnvironment::checkAndClearExceptions(env))
        return JNI_FALSE;

    jobject activity = env->CallStaticObjectMethod(jQtNative, activityMethodID);
    if (QJniEnvironment::checkAndClearExceptions(env))
        return JNI_FALSE;

    QWriteLocker locker(g_updateMutex());

    if (g_jActivity) {
        env->DeleteGlobalRef(g_jActivity);
        g_jActivity = nullptr;
    }

    if (activity) {
        g_jActivity = env->NewGlobalRef(activity);
        env->DeleteLocalRef(activity);
    }

    env->DeleteLocalRef(jQtNative);
    if (QJniEnvironment::checkAndClearExceptions(env))
        return JNI_FALSE;

    return JNI_TRUE;
}

namespace {
    class ActivityResultListeners
    {
    public:
        QMutex mutex;
        QList<QtAndroidPrivate::ActivityResultListener *> listeners;
    };
}

Q_GLOBAL_STATIC(ActivityResultListeners, g_activityResultListeners)

void QtAndroidPrivate::registerActivityResultListener(ActivityResultListener *listener)
{
    QMutexLocker locker(&g_activityResultListeners()->mutex);
    g_activityResultListeners()->listeners.append(listener);
}

void QtAndroidPrivate::unregisterActivityResultListener(ActivityResultListener *listener)
{
    QMutexLocker locker(&g_activityResultListeners()->mutex);
    g_activityResultListeners()->listeners.removeAll(listener);
}

void QtAndroidPrivate::handleActivityResult(jint requestCode, jint resultCode, jobject data)
{
    QMutexLocker locker(&g_activityResultListeners()->mutex);
    const QList<QtAndroidPrivate::ActivityResultListener *> &listeners = g_activityResultListeners()->listeners;
    for (int i=0; i<listeners.size(); ++i) {
        if (listeners.at(i)->handleActivityResult(requestCode, resultCode, data))
            break;
    }
}

namespace {
    class NewIntentListeners
    {
    public:
        QMutex mutex;
        QList<QtAndroidPrivate::NewIntentListener *> listeners;
    };
}

Q_GLOBAL_STATIC(NewIntentListeners, g_newIntentListeners)

void QtAndroidPrivate::registerNewIntentListener(NewIntentListener *listener)
{
    QMutexLocker locker(&g_newIntentListeners()->mutex);
    g_newIntentListeners()->listeners.append(listener);
}

void QtAndroidPrivate::unregisterNewIntentListener(NewIntentListener *listener)
{
    QMutexLocker locker(&g_newIntentListeners()->mutex);
    g_newIntentListeners()->listeners.removeAll(listener);
}

void QtAndroidPrivate::handleNewIntent(JNIEnv *env, jobject intent)
{
    QMutexLocker locker(&g_newIntentListeners()->mutex);
    const QList<QtAndroidPrivate::NewIntentListener *> &listeners = g_newIntentListeners()->listeners;
    for (int i=0; i<listeners.size(); ++i) {
        if (listeners.at(i)->handleNewIntent(env, intent))
            break;
    }
}

QtAndroidPrivate::GenericMotionEventListener::~GenericMotionEventListener() {}
namespace {
struct GenericMotionEventListeners {
    QMutex mutex;
    QList<QtAndroidPrivate::GenericMotionEventListener *> listeners;
};
}
Q_GLOBAL_STATIC(GenericMotionEventListeners, g_genericMotionEventListeners)

static jboolean dispatchGenericMotionEvent(JNIEnv *, jclass, QtJniTypes::MotionEvent event)
{
    jboolean ret = JNI_FALSE;
    QMutexLocker locker(&g_genericMotionEventListeners()->mutex);
    for (auto *listener : std::as_const(g_genericMotionEventListeners()->listeners))
        ret |= listener->handleGenericMotionEvent(event.object());
    return ret;
}
Q_DECLARE_JNI_NATIVE_METHOD(dispatchGenericMotionEvent);

QtAndroidPrivate::KeyEventListener::~KeyEventListener() {}
namespace {
struct KeyEventListeners {
    QMutex mutex;
    QList<QtAndroidPrivate::KeyEventListener *> listeners;
};
}
Q_GLOBAL_STATIC(KeyEventListeners, g_keyEventListeners)

static jboolean dispatchKeyEvent(JNIEnv *, jclass, QtJniTypes::KeyEvent event)
{
    jboolean ret = JNI_FALSE;
    QMutexLocker locker(&g_keyEventListeners()->mutex);
    for (auto *listener : std::as_const(g_keyEventListeners()->listeners))
        ret |= listener->handleKeyEvent(event.object());
    return ret;
}
Q_DECLARE_JNI_NATIVE_METHOD(dispatchKeyEvent);

void QtAndroidPrivate::registerGenericMotionEventListener(QtAndroidPrivate::GenericMotionEventListener *listener)
{
    QMutexLocker locker(&g_genericMotionEventListeners()->mutex);
    g_genericMotionEventListeners()->listeners.push_back(listener);
}

void QtAndroidPrivate::unregisterGenericMotionEventListener(QtAndroidPrivate::GenericMotionEventListener *listener)
{
    QMutexLocker locker(&g_genericMotionEventListeners()->mutex);
    g_genericMotionEventListeners()->listeners.removeOne(listener);
}

void QtAndroidPrivate::registerKeyEventListener(QtAndroidPrivate::KeyEventListener *listener)
{
    QMutexLocker locker(&g_keyEventListeners()->mutex);
    g_keyEventListeners()->listeners.push_back(listener);
}

void QtAndroidPrivate::unregisterKeyEventListener(QtAndroidPrivate::KeyEventListener *listener)
{
    QMutexLocker locker(&g_keyEventListeners()->mutex);
    g_keyEventListeners()->listeners.removeOne(listener);
}

namespace {
    class ResumePauseListeners
    {
    public:
        QMutex mutex;
        QList<QtAndroidPrivate::ResumePauseListener *> listeners;
    };
}

Q_GLOBAL_STATIC(ResumePauseListeners, g_resumePauseListeners)

void QtAndroidPrivate::registerResumePauseListener(ResumePauseListener *listener)
{
    QMutexLocker locker(&g_resumePauseListeners()->mutex);
    g_resumePauseListeners()->listeners.append(listener);
}

void QtAndroidPrivate::unregisterResumePauseListener(ResumePauseListener *listener)
{
    QMutexLocker locker(&g_resumePauseListeners()->mutex);
    g_resumePauseListeners()->listeners.removeAll(listener);
}

void QtAndroidPrivate::handlePause()
{
    QMutexLocker locker(&g_resumePauseListeners()->mutex);
    const QList<QtAndroidPrivate::ResumePauseListener *> &listeners = g_resumePauseListeners()->listeners;
    for (int i=0; i<listeners.size(); ++i)
        listeners.at(i)->handlePause();
}

void QtAndroidPrivate::handleResume()
{
    QMutexLocker locker(&g_resumePauseListeners()->mutex);
    const QList<QtAndroidPrivate::ResumePauseListener *> &listeners = g_resumePauseListeners()->listeners;
    for (int i=0; i<listeners.size(); ++i)
        listeners.at(i)->handleResume();
}

bool QtAndroidPrivate::isUncompressedNativeLibs()
{
    const static bool isUncompressed = QtJniTypes::QtLoader::callStaticMethod<bool>(
                "isUncompressedNativeLibs");
    return isUncompressed;
}

QString QtAndroidPrivate::resolveApkPath(const QString &fileName)
{
#if QT_CONFIG(regularexpression)
    const static QRegularExpression inApkRegex("(.+\\.apk)!\\/.+"_L1);
    auto match = inApkRegex.matchView(fileName);
    if (match.hasMatch())
        return match.captured(1);
#else
    if (int index = fileName.lastIndexOf(u".apk!/"); index > 0)
        return fileName.mid(0, index + 4);
#endif

    return {};
}

jint QtAndroidPrivate::initJNI(JavaVM *vm, JNIEnv *env)
{
    g_javaVM = vm;

    jclass jQtNative = env->FindClass("org/qtproject/qt/android/QtNative");

    if (QJniEnvironment::checkAndClearExceptions(env))
        return JNI_ERR;

    jmethodID activityMethodID = env->GetStaticMethodID(jQtNative,
                                                        "activity",
                                                        "()Landroid/app/Activity;");

    if (QJniEnvironment::checkAndClearExceptions(env))
        return JNI_ERR;

    jobject activity = env->CallStaticObjectMethod(jQtNative, activityMethodID);

    if (QJniEnvironment::checkAndClearExceptions(env))
        return JNI_ERR;

    jmethodID serviceMethodID = env->GetStaticMethodID(jQtNative,
                                                       "service",
                                                       "()Landroid/app/Service;");

    if (QJniEnvironment::checkAndClearExceptions(env))
        return JNI_ERR;

    jobject service = env->CallStaticObjectMethod(jQtNative, serviceMethodID);

    if (QJniEnvironment::checkAndClearExceptions(env))
        return JNI_ERR;

    jmethodID classLoaderMethodID = env->GetStaticMethodID(jQtNative,
                                                           "classLoader",
                                                           "()Ljava/lang/ClassLoader;");

    if (QJniEnvironment::checkAndClearExceptions(env))
        return JNI_ERR;

    jobject classLoader = env->CallStaticObjectMethod(jQtNative, classLoaderMethodID);
    if (QJniEnvironment::checkAndClearExceptions(env))
        return JNI_ERR;

    g_jClassLoader = env->NewGlobalRef(classLoader);
    env->DeleteLocalRef(classLoader);
    if (activity) {
        g_jActivity = env->NewGlobalRef(activity);
        env->DeleteLocalRef(activity);
    }
    if (service) {
        g_jService = env->NewGlobalRef(service);
        env->DeleteLocalRef(service);
    }

    static const JNINativeMethod methods[] = {
        {"updateNativeActivity", "()Z", reinterpret_cast<void *>(updateNativeActivity) },
    };

    const bool regOk = (env->RegisterNatives(jQtNative, methods, sizeof(methods) / sizeof(methods[0])) == JNI_OK);
    env->DeleteLocalRef(jQtNative);
    if (!regOk && QJniEnvironment::checkAndClearExceptions(env))
        return JNI_ERR;

    QJniEnvironment qJniEnv;
    using namespace QtJniTypes;
    if (!QtInputDelegate::registerNativeMethods(
                { Q_JNI_NATIVE_METHOD(dispatchGenericMotionEvent),
                  Q_JNI_NATIVE_METHOD(dispatchKeyEvent) })) {
        qCritical() << "Failed to register natives methods for"
                    << Traits<QtInputDelegate>::className();
        return JNI_ERR;
    }

#if QT_CONFIG(permissions)
    if (!registerPermissionNatives(qJniEnv))
        return JNI_ERR;
#endif

    if (!registerNativeInterfaceNatives(qJniEnv))
        return JNI_ERR;

    if (!registerExtrasNatives(qJniEnv))
        return JNI_ERR;

    return JNI_OK;
}

Q_CORE_EXPORT jobject qt_androidActivity()
{
    QReadLocker locker(g_updateMutex());
    return g_jActivity;
}


QtJniTypes::Activity QtAndroidPrivate::activity()
{
    QReadLocker locker(g_updateMutex());
    return g_jActivity;
}

Q_CORE_EXPORT jobject qt_androidService()
{
    return g_jService;
}

QtJniTypes::Service QtAndroidPrivate::service()
{
    return g_jService;
}

QtJniTypes::Context QtAndroidPrivate::context()
{
    QReadLocker locker(g_updateMutex());
    if (g_jActivity)
        return g_jActivity;
    if (g_jService)
        return g_jService;

    return nullptr;
}

JavaVM *QtAndroidPrivate::javaVM()
{
    return g_javaVM;
}

jobject QtAndroidPrivate::classLoader()
{
    return g_jClassLoader;
}

jint QtAndroidPrivate::androidSdkVersion()
{
    static jint sdkVersion = 0;
    if (!sdkVersion)
        sdkVersion = QJniObject::getStaticField<jint>("android/os/Build$VERSION", "SDK_INT");
    return sdkVersion;
}

void QtAndroidPrivate::waitForServiceSetup()
{
    g_waitForServiceSetupSemaphore->acquire();
}

int QtAndroidPrivate::acuqireServiceSetup(int flags)
{
    g_serviceSetupLockers.ref();
    return flags;
}

void QtAndroidPrivate::setOnBindListener(QtAndroidPrivate::OnBindListener *listener)
{
    const auto lock = qt_scoped_lock(g_onBindListenerMutex);
    g_onBindListener = listener;
    if (!g_serviceSetupLockers.deref())
        g_waitForServiceSetupSemaphore->release();
}

jobject QtAndroidPrivate::callOnBindListener(jobject intent)
{
    const auto lock = qt_scoped_lock(g_onBindListenerMutex);
    if (g_onBindListener)
        return g_onBindListener->onBind(intent);
    return nullptr;
}

Q_CONSTINIT static QBasicAtomicInt g_androidDeadlockProtector = Q_BASIC_ATOMIC_INITIALIZER(0);

bool QtAndroidPrivate::acquireAndroidDeadlockProtector()
{
    return g_androidDeadlockProtector.testAndSetAcquire(0, 1);
}

void QtAndroidPrivate::releaseAndroidDeadlockProtector()
{
    g_androidDeadlockProtector.storeRelease(0);
}

QtAndroidPrivate::AndroidDeadlockProtector::AndroidDeadlockProtector(const QString &lockedBy)
    : m_lockedBy(lockedBy)
{ }

QtAndroidPrivate::AndroidDeadlockProtector::~AndroidDeadlockProtector() {
    if (m_acquired) {
        QtAndroidPrivate::releaseAndroidDeadlockProtector();
        s_lockers.removeOne(m_lockedBy);
    }
}

bool QtAndroidPrivate::AndroidDeadlockProtector::acquire() {
    m_acquired = QtAndroidPrivate::acquireAndroidDeadlockProtector();
    if (m_acquired) {
        s_lockers.append(m_lockedBy);
    } else {
        qWarning("Failed to acquire deadlock protector for '%s' while already locked by '%s'.",
                 qPrintable(m_lockedBy), qPrintable(s_lockers.join(u',')));
    }
    return m_acquired;
}

QT_END_NAMESPACE

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved)
{
    Q_UNUSED(reserved);

    static const char logTag[] = "QtCore";
    static bool initialized = false;
    if (initialized)
        return JNI_VERSION_1_6;
    initialized = true;

    typedef union {
        JNIEnv *nenv;
        void *venv;
    } _JNIEnv;

    __android_log_print(ANDROID_LOG_INFO, logTag, "Start");

    _JNIEnv uenv;
    uenv.venv = nullptr;

    if (vm->GetEnv(&uenv.venv, JNI_VERSION_1_6) != JNI_OK) {
        __android_log_print(ANDROID_LOG_FATAL, logTag, "GetEnv failed");
        return JNI_ERR;
    }

    JNIEnv *env = uenv.nenv;
    const jint ret = QT_PREPEND_NAMESPACE(QtAndroidPrivate::initJNI(vm, env));
    if (ret != 0) {
        __android_log_print(ANDROID_LOG_FATAL, logTag, "initJNI failed");
        return ret;
    }

    return JNI_VERSION_1_6;
}
