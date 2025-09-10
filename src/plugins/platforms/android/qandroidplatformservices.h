// Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef ANDROIDPLATFORMDESKTOPSERVICE_H
#define ANDROIDPLATFORMDESKTOPSERVICE_H

#include <qpa/qplatformservices.h>
#include "androidjnimain.h"
#include <QtCore/private/qjnihelpers_p.h>
#include <QtCore/qobject.h>
#include <QUrl>

QT_BEGIN_NAMESPACE

class QAndroidPlatformServices : public QObject,
                                 public QPlatformServices
#if QT_CONFIG(desktopservices)
                                 , public QtAndroidPrivate::NewIntentListener
#endif
{
public:
    QAndroidPlatformServices();

    QByteArray desktopEnvironment() const override;

#if QT_CONFIG(desktopservices)
    bool openUrl(const QUrl &url) override;
    bool openDocument(const QUrl &url) override;
    bool handleNewIntent(JNIEnv *env, jobject intent) override;

private:
    bool openURL(const QUrl &url) const;
    bool openURL(const QString &url) const;
    bool openUrlWithFileProvider(const QUrl &url);
    bool openUrlWithAuthority(const QUrl &url, const QString &authority);

    QString getMimeOfUrl(const QUrl &url) const;
    QStringList getFileProviderAuthorities(const QJniObject &context) const;
    QString getAdequateFileproviderAuthority(const QStringList &authorities) const;

private:
    QUrl m_handlingUrl;
    QString m_actionView;
#endif
};

QT_END_NAMESPACE

#endif // ANDROIDPLATFORMDESKTOPSERVICE_H
