/****************************************************************************
**
** Copyright (C) 2014 BogDan Vatra <bogdan@kde.org>
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef ANDROID_APP_H
#define ANDROID_APP_H

#include <android/log.h>

#include <jni.h>
#include <android/asset_manager.h>

#include <QImage>

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
    int desktopWidthPixels();
    int desktopHeightPixels();
    double scaledDensity();
    double pixelDensity();
    JavaVM *javaVM();
    jobject assets();
    AAssetManager *assetManager();
    jclass applicationClass();
    jobject activity();
    jobject service();

    void showStatusBar();
    void hideStatusBar();

    jobject createBitmap(QImage img, JNIEnv *env = 0);
    jobject createBitmap(int width, int height, QImage::Format format, JNIEnv *env);
    jobject createBitmapDrawable(jobject bitmap, JNIEnv *env = 0);

    void notifyAccessibilityLocationChange();
    void notifyObjectHide(uint accessibilityObjectId);
    void notifyObjectFocus(uint accessibilityObjectId);

    const char *classErrorMsgFmt();
    const char *methodErrorMsgFmt();
    const char *qtTagText();

    QString deviceName();
    bool blockEventLoopsWhenSuspended();
}

QT_END_NAMESPACE

#endif // ANDROID_APP_H
