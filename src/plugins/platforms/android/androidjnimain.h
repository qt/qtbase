// Copyright (C) 2014 BogDan Vatra <bogdan@kde.org>
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef ANDROID_APP_H
#define ANDROID_APP_H

#include <android/log.h>

#include <jni.h>
#include <android/asset_manager.h>

#include <QImage>
#include <private/qjnihelpers_p.h>

QT_BEGIN_NAMESPACE

class QRect;
class QPoint;
class QThread;
class QAndroidPlatformIntegration;
class QWidget;
class QString;
class QWindow;
class AndroidSurfaceClient;
class QBasicMutex;

namespace QtAndroid
{
    QBasicMutex *platformInterfaceMutex();
    QAndroidPlatformIntegration *androidPlatformIntegration();
    void setAndroidPlatformIntegration(QAndroidPlatformIntegration *androidPlatformIntegration);
    void setQtThread(QThread *thread);


    int createSurface(AndroidSurfaceClient * client, const QRect &geometry, bool onTop, int imageDepth);
    int insertNativeView(jobject view, const QRect &geometry);
    void setViewVisibility(jobject view, bool visible);
    void setSurfaceGeometry(int surfaceId, const QRect &geometry);
    void destroySurface(int surfaceId);
    void bringChildToFront(int surfaceId);
    void bringChildToBack(int surfaceId);

    QWindow *topLevelWindowAt(const QPoint &globalPos);
    int availableWidthPixels();
    int availableHeightPixels();
    double scaledDensity();
    double pixelDensity();
    JavaVM *javaVM();
    jobject assets();
    AAssetManager *assetManager();
    jclass applicationClass();
    QtJniTypes::Activity activity();
    QtJniTypes::Service service();

    // Keep synchronized with flags in ActivityDelegate.java
    enum SystemUiVisibility {
        SYSTEM_UI_VISIBILITY_NORMAL = 0,
        SYSTEM_UI_VISIBILITY_FULLSCREEN = 1,
        SYSTEM_UI_VISIBILITY_TRANSLUCENT = 2
    };
    void setSystemUiVisibility(SystemUiVisibility uiVisibility);

    jobject createBitmap(QImage img, JNIEnv *env = nullptr);
    jobject createBitmap(int width, int height, QImage::Format format, JNIEnv *env);
    jobject createBitmapDrawable(jobject bitmap, JNIEnv *env = nullptr);

    void notifyAccessibilityLocationChange(uint accessibilityObjectId);
    void notifyObjectHide(uint accessibilityObjectId, uint parentObjectId);
    void notifyObjectFocus(uint accessibilityObjectId);
    void notifyValueChanged(uint accessibilityObjectId, jstring value);
    void notifyScrolledEvent(uint accessibilityObjectId);
    void notifyQtAndroidPluginRunning(bool running);

    const char *classErrorMsgFmt();
    const char *methodErrorMsgFmt();
    const char *qtTagText();

    QString deviceName();
    bool blockEventLoopsWhenSuspended();
}

QT_END_NAMESPACE

#endif // ANDROID_APP_H
