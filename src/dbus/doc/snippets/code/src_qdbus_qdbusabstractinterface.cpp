// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QString>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDBusInterface>

using namespace Qt::StringLiterals;

class Abstract_DBus_Interface : public QObject
{
    Q_OBJECT

public:
    Abstract_DBus_Interface(QObject *parent = nullptr)
    : QObject(parent) {
        interface = new QDBusInterface("org.example.Interface", "/Example/Methods");
    }

    ~Abstract_DBus_Interface() {  delete interface; }
    void interfaceMain();
    void asyncCall();
    QString retrieveValue() { return QString(); }

public slots:
    void callFinishedSlot();

private:
    QDBusInterface *interface;
};

void Abstract_DBus_Interface::interfaceMain()
{
//! [0]
QString value = retrieveValue();
QDBusMessage reply;

QDBusReply<int> api = interface->call("GetAPIVersion"_L1);
if (api >= 14)
  reply = interface->call("ProcessWorkUnicode"_L1, value);
else
  reply = interface->call("ProcessWork"_L1, "UTF-8"_L1, value.toUtf8());
//! [0]
}

void Abstract_DBus_Interface::asyncCall()
{
//! [1]
QDBusPendingCall pcall = interface->asyncCall("GetAPIVersion"_L1);
auto watcher = new QDBusPendingCallWatcher(pcall, this);

QObject::connect(watcher, &QDBusPendingCallWatcher::finished, this,
                 [&](QDBusPendingCallWatcher *w) {
    QString value = retrieveValue();
    QDBusPendingReply<int> reply(*w);
    QDBusPendingCall pcall;
    if (reply.argumentAt<0>() >= 14)
        pcall = interface->asyncCall("ProcessWorkUnicode"_L1, value);
    else
        pcall = interface->asyncCall("ProcessWork"_L1, "UTF-8"_L1, value.toUtf8());

    w = new QDBusPendingCallWatcher(pcall);
    QObject::connect(w,  &QDBusPendingCallWatcher::finished, this,
                     &Abstract_DBus_Interface::callFinishedSlot);
});
//! [1]
}
