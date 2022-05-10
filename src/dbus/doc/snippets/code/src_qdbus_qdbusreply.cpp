// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QDBusPendingCall>
#include <QDBusInterface>
#include <QDBusPendingReply>
#include <QDBusReply>

class DBus_Process_String_Interface : public QObject
{
    Q_OBJECT

public:
    DBus_Process_String_Interface(QObject *parent = nullptr)
    : QObject(parent) {
        interface = new QDBusInterface("org.example.Interface", "/Example/Methods");
    }

    ~DBus_Process_String_Interface() {  delete interface; }
    void QDBus_reply();
    void useValue(QVariant);
    void showError(const QDBusError&);
public slots:

private:
    QDBusInterface *interface;
};
void DBus_Process_String_Interface::QDBus_reply()
{
//! [0]
QDBusReply<QString> reply = interface->call("RemoteMethod");
if (reply.isValid())
    // use the returned value
    useValue(reply.value());
else
    // call failed. Show an error condition.
    showError(reply.error());
//! [0]


//! [1]
reply = interface->call("RemoteMethod");
//! [1]
}
