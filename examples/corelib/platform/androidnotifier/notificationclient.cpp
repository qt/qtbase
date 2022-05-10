// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "notificationclient.h"

#include <QtCore/qjniobject.h>
#include <QtCore/qcoreapplication.h>

NotificationClient::NotificationClient(QObject *parent)
    : QObject(parent)
{
    connect(this, &NotificationClient::notificationChanged,
            this, &NotificationClient::updateAndroidNotification);
}

void NotificationClient::setNotification(const QString &notification)
{
    if (m_notification == notification)
        return;

//! [notification changed signal]
    m_notification = notification;
    emit notificationChanged();
//! [notification changed signal]
}

QString NotificationClient::notification() const
{
    return m_notification;
}

//! [Send notification message to Java]
void NotificationClient::updateAndroidNotification()
{
    QJniObject javaNotification = QJniObject::fromString(m_notification);
    QJniObject::callStaticMethod<void>(
                    "org/qtproject/example/androidnotifier/NotificationClient",
                    "notify",
                    "(Landroid/content/Context;Ljava/lang/String;)V",
                    QNativeInterface::QAndroidApplication::context(),
                    javaNotification.object<jstring>());
}
//! [Send notification message to Java]
