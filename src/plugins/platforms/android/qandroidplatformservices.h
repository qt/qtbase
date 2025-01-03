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
                                 public QPlatformServices,
                                 public QtAndroidPrivate::NewIntentListener
{
public:
    QAndroidPlatformServices();

    bool openUrl(const QUrl &url) override;
    bool openDocument(const QUrl &url) override;
    QByteArray desktopEnvironment() const override;

    bool handleNewIntent(JNIEnv *env, jobject intent) override;

private:
    QUrl m_handlingUrl;
    QString m_actionView;
};

QT_END_NAMESPACE

#endif // ANDROIDPLATFORMDESKTOPSERVICE_H
