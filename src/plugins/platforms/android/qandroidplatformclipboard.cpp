// Copyright (C) 2023 The Qt Company Ltd.
// Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qandroidplatformclipboard.h"

#include <QtCore/QUrl>
#include <QtCore/QMetaObject>
#include <QtCore/QJniEnvironment>
#include <QtCore/QJniObject>
#include <QtCore/private/qjnihelpers_p.h>
#include <QtGui/QGuiApplication>
#include <QtGui/private/qinternalmimedata_p.h>

#ifndef QT_NO_CLIPBOARD

using namespace QtJniTypes;
using namespace Qt::StringLiterals;

QT_BEGIN_NAMESPACE

class QAndroidPlatformMimeData : public QInternalMimeData
{
public:
    explicit QAndroidPlatformMimeData(const QtClipboardManager *clipboardManager)
        : m_clipboardManager(clipboardManager)
    {
        if (m_clipboardManager->callMethod<jboolean>("hasClipboardText"))
            m_formats.append("text/plain"_L1);
        if (m_clipboardManager->callMethod<jboolean>("hasClipboardHtml"))
            m_formats.append("text/html"_L1);
        if (m_clipboardManager->callMethod<jboolean>("hasClipboardUri"))
            m_formats.append("text/uri-list"_L1);
    }

protected:
    bool hasFormat_sys(const QString &mimeType) const override
    {
        return m_formats.contains(mimeType);
    }

    QStringList formats_sys() const override
    {
        return m_formats;
    }

    QVariant retrieveData_sys(const QString &mimeType, QMetaType) const override
    {
        if (mimeType == "text/plain"_L1)
            return m_clipboardManager->callMethod<QString>("getClipboardText").toUtf8();
        if (mimeType == "text/html"_L1)
            return m_clipboardManager->callMethod<QString>("getClipboardHtml").toUtf8();
        if (mimeType == "text/uri-list"_L1) {
            QByteArray result;
            const auto uris = m_clipboardManager->callMethod<QString[]>("getClipboardUris");
            if (!uris.isValid())
                return result;
            for (const QString &uri : uris) {
                result += QUrl(uri).toEncoded();
                result += "\r\n";
            }
            return result;
        }
        return {};
    }

private:
    const QtClipboardManager *const m_clipboardManager;
    QStringList m_formats;
};

void QAndroidPlatformClipboard::onClipboardDataChanged(JNIEnv *env, jobject obj, jlong nativePointer)
{
    Q_UNUSED(env)
    Q_UNUSED(obj)

    auto *clipboardManager = reinterpret_cast<QAndroidPlatformClipboard *>(nativePointer);
    if (clipboardManager)
        QMetaObject::invokeMethod(QGuiApplication::clipboard(), [] {
            QClipboard *clipboard = QGuiApplication::clipboard();
            emit clipboard->dataChanged();
            emit clipboard->changed(QClipboard::Clipboard);
        }, Qt::QueuedConnection);
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
    return new QAndroidPlatformMimeData(&m_clipboardManager);
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
            m_clipboardManager.callMethod<void>("setClipboardUri", context,
                                                 u.toString(QUrl::FullyEncoded));
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
