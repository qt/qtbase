// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QCoreApplication>
#include <QDBusServer>

#include "../myobject.h"

static const char serviceName[] = "org.qtproject.autotests.qmyserver";
static const char objectPath[] = "/org/qtproject/qmyserver";
//static const char *interfaceName = serviceName;

int MyObject::callCount = 0;
QVariantList MyObject::callArgs;

class MyServer : public QDBusServer, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.qtproject.autotests.qmyserver")

public:
    MyServer(QObject *parent = nullptr)
        : QDBusServer(parent),
          m_conn("none")
    {
        connect(this, SIGNAL(newConnection(QDBusConnection)), SLOT(handleConnection(QDBusConnection)));
    }

public slots:
    QString address() const
    {
        if (!QDBusServer::isConnected())
            sendErrorReply(QDBusServer::lastError().name(), QDBusServer::lastError().message());
        return QDBusServer::address();
    }

    void waitForConnected()
    {
        if (callPendingReply.type() != QDBusMessage::InvalidMessage) {
            sendErrorReply(QDBusError::NotSupported, "One call already pending!");
            return;
        }
        if (m_conn.isConnected())
            return;
        // not connected, we'll reply later
        setDelayedReply(true);
        callPendingReply = message();
    }

    void emitSignal(const QString &interface, const QString &name, const QString &arg)
    {
        QDBusMessage msg = QDBusMessage::createSignal("/", interface, name);
        msg << arg;
        m_conn.send(msg);
    }

    void reset()
    {
        MyObject::callCount = 0;
        obj.m_complexProp.clear();
    }

    int callCount()
    {
        return MyObject::callCount;
    }

    QVariantList callArgs()
    {
        return MyObject::callArgs;
    }

    void setProp1(int val)
    {
        obj.m_prop1 = val;
    }

    int prop1()
    {
        return obj.m_prop1;
    }

    void setComplexProp(QList<int> val)
    {
        obj.m_complexProp = val;
    }

    QList<int> complexProp()
    {
        return obj.m_complexProp;
    }

    bool interactiveAuthorization()
    {
        if (message().isInteractiveAuthorizationAllowed())
            return true;

        sendErrorReply(QStringLiteral("org.freedesktop.DBus.Error.InteractiveAuthorizationRequired"),
                       QStringLiteral("Interactive authentication required."));
        return false;
    }

    void quit()
    {
        qApp->quit();
    }

private slots:
    void handleConnection(const QDBusConnection& con)
    {
        m_conn = con;
        m_conn.registerObject("/", &obj, QDBusConnection::ExportAllProperties
                       | QDBusConnection::ExportAllSlots
                       | QDBusConnection::ExportAllInvokables);
        if (callPendingReply.type() != QDBusMessage::InvalidMessage) {
            QDBusConnection::sessionBus().send(callPendingReply.createReply());
            callPendingReply = QDBusMessage();
        }
    }

private:
    QDBusConnection m_conn;
    QDBusMessage callPendingReply;
    MyObject obj;
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QDBusConnection con = QDBusConnection::sessionBus();
    if (!con.isConnected())
        exit(1);

    if (!con.registerService(serviceName))
        exit(2);

    MyServer server;
    con.registerObject(objectPath, &server, QDBusConnection::ExportAllSlots);

    printf("ready.\n");
    fflush(stdout);

    return app.exec();
}

#include "qmyserver.moc"
