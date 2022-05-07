/****************************************************************************
**
** Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
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
******************************************************************************/

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
