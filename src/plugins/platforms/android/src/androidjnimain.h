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

#ifndef ANDROID_APP_H
#define ANDROID_APP_H

#include <android/log.h>

#ifdef ANDROID_PLUGIN_OPENGL
#  include <EGL/eglplatform.h>
#endif

#include <QtCore/qsize.h>

#include <jni.h>
#include <android/asset_manager.h>

class QImage;
class QRect;
class QPoint;
class QThread;
class QAndroidPlatformIntegration;
class QWidget;
class QString;
class QWindow;

namespace QtAndroid
{
    QAndroidPlatformIntegration *androidPlatformIntegration();
    void setAndroidPlatformIntegration(QAndroidPlatformIntegration *androidPlatformIntegration);
    void setQtThread(QThread *thread);

    void setFullScreen(QWidget *widget);

#ifndef ANDROID_PLUGIN_OPENGL
    void flushImage(const QPoint &pos, const QImage &image, const QRect &rect);
#else
    EGLNativeWindowType nativeWindow(bool waitToCreate = true);
    QSize nativeWindowSize();
#endif

    QWindow *topLevelWindowAt(const QPoint &globalPos);
    int desktopWidthPixels();
    int desktopHeightPixels();
    double scaledDensity();
    JavaVM *javaVM();
    jclass findClass(const QString &className, JNIEnv *env);
    AAssetManager *assetManager();
    jclass applicationClass();
    jobject activity();

    jobject createBitmap(QImage img, JNIEnv *env = 0);
    jobject createBitmapDrawable(jobject bitmap, JNIEnv *env = 0);

    struct AttachedJNIEnv
    {
        AttachedJNIEnv()
        {
            attached = false;
            if (QtAndroid::javaVM()->GetEnv((void**)&jniEnv, JNI_VERSION_1_6) < 0) {
                if (QtAndroid::javaVM()->AttachCurrentThread(&jniEnv, NULL) < 0) {
                    __android_log_print(ANDROID_LOG_ERROR, "Qt", "AttachCurrentThread failed");
                    jniEnv = 0;
                    return;
                }
                attached = true;
            }
        }

        ~AttachedJNIEnv()
        {
            if (attached)
                QtAndroid::javaVM()->DetachCurrentThread();
        }
        bool attached;
        JNIEnv *jniEnv;
    };
    const char *classErrorMsgFmt();
    const char *methodErrorMsgFmt();
    const char *qtTagText();

}
#endif // ANDROID_APP_H
