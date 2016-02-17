/****************************************************************************
**
** Copyright (C) 2014 BogDan Vatra <bogdan@kde.org>
** Copyright (C) 2016 The Qt Company Ltd.
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

namespace QtAndroid
{
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
    AAssetManager *assetManager();
    jclass applicationClass();
    jobject activity();
    jobject service();

    void setApplicationActive();

    void showStatusBar();
    void hideStatusBar();

    jobject createBitmap(QImage img, JNIEnv *env = 0);
    jobject createBitmap(int width, int height, QImage::Format format, JNIEnv *env);
    jobject createBitmapDrawable(jobject bitmap, JNIEnv *env = 0);

    const char *classErrorMsgFmt();
    const char *methodErrorMsgFmt();
    const char *qtTagText();

    QString deviceName();
    bool blockEventLoopsWhenSuspended();
}

QT_END_NAMESPACE

#endif // ANDROID_APP_H
