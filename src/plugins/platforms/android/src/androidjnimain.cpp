/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <dlfcn.h>
#include <pthread.h>
#include <qcoreapplication.h>
#include <qimage.h>
#include <qpoint.h>
#include <qplugin.h>
#include <qsemaphore.h>
#include <qmutex.h>
#include <qdebug.h>
#include <qglobal.h>
#include <qobjectdefs.h>
#include <stdlib.h>

#include "androidjnimain.h"
#include "androidjniinput.h"
#include "androidjniclipboard.h"
#include "androidjnimenu.h"
#include "qandroidplatformintegration.h"
#include <QtWidgets/QApplication>

#include <qabstracteventdispatcher.h>

#include <android/bitmap.h>
#include <android/asset_manager_jni.h>
#include "qandroidassetsfileenginehandler.h"
#include <android/api-level.h>

#include <qpa/qwindowsysteminterface.h>

#ifdef ANDROID_PLUGIN_OPENGL
#  include "qandroidopenglplatformwindow.h"
#endif

#include <android/native_window_jni.h>

static jmethodID m_redrawSurfaceMethodID = 0;

Q_IMPORT_PLUGIN(QAndroidPlatformIntegrationPlugin)

static JavaVM *m_javaVM = NULL;
static jclass m_applicationClass  = NULL;
static jobject m_classLoaderObject = NULL;
static jmethodID m_loadClassMethodID = NULL;
static AAssetManager *m_assetManager = NULL;
static jobject m_resourcesObj;
static jobject m_activityObject = NULL;

static jclass m_bitmapClass  = 0;
static jmethodID m_createBitmapMethodID = 0;
static jobject m_ARGB_8888_BitmapConfigValue = 0;
static jobject m_RGB_565_BitmapConfigValue = 0;

static jclass m_bitmapDrawableClass = 0;
static jmethodID m_bitmapDrawableConstructorMethodID = 0;

extern "C" typedef int (*Main)(int, char **); //use the standard main method to start the application
static Main m_main = NULL;
static void *m_mainLibraryHnd = NULL;
static QList<QByteArray> m_applicationParams;

#ifndef ANDROID_PLUGIN_OPENGL
static jobject m_surface = NULL;
#else
static EGLNativeWindowType m_nativeWindow = 0;
static QSemaphore m_waitForWindowSemaphore;
static bool m_waitForWindow = false;

static jfieldID m_surfaceFieldID = 0;
#endif


static QSemaphore m_quitAppSemaphore;
static QMutex m_surfaceMutex(QMutex::Recursive);
static QSemaphore m_pauseApplicationSemaphore;
static QMutex m_pauseApplicationMutex;

static QAndroidPlatformIntegration *m_androidPlatformIntegration = 0;

static int m_desktopWidthPixels  = 0;
static int m_desktopHeightPixels = 0;
static double m_scaledDensity = 0;

static volatile bool m_pauseApplication;

static AndroidAssetsFileEngineHandler *m_androidAssetsFileEngineHandler = 0;



static const char m_qtTag[] = "Qt";
static const char m_classErrorMsg[] = "Can't find class \"%s\"";
static const char m_methodErrorMsg[] = "Can't find method \"%s%s\"";

static inline void checkPauseApplication()
{
    m_pauseApplicationMutex.lock();
    if (m_pauseApplication) {
        m_pauseApplicationMutex.unlock();
        m_pauseApplicationSemaphore.acquire(); // wait until surface is created

        m_pauseApplicationMutex.lock();
        m_pauseApplication = false;
        m_pauseApplicationMutex.unlock();

        //FIXME
//        QWindowSystemInterface::handleScreenAvailableGeometryChange(0);
//        QWindowSystemInterface::handleScreenGeometryChange(0);
    } else {
        m_pauseApplicationMutex.unlock();
    }
}

namespace QtAndroid
{
#ifndef ANDROID_PLUGIN_OPENGL
    void flushImage(const QPoint &pos, const QImage &image, const QRect &destinationRect)
    {
        checkPauseApplication();
        QMutexLocker locker(&m_surfaceMutex);
        if (!m_surface)
            return;
        AttachedJNIEnv env;
        if (!env.jniEnv)
            return;

        int bpp = 2;
        AndroidBitmapInfo info;
        int ret;

        if ((ret = AndroidBitmap_getInfo(env.jniEnv, m_surface, &info)) < 0) {
            qWarning() << "AndroidBitmap_getInfo() failed ! error=" << ret;
            m_javaVM->DetachCurrentThread();
            return;
        }

        if (info.format != ANDROID_BITMAP_FORMAT_RGB_565) {
            qWarning() << "Bitmap format is not RGB_565!";
            m_javaVM->DetachCurrentThread();
            return;
        }

        void *pixels;
        unsigned char *screenBits;
        if ((ret = AndroidBitmap_lockPixels(env.jniEnv, m_surface, &pixels)) < 0) {
            qWarning() << "AndroidBitmap_lockPixels() failed! error=" << ret;
            m_javaVM->DetachCurrentThread();
            return;
        }

        screenBits = static_cast<unsigned char *>(pixels);
        int sbpl = info.stride;
        int swidth = info.width;
        int sheight = info.height;

        unsigned sposx = pos.x() + destinationRect.x();
        unsigned sposy = pos.y() + destinationRect.y();

        screenBits += sposy * sbpl;

        unsigned ibpl = image.bytesPerLine();
        unsigned iposx = destinationRect.x();
        unsigned iposy = destinationRect.y();

        const unsigned char *imageBits = static_cast<const unsigned char *>(image.bits());
        imageBits += iposy * ibpl;

        unsigned width = swidth - sposx < unsigned(destinationRect.width())
                         ? (swidth-sposx)
                         : destinationRect.width();
        unsigned height = sheight - sposy < unsigned(destinationRect.height())
                          ? (sheight - sposy)
                          : destinationRect.height();

        for (unsigned y = 0; y < height; y++) {
            memcpy(screenBits + y*sbpl + sposx*bpp,
                   imageBits + y*ibpl + iposx*bpp,
                   width*bpp);
        }
        AndroidBitmap_unlockPixels(env.jniEnv, m_surface);

        env.jniEnv->CallStaticVoidMethod(m_applicationClass,
                                         m_redrawSurfaceMethodID,
                                         jint(destinationRect.left()),
                                         jint(destinationRect.top()),
                                         jint(destinationRect.right() + 1),
                                         jint(destinationRect.bottom() + 1));
#warning FIXME dirty hack, figure out why it needs to add 1 to right and bottom !!!!
    }

#else // for #ifndef ANDROID_PLUGIN_OPENGL
    EGLNativeWindowType nativeWindow(bool waitForWindow)
    {
        m_surfaceMutex.lock();
        if (!m_nativeWindow && waitForWindow) {
            m_waitForWindow = true;
            m_surfaceMutex.unlock();
            m_waitForWindowSemaphore.acquire();
            m_waitForWindow = false;
            return m_nativeWindow;
        }
        m_surfaceMutex.unlock();
        return m_nativeWindow;
    }

    QSize nativeWindowSize()
    {
        if (m_nativeWindow == 0)
            return QAndroidPlatformIntegration::defaultDesktopSize();

        int width = ANativeWindow_getWidth(m_nativeWindow);
        int height = ANativeWindow_getHeight(m_nativeWindow);

        return QSize(width, height);
    }
#endif

    void setAndroidPlatformIntegration(QAndroidPlatformIntegration *androidPlatformIntegration)
    {
        m_surfaceMutex.lock();
        m_androidPlatformIntegration = androidPlatformIntegration;
        m_surfaceMutex.unlock();
    }

    QAndroidPlatformIntegration *androidPlatformIntegration()
    {
        QMutexLocker locker(&m_surfaceMutex);
        return m_androidPlatformIntegration;
    }

    QWindow *topLevelWindowAt(const QPoint &globalPos)
    {
        return m_androidPlatformIntegration
               ? m_androidPlatformIntegration->screen()->topLevelAt(globalPos)
               : 0;
    }

    int desktopWidthPixels()
    {
        return m_desktopWidthPixels;
    }

    int desktopHeightPixels()
    {
        return m_desktopHeightPixels;
    }

    double scaledDensity()
    {
        return m_scaledDensity;
    }

    JavaVM *javaVM()
    {
        return m_javaVM;
    }

    jclass findClass(const QString &className, JNIEnv *env)
    {
        return static_cast<jclass>(env->CallObjectMethod(m_classLoaderObject,
                                                         m_loadClassMethodID,
                                                         env->NewString(reinterpret_cast<const jchar *>(className.constData()),
                                                                        jsize(className.length()))));
    }

    AAssetManager *assetManager()
    {
        return m_assetManager;
    }

    jclass applicationClass()
    {
        return m_applicationClass;
    }

    jobject activity()
    {
        return m_activityObject;
    }

    jobject createBitmap(QImage img, JNIEnv *env)
    {
        if (img.format() != QImage::Format_ARGB32 && img.format() != QImage::Format_RGB16)
            img = img.convertToFormat(QImage::Format_ARGB32);

        jobject bitmap = env->CallStaticObjectMethod(m_bitmapClass,
                                                     m_createBitmapMethodID,
                                                     img.width(),
                                                     img.height(),
                                                     img.format() == QImage::Format_ARGB32
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

    jobject createBitmapDrawable(jobject bitmap, JNIEnv *env)
    {
        if (!bitmap)
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
}

static jboolean startQtAndroidPlugin(JNIEnv* /*env*/, jobject /*object*//*, jobject applicationAssetManager*/)
{
#ifndef ANDROID_PLUGIN_OPENGL
    m_surface = 0;
#else
    m_nativeWindow = 0;
    m_waitForWindow = false;
#endif

    m_androidPlatformIntegration = 0;
    m_androidAssetsFileEngineHandler = new AndroidAssetsFileEngineHandler();

#ifdef ANDROID_PLUGIN_OPENGL
    return true;
#else
    return false;
#endif
}

static void *startMainMethod(void */*data*/)
{
    char const **params;
    params = static_cast<char const **>(malloc(m_applicationParams.length() * sizeof(char *)));
    for (int i = 0; i < m_applicationParams.size(); i++)
        params[i] = static_cast<const char *>(m_applicationParams[i].constData());

    int ret = m_main(m_applicationParams.length(), const_cast<char **>(params));

    free(params);
    Q_UNUSED(ret);

    if (m_mainLibraryHnd) {
        int res = dlclose(m_mainLibraryHnd);
        if (res < 0)
            qWarning() << "dlclose failed:" << dlerror();
    }

    QtAndroid::AttachedJNIEnv env;
    if (!env.jniEnv)
        return 0;

    if (m_applicationClass) {
        jmethodID quitApp = env.jniEnv->GetStaticMethodID(m_applicationClass, "quitApp", "()V");
        env.jniEnv->CallStaticVoidMethod(m_applicationClass, quitApp);
    }

    return 0;
}

static jboolean startQtApplication(JNIEnv *env, jobject /*object*/, jstring paramsString, jstring environmentString)
{
    m_mainLibraryHnd = NULL;
    const char *nativeString = env->GetStringUTFChars(environmentString, 0);
    QByteArray string = nativeString;
    env->ReleaseStringUTFChars(environmentString, nativeString);
    m_applicationParams=string.split('\t');
    foreach (string, m_applicationParams) {
        if (putenv(string.constData()))
            qWarning() << "Can't set environment" << string;
    }

    nativeString = env->GetStringUTFChars(paramsString, 0);
    string = nativeString;
    env->ReleaseStringUTFChars(paramsString, nativeString);

    m_applicationParams=string.split('\t');

    // Go home
    QDir::setCurrent(QDir::homePath());

    //look for main()
    if (m_applicationParams.length()) {
        // Obtain a handle to the main library (the library that contains the main() function).
        // This library should already be loaded, and calling dlopen() will just return a reference to it.
        m_mainLibraryHnd = dlopen(m_applicationParams.first().data(), 0);
        if (m_mainLibraryHnd == NULL) {
            qCritical() << "dlopen failed:" << dlerror();
            return false;
        }
        m_main = (Main)dlsym(m_mainLibraryHnd, "main");
    } else {
        qWarning() << "No main library was specified; searching entire process (this is slow!)";
        m_main = (Main)dlsym(RTLD_DEFAULT, "main");
    }

    if (!m_main) {
        qCritical() << "dlsym failed:" << dlerror();
        qCritical() << "Could not find main method";
        return false;
    }

    pthread_t appThread;
    return pthread_create(&appThread, NULL, startMainMethod, NULL) == 0;
}

static void pauseQtApp(JNIEnv */*env*/, jobject /*thiz*/)
{
    m_surfaceMutex.lock();
    m_pauseApplicationMutex.lock();

    if (m_androidPlatformIntegration)
        m_androidPlatformIntegration->pauseApp();
    m_pauseApplication = true;

    m_pauseApplicationMutex.unlock();
    m_surfaceMutex.unlock();
}

static void resumeQtApp(JNIEnv */*env*/, jobject /*thiz*/)
{
    m_surfaceMutex.lock();
    m_pauseApplicationMutex.lock();
    if (m_androidPlatformIntegration)
        m_androidPlatformIntegration->resumeApp();

    if (m_pauseApplication)
        m_pauseApplicationSemaphore.release();

    m_pauseApplicationMutex.unlock();
    m_surfaceMutex.unlock();
}

static void quitQtAndroidPlugin(JNIEnv *env, jclass /*clazz*/)
{
#ifndef ANDROID_PLUGIN_OPENGL
    if (m_surface) {
        env->DeleteGlobalRef(m_surface);
        m_surface = 0;
    }
#else
    Q_UNUSED(env);
#endif

    m_androidPlatformIntegration = 0;
    delete m_androidAssetsFileEngineHandler;
}

static void terminateQt(JNIEnv *env, jclass /*clazz*/)
{
#ifndef ANDROID_PLUGIN_OPENGL
    if (m_surface)
        env->DeleteGlobalRef(m_surface);
#endif
    env->DeleteGlobalRef(m_applicationClass);
    env->DeleteGlobalRef(m_classLoaderObject);
    env->DeleteGlobalRef(m_resourcesObj);
    env->DeleteGlobalRef(m_activityObject);
    env->DeleteGlobalRef(m_bitmapClass);
    env->DeleteGlobalRef(m_ARGB_8888_BitmapConfigValue);
    env->DeleteGlobalRef(m_RGB_565_BitmapConfigValue);
    env->DeleteGlobalRef(m_bitmapDrawableClass);
}

static void setSurface(JNIEnv *env, jobject /*thiz*/, jobject jSurface)
{
#ifndef ANDROID_PLUGIN_OPENGL
    if (m_surface)
        env->DeleteGlobalRef(m_surface);
    m_surface = env->NewGlobalRef(jSurface);
#else
    m_surfaceMutex.lock();
    EGLNativeWindowType nativeWindow = ANativeWindow_fromSurface(env, jSurface);
    bool sameNativeWindow = (nativeWindow != 0 && nativeWindow == m_nativeWindow);

    m_nativeWindow = nativeWindow;
    if (m_waitForWindow)
        m_waitForWindowSemaphore.release();

    if (m_androidPlatformIntegration) {
        QSize size = QtAndroid::nativeWindowSize();

        QPlatformScreen *screen = m_androidPlatformIntegration->screen();
        QRect geometry(QPoint(0, 0), size);
        QWindowSystemInterface::handleScreenAvailableGeometryChange(screen->screen(), geometry);
        QWindowSystemInterface::handleScreenGeometryChange(screen->screen(), geometry);

        if (!sameNativeWindow) {
            m_surfaceMutex.unlock();
            m_androidPlatformIntegration->surfaceChanged();
        } else {
            // Resize all top level windows, since they share the same surface
            foreach (QWindow *w, QGuiApplication::topLevelWindows()) {
                QAndroidOpenGLPlatformWindow *window =
                        static_cast<QAndroidOpenGLPlatformWindow *>(w->handle());

                if (window != 0) {
                    window->lock();
                    window->scheduleResize(size);

                    QWindowSystemInterface::handleExposeEvent(window->window(),
                                                              QRegion(window->window()->geometry()));
                    window->unlock();
                }
            }

            m_surfaceMutex.unlock();
        }

    } else {
        m_surfaceMutex.unlock();
    }
#endif  // for #ifndef ANDROID_PLUGIN_OPENGL
}

static void destroySurface(JNIEnv *env, jobject /*thiz*/)
{
#ifndef ANDROID_PLUGIN_OPENGL
    if (m_surface) {
        env->DeleteGlobalRef(m_surface);
        m_surface = 0;
    }
#else
    Q_UNUSED(env);
    m_nativeWindow = 0;
    if (m_androidPlatformIntegration != 0)
        m_androidPlatformIntegration->invalidateNativeSurface();
#endif
}

static void setDisplayMetrics(JNIEnv */*env*/, jclass /*clazz*/,
                            jint /*widthPixels*/, jint /*heightPixels*/,
                            jint desktopWidthPixels, jint desktopHeightPixels,
                            jdouble xdpi, jdouble ydpi, jdouble scaledDensity)
{
    m_desktopWidthPixels = desktopWidthPixels;
    m_desktopHeightPixels = desktopHeightPixels;
    m_scaledDensity = scaledDensity;

    if (!m_androidPlatformIntegration) {
        QAndroidPlatformIntegration::setDefaultDisplayMetrics(desktopWidthPixels,desktopHeightPixels,
                                                                qRound(double(desktopWidthPixels)  / xdpi * 25.4),
                                                                qRound(double(desktopHeightPixels) / ydpi * 25.4));
    } else {
        m_androidPlatformIntegration->setDisplayMetrics(qRound(double(desktopWidthPixels)  / xdpi * 25.4),
                                                        qRound(double(desktopHeightPixels) / ydpi * 25.4));
        m_androidPlatformIntegration->setDesktopSize(desktopWidthPixels, desktopHeightPixels);
    }
}

static void lockSurface(JNIEnv */*env*/, jobject /*thiz*/)
{
    m_surfaceMutex.lock();
}

static void unlockSurface(JNIEnv */*env*/, jobject /*thiz*/)
{
    m_surfaceMutex.unlock();
}

static void updateWindow(JNIEnv */*env*/, jobject /*thiz*/)
{
    if (!m_androidPlatformIntegration)
        return;

    if (QGuiApplication::instance() != 0) {
        foreach (QWindow *w, QGuiApplication::topLevelWindows())
            QWindowSystemInterface::handleExposeEvent(w, QRegion(w->geometry()));
    }

#ifndef ANDROID_PLUGIN_OPENGL
    QAndroidPlatformScreen *screen = static_cast<QAndroidPlatformScreen *>(m_androidPlatformIntegration->screen());
    QMetaObject::invokeMethod(screen, "setDirty", Qt::QueuedConnection, Q_ARG(QRect,screen->geometry()));
#else
    qWarning("updateWindow: Dirty screen not implemented yet on OpenGL");
#endif
}

static void handleOrientationChanged(JNIEnv */*env*/, jobject /*thiz*/, jint newOrientation)
{
    if (m_androidPlatformIntegration == 0)
        return;

    Qt::ScreenOrientation screenOrientation = newOrientation == 1
                                              ? Qt::PortraitOrientation
                                              : Qt::LandscapeOrientation;
    QPlatformScreen *screen = m_androidPlatformIntegration->screen();
    QWindowSystemInterface::handleScreenOrientationChange(screen->screen(),
                                                          screenOrientation);
}

static JNINativeMethod methods[] = {
    {"startQtAndroidPlugin", "()Z", (void *)startQtAndroidPlugin},
    {"startQtApplication", "(Ljava/lang/String;Ljava/lang/String;)V", (void *)startQtApplication},
    {"pauseQtApp", "()V", (void *)pauseQtApp},
    {"resumeQtApp", "()V", (void *)resumeQtApp},
    {"quitQtAndroidPlugin", "()V", (void *)quitQtAndroidPlugin},
    {"terminateQt", "()V", (void *)terminateQt},
    {"setDisplayMetrics", "(IIIIDDD)V", (void *)setDisplayMetrics},
    {"setSurface", "(Ljava/lang/Object;)V", (void *)setSurface},
    {"destroySurface", "()V", (void *)destroySurface},
    {"lockSurface", "()V", (void *)lockSurface},
    {"unlockSurface", "()V", (void *)unlockSurface},
    {"updateWindow", "()V", (void *)updateWindow},
    {"handleOrientationChanged", "(I)V", (void *)handleOrientationChanged}
};

#define FIND_AND_CHECK_CLASS(CLASS_NAME) \
clazz = env->FindClass(CLASS_NAME); \
if (!clazz) { \
    __android_log_print(ANDROID_LOG_FATAL, m_qtTag, m_classErrorMsg, CLASS_NAME); \
    return JNI_FALSE; \
}

#define GET_AND_CHECK_METHOD(VAR, CLASS, METHOD_NAME, METHOD_SIGNATURE) \
VAR = env->GetMethodID(CLASS, METHOD_NAME, METHOD_SIGNATURE); \
if (!VAR) { \
    __android_log_print(ANDROID_LOG_FATAL, m_qtTag, m_methodErrorMsg, METHOD_NAME, METHOD_SIGNATURE); \
    return JNI_FALSE; \
}

#define GET_AND_CHECK_STATIC_METHOD(VAR, CLASS, METHOD_NAME, METHOD_SIGNATURE) \
VAR = env->GetStaticMethodID(CLASS, METHOD_NAME, METHOD_SIGNATURE); \
if (!VAR) { \
    __android_log_print(ANDROID_LOG_FATAL, m_qtTag, m_methodErrorMsg, METHOD_NAME, METHOD_SIGNATURE); \
    return JNI_FALSE; \
}

#define GET_AND_CHECK_FIELD(VAR, CLASS, FIELD_NAME, FIELD_SIGNATURE) \
VAR = env->GetFieldID(CLASS, FIELD_NAME, FIELD_SIGNATURE); \
if (!VAR) { \
    __android_log_print(ANDROID_LOG_FATAL, m_qtTag, m_methodErrorMsg, FIELD_NAME, FIELD_SIGNATURE); \
    return JNI_FALSE; \
}

#define GET_AND_CHECK_STATIC_FIELD(VAR, CLASS, FIELD_NAME, FIELD_SIGNATURE) \
VAR = env->GetStaticFieldID(CLASS, FIELD_NAME, FIELD_SIGNATURE); \
if (!VAR) { \
    __android_log_print(ANDROID_LOG_FATAL, m_qtTag, m_methodErrorMsg, FIELD_NAME, FIELD_SIGNATURE); \
    return JNI_FALSE; \
}

static int registerNatives(JNIEnv *env)
{
    jclass clazz;
    FIND_AND_CHECK_CLASS("org/qtproject/qt5/android/QtNative");
    m_applicationClass = static_cast<jclass>(env->NewGlobalRef(clazz));

    if (env->RegisterNatives(m_applicationClass, methods, sizeof(methods) / sizeof(methods[0])) < 0) {
        __android_log_print(ANDROID_LOG_FATAL,"Qt", "RegisterNatives failed");
        return JNI_FALSE;
    }

    GET_AND_CHECK_STATIC_METHOD(m_redrawSurfaceMethodID, m_applicationClass, "redrawSurface", "(IIII)V");

#ifdef ANDROID_PLUGIN_OPENGL
    FIND_AND_CHECK_CLASS("android/view/Surface");
    GET_AND_CHECK_FIELD(m_surfaceFieldID, clazz, "mNativeSurface", "I");
#endif

    jmethodID methodID;
    GET_AND_CHECK_STATIC_METHOD(methodID, m_applicationClass, "activity", "()Landroid/app/Activity;");
    jobject activityObject = env->CallStaticObjectMethod(m_applicationClass, methodID);
    m_activityObject = env->NewGlobalRef(activityObject);
    GET_AND_CHECK_STATIC_METHOD(methodID, m_applicationClass, "classLoader", "()Ljava/lang/ClassLoader;");
    m_classLoaderObject = env->NewGlobalRef(env->CallStaticObjectMethod(m_applicationClass, methodID));

    clazz = env->GetObjectClass(m_classLoaderObject);
    GET_AND_CHECK_METHOD(m_loadClassMethodID, clazz, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");

    FIND_AND_CHECK_CLASS("android/content/ContextWrapper");
    GET_AND_CHECK_METHOD(methodID, clazz, "getAssets", "()Landroid/content/res/AssetManager;");
    m_assetManager = AAssetManager_fromJava(env, env->CallObjectMethod(activityObject, methodID));

    GET_AND_CHECK_METHOD(methodID, clazz, "getResources", "()Landroid/content/res/Resources;");
    m_resourcesObj = env->NewGlobalRef(env->CallObjectMethod(activityObject, methodID));

    FIND_AND_CHECK_CLASS("android/graphics/Bitmap");
    m_bitmapClass = static_cast<jclass>(env->NewGlobalRef(clazz));
    GET_AND_CHECK_STATIC_METHOD(m_createBitmapMethodID, m_bitmapClass
                                , "createBitmap", "(IILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;");

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
                         "<init>",
                         "(Landroid/content/res/Resources;Landroid/graphics/Bitmap;)V");

    return JNI_TRUE;
}

Q_DECL_EXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void */*reserved*/)
{
    typedef union {
        JNIEnv *nativeEnvironment;
        void *venv;
    } UnionJNIEnvToVoid;

    __android_log_print(ANDROID_LOG_INFO, "Qt", "qt start");
    UnionJNIEnvToVoid uenv;
    uenv.venv = NULL;
    m_javaVM = 0;

    if (vm->GetEnv(&uenv.venv, JNI_VERSION_1_4) != JNI_OK) {
        __android_log_print(ANDROID_LOG_FATAL, "Qt", "GetEnv failed");
        return -1;
    }

    JNIEnv *env = uenv.nativeEnvironment;
    if (!registerNatives(env)
            || !QtAndroidInput::registerNatives(env)
            || !QtAndroidClipboard::registerNatives(env)
            || !QtAndroidMenu::registerNatives(env)) {
        __android_log_print(ANDROID_LOG_FATAL, "Qt", "registerNatives failed");
        return -1;
    }

    m_javaVM = vm;
    return JNI_VERSION_1_4;
}
