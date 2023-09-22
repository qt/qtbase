// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QCoreApplication>
#include <QDebug>
#include <QVariant>
#include "message.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QStringList headers;
    headers << "Subject: Hello World"
            << "From: address@example.com";
    QString body = "This is a test.\r\n";

//! [printing a custom type]
    Message message(body, headers);
    qDebug() << "Original:" << message;
//! [printing a custom type]

//! [storing a custom value]
    QVariant stored;
    stored.setValue(message);
//! [storing a custom value]

    qDebug() << "Stored:" << stored;

//! [retrieving a custom value]
    Message retrieved = qvariant_cast<Message>(stored);
    qDebug() << "Retrieved:" << retrieved;
    retrieved = qvariant_cast<Message>(stored);
    qDebug() << "Retrieved:" << retrieved;
//! [retrieving a custom value]

    return 0;
}
