// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QDBusPendingCall>
#include <QDBusInterface>
#include <QDBusPendingReply>

class DBus_PendingReply_Interface : public QObject
{
    Q_OBJECT

public:
    DBus_PendingReply_Interface(QObject *parent = nullptr)
    : QObject(parent) {
        iface = new QDBusInterface("org.example.Interface", "/Example/Methods");
    }

    ~DBus_PendingReply_Interface() {  delete iface; }
    void callInterfaceMainR();
    void PendingReplyString();
    void PendingReplyBool();
    void showErrorD(QDBusError);
    void showSuccess(QVariant);
    void showFailure(QVariant);
    void useValue(QDBusPendingReplyTypes::Select<0, QString, void, void, void, void, void, void, void>::Type);
public slots:

private:
    QDBusInterface *iface;
};

void DBus_PendingReply_Interface::PendingReplyString()
{
//! [0]
    QDBusPendingReply<QString> reply = iface->asyncCall("RemoteMethod");
    reply.waitForFinished();
    if (reply.isError())
        // call failed. Show an error condition.
        showErrorD(reply.error());
    else
        // use the returned value
        useValue(reply.value());
//! [0]
}

void DBus_PendingReply_Interface::PendingReplyBool()
{
//! [2]
    QDBusPendingReply<bool, QString> reply = iface->asyncCall("RemoteMethod");
    reply.waitForFinished();
    if (!reply.isError()) {
        if (reply.argumentAt<0>())
            showSuccess(reply.argumentAt<1>());
        else
            showFailure(reply.argumentAt<1>());
    }
//! [2]
}
