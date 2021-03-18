/****************************************************************************
**
** Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
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

#include "androidjniclipboard.h"
#include <QtCore/QUrl>
#include <QtCore/private/qjni_p.h>

QT_BEGIN_NAMESPACE

using namespace QtAndroid;
namespace QtAndroidClipboard
{
    QAndroidPlatformClipboard *m_manager = nullptr;

    static JNINativeMethod methods[] = {
        {"onClipboardDataChanged", "()V", (void *)onClipboardDataChanged}
    };

    void setClipboardManager(QAndroidPlatformClipboard *manager)
    {
        m_manager = manager;
        QJNIObjectPrivate::callStaticMethod<void>(applicationClass(), "registerClipboardManager");
        jclass appClass = QtAndroid::applicationClass();
        QJNIEnvironmentPrivate env;
        if (env->RegisterNatives(appClass, methods, sizeof(methods) / sizeof(methods[0])) < 0) {
            __android_log_print(ANDROID_LOG_FATAL,"Qt", "RegisterNatives failed");
            return;
        }
    }
    void clearClipboardData()
    {
        QJNIObjectPrivate::callStaticMethod<void>(applicationClass(), "clearClipData");
    }
    void setClipboardMimeData(QMimeData *data)
    {
        clearClipboardData();
        if (data->hasText()) {
            QJNIObjectPrivate::callStaticMethod<void>(applicationClass(),
                                                      "setClipboardText", "(Ljava/lang/String;)V",
                                                      QJNIObjectPrivate::fromString(data->text()).object());
        }
        if (data->hasHtml()) {
            QJNIObjectPrivate::callStaticMethod<void>(applicationClass(),
                                                      "setClipboardHtml",
                                                      "(Ljava/lang/String;Ljava/lang/String;)V",
                                                      QJNIObjectPrivate::fromString(data->text()).object(),
                                                      QJNIObjectPrivate::fromString(data->html()).object());
        }
        if (data->hasUrls()) {
            QList<QUrl> urls = data->urls();
            for (const auto &u : qAsConst(urls)) {
                QJNIObjectPrivate::callStaticMethod<void>(applicationClass(), "setClipboardUri",
                                                          "(Ljava/lang/String;)V",
                                                          QJNIObjectPrivate::fromString(u.toEncoded()).object());
            }
        }
    }

    QMimeData *getClipboardMimeData()
    {
        QMimeData *data = new QMimeData;
        if (QJNIObjectPrivate::callStaticMethod<jboolean>(applicationClass(), "hasClipboardText")) {
            data->setText(QJNIObjectPrivate::callStaticObjectMethod(applicationClass(),
                                                                    "getClipboardText",
                                                                    "()Ljava/lang/String;").toString());
        }
        if (QJNIObjectPrivate::callStaticMethod<jboolean>(applicationClass(), "hasClipboardHtml")) {
            data->setHtml(QJNIObjectPrivate::callStaticObjectMethod(applicationClass(),
                                                                    "getClipboardHtml",
                                                                    "()Ljava/lang/String;").toString());
        }
        if (QJNIObjectPrivate::callStaticMethod<jboolean>(applicationClass(), "hasClipboardUri")) {
            QJNIObjectPrivate uris = QJNIObjectPrivate::callStaticObjectMethod(applicationClass(),
                                                                               "getClipboardUris",
                                                                               "()[Ljava/lang/String;");
            if (uris.isValid()) {
                QList<QUrl> urls;
                QJNIEnvironmentPrivate env;
                jobjectArray juris = static_cast<jobjectArray>(uris.object());
                const jint nUris = env->GetArrayLength(juris);
                urls.reserve(static_cast<int>(nUris));
                for (int i = 0; i < nUris; ++i)
                    urls << QUrl(QJNIObjectPrivate(env->GetObjectArrayElement(juris, i)).toString());
                data->setUrls(urls);
            }
        }
        return data;
    }

    void onClipboardDataChanged(JNIEnv */*env*/, jobject /*thiz*/)
    {
        m_manager->emitChanged(QClipboard::Clipboard);
    }
}

QT_END_NAMESPACE
