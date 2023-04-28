// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QDBusPendingCall>
#include <QDBusInterface>
#include <QDBusPendingReply>

class DBus_PendingCall_Interface : public QObject
{
    Q_OBJECT

public:
    DBus_PendingCall_Interface(QObject *parent = nullptr)
    : QObject(parent) {
        iface = new QDBusInterface("org.example.Interface", "/Example/Methods");
    }

    ~DBus_PendingCall_Interface() {  delete iface; }
    void callInterfaceMain();
    void showError();
    void showReply(QString&, QByteArray&);
    QString value1;
    QString value2;
    void callFinishedSlot(QDBusPendingCallWatcher *call);
public slots:

private:
    QDBusInterface *iface;
};

void DBus_PendingCall_Interface::callInterfaceMain()
{
//! [0]
    QDBusPendingCall async = iface->asyncCall("RemoteMethod", value1, value2);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(async, this);

    QObject::connect(watcher, &QDBusPendingCallWatcher::finished, this,
                     &DBus_PendingCall_Interface::callFinishedSlot);
//! [0]

}

//! [1]
void DBus_PendingCall_Interface::callFinishedSlot(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<QString, QByteArray> reply = *call;
    if (reply.isError()) {
        showError();
    } else {
        QString text = reply.argumentAt<0>();
        QByteArray data = reply.argumentAt<1>();
        showReply(text, data);
    }
    call->deleteLater();
}
//! [1]
