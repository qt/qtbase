// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QString>
#include <QDBusMessage>
#include <QDBusConnection>

struct RequestData;
void appendRequest(RequestData *) {}; // stub

//! [10]
struct RequestData
{
    QString request;
    QString processedData;
    QDBusMessage reply;
};

QString processRequest(const QString &request, const QDBusMessage &message)
{
    RequestData *data = new RequestData;
    data->request = request;
    message.setDelayedReply(true);
    data->reply = message.createReply();

    appendRequest(data);
    return QString();
}
//! [10]


//! [11]
void sendReply(RequestData *data)
{
    // data->processedData has been initialized with the request's reply
    QDBusMessage &reply = data->reply;

    // send the reply over D-Bus:
    reply << data->processedData;
    QDBusConnection::sessionBus().send(reply);

    // dispose of the transaction data
    delete data;
}
//! [11]


//! [12]
Q_NOREPLY void myMethod();
//! [12]
