// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QString>
#include <QDBusConnection>
#include <QDBusArgument>
#include <QDBusMetaType>
#include <QDBusMessage>
#include <QDBusContext>

//! [0]
class MyObject: public QObject,
                protected QDBusContext
{
    Q_OBJECT

    QDBusConnection conn;
    QDBusMessage msg;

    //...

protected slots:
    void process();
public slots:
    void methodWithError();
    QString methodWithDelayedReply();
};

void MyObject::methodWithError()
{
    sendErrorReply(QDBusError::NotSupported,
                   "The method call 'methodWithError()' is not supported");
}

QString MyObject::methodWithDelayedReply()
{
    conn = connection();
    msg = message();
    setDelayedReply(true);
    QMetaObject::invokeMethod(this, &MyObject::process, Qt::QueuedConnection);
    return QString();
}
//! [0]
