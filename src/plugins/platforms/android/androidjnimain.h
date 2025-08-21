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
#include <QtCore/QJniObject>
#include <androidbackendregister.h>

QT_BEGIN_NAMESPACE

class QRect;
class QPoint;
class QThread;
class QAndroidPlatformIntegration;
class QWidget;
class QString;
class QWindow;
class QAndroidPlatformWindow;
class QBasicMutex;

Q_DECLARE_JNI_CLASS(QtActivityDelegateBase, "org/qtproject/qt/android/QtActivityDelegateBase")
Q_DECLARE_JNI_CLASS(QtInputDelegate, "org/qtproject/qt/android/QtInputDelegate")

namespace QtAndroid
{
    QBasicMutex *platformInterfaceMutex();
    QAndroidPlatformIntegration *androidPlatformIntegration();
    AndroidBackendRegister *backendRegister();
    void setAndroidPlatformIntegration(QAndroidPlatformIntegration *androidPlatformIntegration);
    void setQtThread(QThread *thread);
    void setViewVisibility(jobject view, bool visible);

    QWindow *topLevelWindowAt(const QPoint &globalPos);
    QWindow *windowFromId(int windowId);
    int availableWidthPixels();
    int availableHeightPixels();
    double pixelDensity();
    jobject assets();
    AAssetManager *assetManager();
    jclass applicationClass();

    jobject createBitmap(QImage img, JNIEnv *env = nullptr);
    jobject createBitmap(int width, int height, QImage::Format format, JNIEnv *env);
    jobject createBitmapDrawable(jobject bitmap, JNIEnv *env = nullptr);

#if QT_CONFIG(accessibility)
    void notifyAccessibilityLocationChange(uint accessibilityObjectId);
    void notifyObjectHide(uint accessibilityObjectId, uint parentObjectId);
    void notifyObjectShow(uint parentObjectId);
    void notifyObjectFocus(uint accessibilityObjectId);
    void notifyValueChanged(uint accessibilityObjectId, jstring value);
    void notifyDescriptionOrNameChanged(uint accessibilityObjectId, const QString &value);
    void notifyScrolledEvent(uint accessibilityObjectId);
    void notifyAnnouncementEvent(uint accessibilityObjectId, const QString &message);
#endif
    void notifyNativePluginIntegrationReady(bool ready);

    const char *classErrorMsgFmt();
    const char *methodErrorMsgFmt();
    const char *qtTagText();

    QString deviceName();
    bool blockEventLoopsWhenSuspended();

    bool isQtApplication();
}

QT_END_NAMESPACE

#endif // ANDROID_APP_H
