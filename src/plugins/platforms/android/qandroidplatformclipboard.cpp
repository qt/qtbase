// Copyright (C) 2023 The Qt Company Ltd.
// Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qandroidplatformclipboard.h"

#include <QtCore/QUrl>
#include <QtCore/QJniEnvironment>
#include <QtCore/QJniObject>
#include <QtCore/private/qjnihelpers_p.h>

#ifndef QT_NO_CLIPBOARD

using namespace QtJniTypes;

QT_BEGIN_NAMESPACE

void QAndroidPlatformClipboard::onClipboardDataChanged(JNIEnv *env, jobject obj, jlong nativePointer)
{
    Q_UNUSED(env)
    Q_UNUSED(obj)

    auto *clipboardManager = reinterpret_cast<QAndroidPlatformClipboard *>(nativePointer);
    if (clipboardManager)
        clipboardManager->emitChanged(QClipboard::Clipboard);
}

QAndroidPlatformClipboard::QAndroidPlatformClipboard()
{
    m_clipboardManager = QtClipboardManager::construct(QtAndroidPrivate::context(),
                                                       reinterpret_cast<jlong>(this));
}

QAndroidPlatformClipboard::~QAndroidPlatformClipboard()
{
    if (data)
        delete data;
}

QMimeData *QAndroidPlatformClipboard::getClipboardMimeData()
{
    QMimeData *data = new QMimeData;
    if (m_clipboardManager.callMethod<jboolean>("hasClipboardText")) {
        data->setText(m_clipboardManager.callMethod<QString>("getClipboardText"));
    }
    if (m_clipboardManager.callMethod<jboolean>("hasClipboardHtml")) {
        data->setHtml(m_clipboardManager.callMethod<QString>("getClipboardHtml"));
    }
    if (m_clipboardManager.callMethod<jboolean>("hasClipboardUri")) {
        auto uris = m_clipboardManager.callMethod<QString[]>("getClipboardUris");
        if (uris.isValid()) {
            QList<QUrl> urls;
            for (const QString &uri : uris)
                urls << QUrl(uri);
            data->setUrls(urls);
        }
    }
    return data;
}

QMimeData *QAndroidPlatformClipboard::mimeData(QClipboard::Mode mode)
{
    Q_UNUSED(mode);
    Q_ASSERT(supportsMode(mode));
    if (data)
        data->deleteLater();
    data = getClipboardMimeData();
    return data;
}

void QAndroidPlatformClipboard::clearClipboardData()
{
    m_clipboardManager.callMethod<void>("clearClipData");
}

void QAndroidPlatformClipboard::setClipboardMimeData(QMimeData *data)
{
    clearClipboardData();
    auto context = QtAndroidPrivate::context();
    if (data->hasUrls()) {
        QList<QUrl> urls = data->urls();
        for (const auto &u : std::as_const(urls))
            m_clipboardManager.callMethod<void>("setClipboardUri", context, u.toEncoded());
    } else if (data->hasHtml()) { // html can contain text
        m_clipboardManager.callMethod<void>("setClipboardHtml",
                                            context, data->text(), data->html());
    } else if (data->hasText()) { // hasText must be the last (the order matter here)
        m_clipboardManager.callMethod<void>("setClipboardText", context, data->text());
    }
}

void QAndroidPlatformClipboard::setMimeData(QMimeData *data, QClipboard::Mode mode)
{
    if (!data) {
        clearClipboardData();
        return;
    }
    if (data && supportsMode(mode))
        setClipboardMimeData(data);
    if (data != 0)
        data->deleteLater();
}

bool QAndroidPlatformClipboard::supportsMode(QClipboard::Mode mode) const
{
    return QClipboard::Clipboard == mode;
}

bool QAndroidPlatformClipboard::registerNatives(QJniEnvironment &env)
{
    bool success = env.registerNativeMethods(Traits<QtClipboardManager>::className(),
                { Q_JNI_NATIVE_SCOPED_METHOD(onClipboardDataChanged, QAndroidPlatformClipboard) });
    if (!success) {
        qCritical() << "QtClipboardManager: registerNativeMethods() failed";
        return false;
    }

    return true;
}

QT_END_NAMESPACE

#endif // QT_NO_CLIPBOARD
