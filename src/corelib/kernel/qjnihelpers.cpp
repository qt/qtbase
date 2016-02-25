/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include "qjnihelpers_p.h"
#include "qmutex.h"
#include "qlist.h"
#include "qsemaphore.h"
#include "qsharedpointer.h"
#include "qvector.h"
#include <QtCore/qrunnable.h>

#include <deque>

QT_BEGIN_NAMESPACE

static JavaVM *g_javaVM = Q_NULLPTR;
static jobject g_jActivity = Q_NULLPTR;
static jobject g_jService = Q_NULLPTR;
static jobject g_jClassLoader = Q_NULLPTR;
static jint g_androidSdkVersion = 0;
static jclass g_jNativeClass = Q_NULLPTR;
static jmethodID g_runPendingCppRunnablesMethodID = Q_NULLPTR;
static jmethodID g_hideSplashScreenMethodID = Q_NULLPTR;
Q_GLOBAL_STATIC(std::deque<QtAndroidPrivate::Runnable>, g_pendingRunnables);
Q_GLOBAL_STATIC(QMutex, g_pendingRunnablesMutex);

// function called from Java from Android UI thread
static void runPendingCppRunnables(JNIEnv */*env*/, jobject /*obj*/)
{
    for (;;) { // run all posted runnables
        g_pendingRunnablesMutex->lock();
        if (g_pendingRunnables->empty()) {
            g_pendingRunnablesMutex->unlock();
            break;
        }
        QtAndroidPrivate::Runnable runnable(std::move(g_pendingRunnables->front()));
        g_pendingRunnables->pop_front();
        g_pendingRunnablesMutex->unlock();
        runnable(); // run it outside the sync block!
    }
}

namespace {
    struct GenericMotionEventListeners {
        QMutex mutex;
        QVector<QtAndroidPrivate::GenericMotionEventListener *> listeners;
    };
}
Q_GLOBAL_STATIC(GenericMotionEventListeners, g_genericMotionEventListeners)

static jboolean dispatchGenericMotionEvent(JNIEnv *, jclass, jobject event)
{
    jboolean ret = JNI_FALSE;
    QMutexLocker locker(&g_genericMotionEventListeners()->mutex);
    for (auto *listener : qAsConst(g_genericMotionEventListeners()->listeners))
        ret |= listener->handleGenericMotionEvent(event);
    return ret;
}

namespace {
    struct KeyEventListeners {
        QMutex mutex;
        QVector<QtAndroidPrivate::KeyEventListener *> listeners;
    };
}
Q_GLOBAL_STATIC(KeyEventListeners, g_keyEventListeners)

static jboolean dispatchKeyEvent(JNIEnv *, jclass, jobject event)
{
    jboolean ret = JNI_FALSE;
    QMutexLocker locker(&g_keyEventListeners()->mutex);
    for (auto *listener : qAsConst(g_keyEventListeners()->listeners))
        ret |= listener->handleKeyEvent(event);
    return ret;
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

static inline bool exceptionCheck(JNIEnv *env)
{
    if (env->ExceptionCheck()) {
#ifdef QT_DEBUG
        env->ExceptionDescribe();
#endif // QT_DEBUG
        env->ExceptionClear();
        return true;
    }

    return false;
}

static void setAndroidSdkVersion(JNIEnv *env)
{
    jclass androidVersionClass = env->FindClass("android/os/Build$VERSION");
    if (exceptionCheck(env))
        return;

    jfieldID androidSDKFieldID = env->GetStaticFieldID(androidVersionClass, "SDK_INT", "I");
    if (exceptionCheck(env))
        return;

    g_androidSdkVersion = env->GetStaticIntField(androidVersionClass, androidSDKFieldID);
}

static void setNativeActivity(JNIEnv *env, jclass, jobject activity)
{
    if (g_jActivity != 0)
        env->DeleteGlobalRef(g_jActivity);

    if (activity != 0) {
        g_jActivity = env->NewGlobalRef(activity);
        env->DeleteLocalRef(activity);
    } else {
        g_jActivity = 0;
    }
}

static void setNativeService(JNIEnv *env, jclass, jobject service)
{
    if (g_jService != 0)
        env->DeleteGlobalRef(g_jService);

    if (service != 0) {
        g_jService = env->NewGlobalRef(service);
        env->DeleteLocalRef(service);
    } else {
        g_jService = 0;
    }
}

jint QtAndroidPrivate::initJNI(JavaVM *vm, JNIEnv *env)
{
    jclass jQtNative = env->FindClass("org/qtproject/qt5/android/QtNative");

    if (exceptionCheck(env))
        return JNI_ERR;

    jmethodID activityMethodID = env->GetStaticMethodID(jQtNative,
                                                        "activity",
                                                        "()Landroid/app/Activity;");

    if (exceptionCheck(env))
        return JNI_ERR;

    jobject activity = env->CallStaticObjectMethod(jQtNative, activityMethodID);

    if (exceptionCheck(env))
        return JNI_ERR;

    jmethodID serviceMethodID = env->GetStaticMethodID(jQtNative,
                                                       "service",
                                                       "()Landroid/app/Service;");

    if (exceptionCheck(env))
        return JNI_ERR;

    jobject service = env->CallStaticObjectMethod(jQtNative, serviceMethodID);

    if (exceptionCheck(env))
        return JNI_ERR;

    jmethodID classLoaderMethodID = env->GetStaticMethodID(jQtNative,
                                                           "classLoader",
                                                           "()Ljava/lang/ClassLoader;");

    if (exceptionCheck(env))
        return JNI_ERR;

    jobject classLoader = env->CallStaticObjectMethod(jQtNative, classLoaderMethodID);
    if (exceptionCheck(env))
        return JNI_ERR;

    setAndroidSdkVersion(env);

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
    g_javaVM = vm;

    static const JNINativeMethod methods[] = {
        {"runPendingCppRunnables", "()V",  reinterpret_cast<void *>(runPendingCppRunnables)},
        {"dispatchGenericMotionEvent", "(Landroid/view/MotionEvent;)Z", reinterpret_cast<void *>(dispatchGenericMotionEvent)},
        {"dispatchKeyEvent", "(Landroid/view/KeyEvent;)Z", reinterpret_cast<void *>(dispatchKeyEvent)},
        {"setNativeActivity", "(Landroid/app/Activity;)V", reinterpret_cast<void *>(setNativeActivity)},
        {"setNativeService", "(Landroid/app/Service;)V", reinterpret_cast<void *>(setNativeService)}
    };

    const bool regOk = (env->RegisterNatives(jQtNative, methods, sizeof(methods) / sizeof(methods[0])) == JNI_OK);

    if (!regOk && exceptionCheck(env))
        return JNI_ERR;

    g_runPendingCppRunnablesMethodID = env->GetStaticMethodID(jQtNative,
                                                       "runPendingCppRunnablesOnUiThread",
                                                       "()V");
    g_hideSplashScreenMethodID = env->GetStaticMethodID(jQtNative, "hideSplashScreen", "()V");
    g_jNativeClass = static_cast<jclass>(env->NewGlobalRef(jQtNative));
    env->DeleteLocalRef(jQtNative);

    return JNI_OK;
}


jobject QtAndroidPrivate::activity()
{
    return g_jActivity;
}

jobject QtAndroidPrivate::service()
{
    return g_jService;
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
    return g_androidSdkVersion;
}

void QtAndroidPrivate::runOnUiThread(QRunnable *runnable, JNIEnv *env)
{
    runOnAndroidThread([runnable]() {
        runnable->run();
        if (runnable->autoDelete())
            delete runnable;
    }, env);
}

void QtAndroidPrivate::runOnAndroidThread(const QtAndroidPrivate::Runnable &runnable, JNIEnv *env)
{
    g_pendingRunnablesMutex->lock();
    const bool triggerRun = g_pendingRunnables->empty();
    g_pendingRunnables->push_back(runnable);
    g_pendingRunnablesMutex->unlock();
    if (triggerRun)
        env->CallStaticVoidMethod(g_jNativeClass, g_runPendingCppRunnablesMethodID);
}

void QtAndroidPrivate::runOnAndroidThreadSync(const QtAndroidPrivate::Runnable &runnable, JNIEnv *env, int timeoutMs)
{
    QSharedPointer<QSemaphore> sem(new QSemaphore);
    runOnAndroidThread([&runnable, sem]{
        runnable();
        sem->release();
    }, env);
    sem->tryAcquire(1, timeoutMs);
}

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

void QtAndroidPrivate::hideSplashScreen(JNIEnv *env)
{
    env->CallStaticVoidMethod(g_jNativeClass, g_hideSplashScreenMethodID);
}

QT_END_NAMESPACE
